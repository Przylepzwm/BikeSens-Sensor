#pragma once
#include <Arduino.h>

/*
  Makra debugujące:
  - gdy DEBUG_SERIAL == 1: wypisują na Serial
  - gdy DEBUG_SERIAL == 0: są całkowicie wyłączone (nie generują kodu)
*/
#if DEBUG_SERIAL
  #define DBG(x) Serial.println(x)
  #define DBGF(fmt, ...) Serial.printf((fmt), ##__VA_ARGS__)
#else
  #define DBG(x) do {} while (0)
  #define DBGF(fmt, ...) do {} while (0)
#endif
