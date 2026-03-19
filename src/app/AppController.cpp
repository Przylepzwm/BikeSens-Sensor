/*
  BikeSensSensor – BLE Advertising (zliczanie w oknie czasowym)
  Cel: ESP32-C3 (Arduino-ESP32 v3.x)

  Zachowanie wysokopoziomowe:
  - Urządzenie śpi (deep sleep), gdy jest bezczynne.
  - Impuls z Halla wybudza urządzenie i otwiera okno zliczania.
  - W trakcie okna zliczamy impulsy Halla w ISR.
  - Napięcie baterii mierzymy raz, możliwie wcześnie po starcie okna.
  - Po zakończeniu okna wysyłamy kilka reklam BLE z losowymi przerwami.
  - Jeśli po komunikacji nie pojawi się nowe okno, urządzenie wraca do deep sleep.

  Telegram do mastera (Manufacturer Specific Data):
  - prefix (2B)
  - seq (numer okna; zawijanie jest OK)
  - device_id (pochodny od MAC)
  - pulses_in_window
  - battery_percent (0..100)
*/
#include <Arduino.h>
#include <esp_system.h>

#include "app/AppController.h"
#include "platform/BatteryMonitor.h"
#include "comm/BleAdvComm.h"
#include "config/Config.h"
#include "debug/Debug.h"
#include "debug/DebugTest.h"
#include "debug/IsrTrace.h"
#include "platform/SleepManager.h"

