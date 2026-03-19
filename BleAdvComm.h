#pragma once
#include <Arduino.h>
#include "Config.h"

// NimBLE-Arduino (h2zero)
#include <NimBLEDevice.h>

/**
 * Telegram wysyłany do „mastera” (Manufacturer Specific Data).
 * Uwaga: wszystkie pola wielobajtowe są w little-endian.
 */
struct BleTelegram {
  uint16_t device_id;
  uint16_t seq;
  uint16_t pulses_in_window;
  uint8_t  battery_percent; // 0..100
};

/**
 * Prosta warstwa nad NimBLE Advertising.
 * Przygotowuje payload i uruchamia/zatrzymuje advertising „shoty”.
 */
class BleAdvComm {
public:
  void begin(uint16_t deviceId16);
  void setTelegram(const BleTelegram& t);
  void start();
  void stop();
  bool isRunning() const { return _running; }

private:
  static void writeLE16(uint8_t* dst, size_t off, uint16_t v);

  bool _inited = false;
  bool _running = false;
  bool _dirty = true;
  NimBLEAdvertising* _adv = nullptr;
  BleTelegram _telegram{};
};
