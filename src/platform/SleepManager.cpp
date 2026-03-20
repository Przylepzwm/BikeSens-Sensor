#include "SleepManager.h"
#include "../config/Config.h"

void SleepManager::configureHallWake(gpio_num_t pin, bool activeLow) {
  // Konfiguracja pinu jako wejście + odpowiednie podciąganie
  gpio_config_t io_conf{};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = 1ULL << pin;
  io_conf.pull_down_en = activeLow ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE;
  io_conf.pull_up_en   = activeLow ? GPIO_PULLUP_ENABLE   : GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  // Upewnij się, że podciągania zostaną zachowane w deep sleep (RTC IO).
  // GPIO4 na ESP32-C3 jest RTC-capable; dla innych pinów te wywołania mogą być no-op.
  gpio_pullup_dis(pin);
  gpio_pulldown_dis(pin);
  if (activeLow) {
    gpio_pullup_en(pin);
  } else {
    gpio_pulldown_en(pin);
  }

  // Włącz wybudzanie z deep sleep na poziomie GPIO (ESP32-C3)
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  // Jeśli magnes trzyma już stan aktywny, uzbrajamy wake-up na "zwolnienie",
  // żeby uniknąć natychmiastowego wybudzenia zaraz po wejściu w sen.
  const int levelNow = gpio_get_level(pin);
  const int activeLevel = activeLow ? 0 : 1;
  const int inactiveLevel = activeLow ? 1 : 0;

  const int wakeLevel = (levelNow == activeLevel) ? inactiveLevel : activeLevel;

  const uint64_t mask = 1ULL << (uint8_t)pin;
  esp_deep_sleep_enable_gpio_wakeup(
    mask,
    wakeLevel == 0 ? ESP_GPIO_WAKEUP_GPIO_LOW : ESP_GPIO_WAKEUP_GPIO_HIGH
  );
}

void SleepManager::enterDeepSleep() {
#if DEBUG_SERIAL
  // Minimalny flush UART (tylko debug)
  Serial.flush();
  delay(5);
#endif
  esp_deep_sleep_start();
}