namespace {

RTC_DATA_ATTR uint16_t g_seq = 0;

struct WindowRuntime {
  bool active = false;
  uint32_t end_ms = 0;
  uint16_t pulses = 0;
};

struct BatterySnapshot {
  uint16_t vbat_mV = 0;
  uint8_t percent = 0;
};

struct CommRuntime {
  bool active = false;
  bool adv_running = false;
  uint8_t repeats_left = 0;
  uint32_t next_start_ms = 0;
  uint32_t adv_stop_ms = 0;
  BleTelegram telegram{};
};

static volatile uint16_t g_hall_pulses = 0;
static WindowRuntime g_window;
static BatterySnapshot g_battery;
static CommRuntime g_comm;
static uint32_t g_idle_deadline_ms = 0;
static bool g_pending_telegram = false;
static BleTelegram g_pending{};
static BleAdvComm g_ble;
static BatteryMonitor g_batt(ADC_BAT_PIN, DIVIDER_R1_OHMS, DIVIDER_R2_OHMS);
static uint16_t g_deviceId16 = 0;

static void IRAM_ATTR onHallEdge() {
  // Liczymy pełny impuls ON->OFF, filtrując zbyt krótkie / zbyt długie stany
  // oraz impulsy pojawiające się zbyt szybko po poprzednim zaakceptowanym.
  static uint32_t t_on_us = 0;
  static uint32_t last_accept_us = 0;
  static bool armed = false;

  const uint32_t now_us = (uint32_t)micros();
  const int level = gpio_get_level((gpio_num_t)GPIO_HALL_WAKE);

  const uint32_t min_gap_us = HALL_DEBOUNCE_US;
  const uint32_t min_width_us = 2000;
  const uint32_t max_width_us = 2000000;

  const int active_level = HALL_WAKE_ACTIVE_LOW ? 0 : 1;
  const int inactive_level = 1 - active_level;

  if (armed && (uint32_t)(now_us - t_on_us) > max_width_us) {
    armed = false;
    g_isr_event_bits |= EVT_TIMEOUT_CANCEL;
    g_isr_cnt_timeout_cancel++;
  }

  if (level == active_level) {
    if ((uint32_t)(now_us - last_accept_us) < min_gap_us) {
      g_isr_event_bits |= EVT_DROP_MIN_GAP;
      g_isr_cnt_drop_min_gap++;
      return;
    }
    if (armed) {
      g_isr_event_bits |= EVT_DROP_ON_WHILE_ARMED;
      g_isr_cnt_drop_on_while_armed++;
      return;
    }

    t_on_us = now_us;
    armed = true;
    g_isr_event_bits |= EVT_ON_ARMED;
    g_isr_cnt_on_armed++;
    return;
  }

  if (level != inactive_level) return;

  if (!armed) {
    g_isr_event_bits |= EVT_OFF_NO_ARM;
    g_isr_cnt_off_no_arm++;
    return;
  }

  const uint32_t width_us = (uint32_t)(now_us - t_on_us);
  armed = false;

  if (width_us < min_width_us) {
    g_isr_event_bits |= EVT_DROP_MIN_WIDTH;
    g_isr_cnt_drop_min_width++;
    return;
  }
  if (width_us > max_width_us) {
    g_isr_event_bits |= EVT_DROP_MAX_WIDTH;
    g_isr_cnt_drop_max_width++;
    return;
  }

  if (g_hall_pulses != 0xFFFF) {
    g_hall_pulses++;
    g_isr_last_pulse_value = g_hall_pulses;
  }
  g_isr_last_width_us = width_us;

  g_isr_event_bits |= EVT_PULSE_ACCEPTED;
  g_isr_cnt_pulse_accepted++;
  g_isr_total_accepted++;

  last_accept_us = now_us;
}

static uint16_t takeHallPulses() {
  noInterrupts();
  const uint16_t count = g_hall_pulses;
  g_hall_pulses = 0;
  interrupts();
  return count;
}

static uint16_t makeDeviceId16() {
  const uint64_t mac64 = ESP.getEfuseMac();
  uint8_t mac[6];
  mac[0] = (uint8_t)(mac64 >> 40);
  mac[1] = (uint8_t)(mac64 >> 32);
  mac[2] = (uint8_t)(mac64 >> 24);
  mac[3] = (uint8_t)(mac64 >> 16);
  mac[4] = (uint8_t)(mac64 >> 8);
  mac[5] = (uint8_t)(mac64);

  uint16_t x = (uint16_t)mac[0] << 8 | mac[1];
  x ^= (uint16_t)mac[2] << 8 | mac[3];
  x ^= (uint16_t)mac[4] << 8 | mac[5];
  x ^= 0xB1C1;
  return x;
}

static void goDeepSleepNow() {
#if DEBUG_SERIAL
  DBGF("Dobranoc \n");
  digitalWrite(LED_PIN, HIGH);
  delay(80);
  digitalWrite(LED_PIN, LOW);
#endif

  g_ble.stop();
  SleepManager::configureHallWake(GPIO_HALL_WAKE, HALL_WAKE_ACTIVE_LOW);
  SleepManager::enterDeepSleep();
}

static void measureBatteryOnceOrSleep() {
  g_battery.vbat_mV = g_batt.readBatteryMilliVolts();
  g_battery.percent =
      g_batt.percentFromMilliVolts(g_battery.vbat_mV, VBAT_EMPTY_MV, VBAT_FULL_MV);

#if DEBUG_SERIAL == 0
  if (g_battery.vbat_mV < VBAT_CRIT_MV) {
    goDeepSleepNow();
  }
#endif
}

static void startWindow(uint16_t initialPulses) {
  g_window.active = true;
  g_window.pulses = initialPulses;
  g_window.end_ms = millis() + WINDOW_MS;

  DBGF("Window started \n");
  measureBatteryOnceOrSleep();

  g_idle_deadline_ms = 0;
}

static void beginCommForTelegram(const BleTelegram& telegram) {
  g_comm.telegram = telegram;
  g_ble.setTelegram(telegram);
  g_comm.active = true;
  g_comm.adv_running = false;
  g_comm.repeats_left = BLE_REPEAT_COUNT;
  g_comm.next_start_ms = millis() + (uint32_t)random(0, (uint32_t)BLE_BACKOFF_MAX_MS + 1);
  g_comm.adv_stop_ms = 0;
}

static void endWindowAndScheduleComm() {
  DBG("Window ended \n");
  DBGF("Pulses in window: %u\n", g_window.pulses);

  BleTelegram telegram{};
  telegram.device_id = g_deviceId16;
  telegram.seq = ++g_seq;
  telegram.pulses_in_window = g_window.pulses;
  telegram.battery_percent = g_battery.percent;

  g_window.active = false;
  g_window.pulses = 0;

  if (!g_comm.active) {
    beginCommForTelegram(telegram);
  } else {
    g_pending_telegram = true;
    g_pending = telegram;
  }
}

static void commStep() {
  if (!g_comm.active) return;

  const uint32_t now = millis();

  if (!g_comm.adv_running) {
    if (now >= g_comm.next_start_ms) {
      g_ble.begin(g_deviceId16);
      g_ble.start();
      g_comm.adv_running = true;
      g_comm.adv_stop_ms = now + BLE_ADV_ON_MS;
    }
    return;
  }

  if (now < g_comm.adv_stop_ms) return;

  g_ble.stop();
  g_comm.adv_running = false;

  if (g_comm.repeats_left > 0) {
    g_comm.repeats_left--;
  }

  if (g_comm.repeats_left == 0) {
    g_comm.active = false;

    DBG("BLE telegram sent \n");
    DBGF("SEQ=%u device=0x%04X pulses=%u batt=%u%% \n",
         g_comm.telegram.seq,
         g_comm.telegram.device_id,
         g_comm.telegram.pulses_in_window,
         g_comm.telegram.battery_percent);

    if (g_pending_telegram) {
      g_pending_telegram = false;
      beginCommForTelegram(g_pending);
      return;
    }

    if (!g_window.active) {
      g_idle_deadline_ms = millis() + NEXT_WINDOW_TIMEOUT_MS;
    }
    return;
  }

  const uint32_t gap =
      (uint32_t)random((uint32_t)BLE_GAP_MIN_MS, (uint32_t)BLE_GAP_MAX_MS + 1);
  g_comm.next_start_ms = millis() + gap;
}

}  // namespace

