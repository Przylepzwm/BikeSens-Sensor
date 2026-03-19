#include "debug/DebugTest.h"

#if DEBUG_SERIAL

// Lokalny stan tylko do testów (nie zaśmieca Config.h)
static bool     s_lastHallState = true;   // poprzedni poziom GPIO (HIGH = spoczynek)
static uint32_t s_testPulses    = 0;

void DebugTest_updateHallEdgeMonitor(bool windowActive) {
  if (windowActive) {
    // W czasie okna liczymy zbocza „ręcznie” (debug) — ISR liczy produkcyjnie.
    const bool hall = digitalRead(GPIO_HALL_WAKE);
    if (hall == false && s_lastHallState == true) {
      s_testPulses++;
      DBGF("HALL GPIO=%d test_pulses=%lu\n", (int)hall, (unsigned long)s_testPulses);
    }
    s_lastHallState = hall;
  } else {
      s_testPulses = 0;
  }
}

uint32_t DebugTest_getTestPulses() {
  return s_testPulses;
}

#endif

