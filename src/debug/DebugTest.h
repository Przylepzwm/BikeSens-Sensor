#pragma once
#include <Arduino.h>
#include "config/Config.h"
#include "debug/Debug.h"

/**
 * Narzędzia pomocnicze do testów / diagnostyki.
 * Kompilowane wyłącznie gdy DEBUG_SERIAL == 1.
 */
#if DEBUG_SERIAL
  void DebugTest_updateHallEdgeMonitor(bool windowActive);
  uint32_t DebugTest_getTestPulses();
#endif