void appSetup() {
  randomSeed(esp_random());
  g_deviceId16 = makeDeviceId16();

#if DEBUG_SERIAL
  Serial.begin(115200);
  delay(SERIAL_BOOT_DELAY_MS);
  DBG("Boot");
  DBGF("Company ID: 0x%04X\n", BLE_COMPANY_ID);
  DBGF("Device ID: 0x%04X\n", g_deviceId16);

  const esp_sleep_wakeup_cause_t cause_dbg = esp_sleep_get_wakeup_cause();
  DBGF("Wake cause: %d\n", (int)cause_dbg);
  DBGF("Reset reason: %d\n", (int)esp_reset_reason());
  DBGF("Hall state at boot: %d\n", digitalRead((int)GPIO_HALL_WAKE));

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(200);
#endif

  SleepManager::configureHallWake(GPIO_HALL_WAKE, HALL_WAKE_ACTIVE_LOW);
  g_batt.begin(ADC_SAMPLES, ADC_SAMPLE_DELAY_MS);

  pinMode((int)GPIO_HALL_WAKE, INPUT_PULLUP);
  attachInterrupt((int)GPIO_HALL_WAKE, onHallEdge, CHANGE);

  const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause == ESP_SLEEP_WAKEUP_GPIO) {
    startWindow(1);
  } else {
    g_idle_deadline_ms = millis() + NEXT_WINDOW_TIMEOUT_MS;
  }
}

void appLoop() {
  const uint16_t newPulses = takeHallPulses();

  if (newPulses > 0) {
    if (g_window.active) {
      const uint32_t sum = (uint32_t)g_window.pulses + newPulses;
      g_window.pulses = (sum > 0xFFFF) ? 0xFFFF : (uint16_t)sum;
    } else {
      startWindow(newPulses);
    }
  }

  if (g_window.active && (int32_t)(millis() - g_window.end_ms) >= 0) {
    endWindowAndScheduleComm();
  }

  commStep();

  if (!g_window.active && !g_comm.active && g_idle_deadline_ms != 0) {
    if ((int32_t)(millis() - g_idle_deadline_ms) >= 0) {
      goDeepSleepNow();
    }
  }

#if DEBUG_SERIAL
  DebugTest_updateHallEdgeMonitor(g_window.active);
  dumpIsrTrace();
#endif

  delay(1);
}
