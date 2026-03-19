#include "debug/IsrTrace.h"
#include "config/Config.h"

volatile uint32_t g_isr_event_bits = 0;

volatile uint16_t g_isr_cnt_entered = 0;
volatile uint16_t g_isr_cnt_timeout_cancel = 0;
volatile uint16_t g_isr_cnt_drop_min_gap = 0;
volatile uint16_t g_isr_cnt_drop_on_while_armed = 0;
volatile uint16_t g_isr_cnt_on_armed = 0;
volatile uint16_t g_isr_cnt_off_no_arm = 0;
volatile uint16_t g_isr_cnt_drop_min_width = 0;
volatile uint16_t g_isr_cnt_drop_max_width = 0;
volatile uint16_t g_isr_cnt_pulse_accepted = 0;

volatile uint16_t g_isr_last_pulse_value = 0;
volatile uint32_t g_isr_last_width_us = 0;
volatile uint32_t g_isr_total_accepted = 0;

static inline void atomicCopyAndClear32(volatile uint32_t &src, uint32_t &out) {
  noInterrupts();
  out = src;
  src = 0;
  interrupts();
}

static inline void atomicCopyAndClear16(volatile uint16_t &src, uint16_t &out) {
  noInterrupts();
  out = src;
  src = 0;
  interrupts();
}

static inline void atomicCopyAndClearU32(volatile uint32_t &src, uint32_t &out) {
  noInterrupts();
  out = src;
  src = 0;
  interrupts();
}

void dumpIsrTrace() {
#if DEBUG_SERIAL
  uint32_t bits = 0;
  atomicCopyAndClear32(g_isr_event_bits, bits);
  if (bits == 0) return;

  uint16_t c_entered=0, c_to=0, c_gap=0, c_onWhile=0, c_on=0, c_offNo=0, c_minw=0, c_maxw=0, c_ok=0;
  atomicCopyAndClear16(g_isr_cnt_entered, c_entered);
  atomicCopyAndClear16(g_isr_cnt_timeout_cancel, c_to);
  atomicCopyAndClear16(g_isr_cnt_drop_min_gap, c_gap);
  atomicCopyAndClear16(g_isr_cnt_drop_on_while_armed, c_onWhile);
  atomicCopyAndClear16(g_isr_cnt_on_armed, c_on);
  atomicCopyAndClear16(g_isr_cnt_off_no_arm, c_offNo);
  atomicCopyAndClear16(g_isr_cnt_drop_min_width, c_minw);
  atomicCopyAndClear16(g_isr_cnt_drop_max_width, c_maxw);
  atomicCopyAndClear16(g_isr_cnt_pulse_accepted, c_ok);

  uint16_t lastPulse = 0;
  uint32_t lastWidth = 0;
  atomicCopyAndClear16(g_isr_last_pulse_value, lastPulse);
  atomicCopyAndClearU32(g_isr_last_width_us, lastWidth);

  Serial.println();
  Serial.println("[ISR TRACE]");
  if (bits & EVT_ISR_ENTERED)        Serial.printf("- ISR entered: %u\n", c_entered);

  if (bits & EVT_ON_ARMED)           Serial.printf("- ON armed: %u\n", c_on);
  //if (bits & EVT_PULSE_ACCEPTED)     Serial.printf("- PULSE accepted: %u (last=%u, width_us=%lu)\n", c_ok, lastPulse, (unsigned long)lastWidth);
if (bits & EVT_PULSE_ACCEPTED)     Serial.printf("- PULSE accepted (total): %lu\n", (unsigned long)g_isr_total_accepted);

  if (bits & EVT_DROP_MIN_GAP)       Serial.printf("- drop MIN_GAP: %u\n", c_gap);
  if (bits & EVT_DROP_ON_WHILE_ARMED)Serial.printf("- drop ON while armed: %u\n", c_onWhile);
  if (bits & EVT_OFF_NO_ARM)         Serial.printf("- drop OFF without ARM: %u\n", c_offNo);
  if (bits & EVT_DROP_MIN_WIDTH)     Serial.printf("- drop MIN_WIDTH: %u\n", c_minw);
  if (bits & EVT_DROP_MAX_WIDTH)     Serial.printf("- drop MAX_WIDTH: %u\n", c_maxw);
  if (bits & EVT_TIMEOUT_CANCEL)     Serial.printf("- cancel TIMEOUT/MAX_WIDTH: %u\n", c_to);
#endif
}
