#include "BatteryMonitor.h"

#if defined(ARDUINO_ARCH_ESP32)
  #include <driver/gpio.h>
#endif

BatteryMonitor::BatteryMonitor(int adcPin, float r1_ohms, float r2_ohms)
: _adcPin(adcPin), _r1(r1_ohms), _r2(r2_ohms) {}

void BatteryMonitor::begin(uint8_t samples, uint8_t sampleDelayMs) {
  _samples = max<uint8_t>(1, samples);
  _sampleDelayMs = sampleDelayMs;

#if defined(ARDUINO_ARCH_ESP32)
  const gpio_num_t g = (gpio_num_t)_adcPin;

  // Reset konfiguracji GPIO (usuwa pull-up/down i inne tryby)
  gpio_reset_pin(g);
  gpio_set_direction(g, GPIO_MODE_INPUT);

  // Wyłącz podciąganie
  gpio_pullup_dis(g);
  gpio_pulldown_dis(g);
#endif

  pinMode(_adcPin, INPUT);

#if defined(ARDUINO_ARCH_ESP32)
  // Użyj API Arduino-ESP32 z kalibracją ADC (jeśli dostępne)
  analogReadResolution(12);
#endif
}

uint16_t BatteryMonitor::readBatteryMilliVolts() const {
  uint32_t acc = 0;
  for (uint8_t i = 0; i < _samples; ++i) {
#if defined(ARDUINO_ARCH_ESP32)
    // Zwraca mV z kalibracją (preferowane na ESP32)
    acc += (uint32_t)analogReadMilliVolts(_adcPin);
#else
    acc += (uint32_t)analogRead(_adcPin);
#endif
    if (_sampleDelayMs) delay(_sampleDelayMs);
  }

  const float vadc_mV = (float)acc / (float)_samples;
  const float scale = (_r1 + _r2) / _r2;
  const float vbat_mV = vadc_mV * scale;
  return (uint16_t)(vbat_mV + 0.5f);
}

uint8_t BatteryMonitor::percentFromMilliVolts(uint16_t vbat_mV, uint16_t empty_mV, uint16_t full_mV) const {
  if (full_mV <= empty_mV) return 0;
  if (vbat_mV <= empty_mV) return 0;
  if (vbat_mV >= full_mV) return 100;

  const uint32_t num = (uint32_t)(vbat_mV - empty_mV) * 100UL;
  const uint32_t den = (uint32_t)(full_mV - empty_mV);
  return (uint8_t)((num + den / 2) / den);
}
