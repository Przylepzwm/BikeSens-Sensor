#pragma once
#include <Arduino.h>

/**
 * Monitor baterii (pomiar przez ADC + dzielnik rezystorowy).
 *
 * Uwaga: nie zmienia żadnych wartości konfiguracyjnych — tylko enkapsuluje logikę pomiaru.
 */
class BatteryMonitor {
public:
  BatteryMonitor(int adcPin, float r1_ohms, float r2_ohms);

  /** Inicjalizacja ADC oraz parametrów uśredniania próbek. */
  void begin(uint8_t samples, uint8_t sampleDelayMs);

  /** Odczyt napięcia baterii w mV (uwzględnia dzielnik R1/R2). */
  uint16_t readBatteryMilliVolts() const;

  /**
   * Proste mapowanie liniowe mV -> 0..100% (z ograniczeniem).
   * Jest celowo „tanie” obliczeniowo i deterministyczne.
   */
  uint8_t percentFromMilliVolts(uint16_t vbat_mV, uint16_t empty_mV, uint16_t full_mV) const;

private:
  int _adcPin;
  float _r1, _r2;
  uint8_t _samples = 8;
  uint8_t _sampleDelayMs = 2;
};
