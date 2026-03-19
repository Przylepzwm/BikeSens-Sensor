#pragma once
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

/**
 * Zarządzanie deep sleep + wybudzanie na GPIO (ESP32-C3).
 *
 * ESP32-C3 (Arduino-ESP32 v3 / IDF5): używamy API GPIO wakeup,
 * bo EXT0/EXT1 nie są dostępne.
 */
class SleepManager {
public:
  /** Konfiguracja wybudzania na poziomie GPIO (LOW/HIGH). */
  static void configureHallWake(gpio_num_t pin, bool activeLow);

  /** Wejście w deep sleep (z krótkim flush UART tylko w DEBUG). */
  static void enterDeepSleep();
};
