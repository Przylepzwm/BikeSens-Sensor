#pragma once
#include <Arduino.h>
#include <esp_system.h>


// ---------------- Piny sprzętowe ----------------
// Ustalone dla Twojej płytki ESP32-C3 SuperMini:
// - GPIO 4: HALL (wybudzanie)
// - GPIO 2: ADC (pomiar baterii przez dzielnik)
// - GPIO 8: wbudowana dioda (LED)
static constexpr gpio_num_t GPIO_HALL_WAKE = GPIO_NUM_4;
static constexpr bool HALL_WAKE_ACTIVE_LOW = true;   // Hall pulls LOW on magnet
#define LED_PIN 8
static constexpr bool LED_ENABLED = false;

// Minimalny odstęp czasu między zliczanymi zboczami Halla (debounce / filtr szumu)
static constexpr uint32_t HALL_DEBOUNCE_US = 10000; // 5 ms

static constexpr int ADC_BAT_PIN = A0; // GPIO0 (ADC)

// -------------- Battery divider -----------------
// Vbat -> R1 -> ADC -> R2 -> GND
// Vadc = Vbat * R2 / (R1+R2)
// Ustalone: R1=220k (VBAT->ADC), R2=100k (ADC->GND)
static constexpr float DIVIDER_R1_OHMS = 220000.0f;
static constexpr float DIVIDER_R2_OHMS = 100000.0f;

// -------------- Build-time sanity checks --------
// (żeby już nigdy nie wróciły „placeholdery” i kręcenie się w kółko)
static_assert((int)GPIO_HALL_WAKE == 4, "Config sanity: expected HALL on GPIO4");
static_assert(ADC_BAT_PIN == A0, "Config sanity: expected ADC on GPIO0");
static_assert((int)DIVIDER_R1_OHMS == 220000, "Config sanity: expected R1=220k");
static_assert((int)DIVIDER_R2_OHMS == 100000, "Config sanity: expected R2=100k");

// -------------- Serial / boot -------------------
// Minimalny delay na start UART (jeśli w ogóle używasz Serial w buildzie testowym)
#define DEBUG_SERIAL 0
static constexpr uint8_t SERIAL_BOOT_DELAY_MS = 5;

// -------------- Battery thresholds --------------
static constexpr uint16_t VBAT_WARN_MV = 3500;  // warning threshold (tune)
static constexpr uint16_t VBAT_CRIT_MV = 3300;  // critical threshold (tune)

// Battery percent mapping (linear, clamp)
// 0% at VBAT_EMPTY_MV, 100% at VBAT_FULL_MV
static constexpr uint16_t VBAT_EMPTY_MV = 3300;
static constexpr uint16_t VBAT_FULL_MV  = 4200;

// -------------- ADC sampling --------------------
static constexpr uint8_t  ADC_SAMPLES = 10;
static constexpr uint8_t  ADC_SAMPLE_DELAY_MS = 3;

// -------------- Window logic --------------------
// Measurement/counting window length
static constexpr uint32_t WINDOW_MS = 60UL * 1000UL; //60->10->60
// If no new window starts within this time after comm ends -> deep sleep
static constexpr uint32_t NEXT_WINDOW_TIMEOUT_MS = 2500UL; //60->10->8->4->2.5

// -------------- BLE "komunikacja" --------------
// Number of advertising "shots" per window report
static constexpr uint8_t  BLE_REPEAT_COUNT = 5;
// How long a single "shot" keeps advertising ON (ms)
static constexpr uint16_t BLE_ADV_ON_MS = 140; // >=100ms recommended
// Random gap between shots (ms)
static constexpr uint16_t BLE_GAP_MIN_MS = 300; //30->50->300
static constexpr uint16_t BLE_GAP_MAX_MS = 450; //80 ->120->450

// Losowy backoff przed pierwszym „shotem” (zmniejsza kolizje gdy wiele urządzeń startuje równocześnie)
static constexpr uint16_t BLE_BACKOFF_MAX_MS = 400;

// Interwały advertising (jednostki 0.625 ms). 32=20ms, 80=50ms
static constexpr uint16_t BLE_ADV_MIN_INTERVAL = 48;  // 30 ms
static constexpr uint16_t BLE_ADV_MAX_INTERVAL = 80;  // 50 ms

// Company ID dla Manufacturer Specific Data (0xFFFF to „test”; możesz zmienić później)
static constexpr uint16_t BLE_COMPANY_ID = 0xBAFF;

// Prefiks payloadu do szybkiego filtrowania po stronie mastera (2 bajty)
static constexpr uint8_t  BLE_PREFIX0 = 0xB1;
static constexpr uint8_t  BLE_PREFIX1 = 0x6B; // 'k'
