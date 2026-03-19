#pragma once
#include <Arduino.h>

// Bitmask zdarzeń zbieranych w ISR (ustawiane przez if-y)
enum IsrEventBits : uint32_t {
  EVT_ISR_ENTERED      = 1u << 0,

  EVT_TIMEOUT_CANCEL   = 1u << 1,  // ACTIVE za długo (MAX_WIDTH) -> anuluj walidację
  EVT_DROP_MIN_GAP     = 1u << 2,  // zbyt szybko po poprzednim zaliczonym impulsie
  EVT_DROP_ON_WHILE_ARMED = 1u << 3, // kolejne ON gdy już walidujemy (drgania)

  EVT_ON_ARMED         = 1u << 4,  // start walidacji (ON)
  EVT_OFF_NO_ARM       = 1u << 5,  // OFF bez wcześniejszego ON

  EVT_DROP_MIN_WIDTH   = 1u << 6,  // impuls za krótki
  EVT_DROP_MAX_WIDTH   = 1u << 7,  // impuls za długi

  EVT_PULSE_ACCEPTED   = 1u << 8,  // impuls zaliczony
};

// Zdarzenia i liczniki (ISR zapisuje, loop() odczytuje i zeruje)
extern volatile uint32_t g_isr_event_bits;

extern volatile uint16_t g_isr_cnt_entered;
extern volatile uint16_t g_isr_cnt_timeout_cancel;
extern volatile uint16_t g_isr_cnt_drop_min_gap;
extern volatile uint16_t g_isr_cnt_drop_on_while_armed;
extern volatile uint16_t g_isr_cnt_on_armed;
extern volatile uint16_t g_isr_cnt_off_no_arm;
extern volatile uint16_t g_isr_cnt_drop_min_width;
extern volatile uint16_t g_isr_cnt_drop_max_width;
extern volatile uint16_t g_isr_cnt_pulse_accepted;

// Dodatkowe dane do debugowania: "wartość zliczonego impulsu"
extern volatile uint16_t g_isr_last_pulse_value;   // g_hall_pulses po ++
extern volatile uint32_t g_isr_last_width_us;      // szerokość ostatniego zaliczonego impulsu
extern volatile uint32_t g_isr_total_accepted;     // Licznik narastający (nie jest zerowany przy dumpie) – żeby widzieć np. 98

// Wywołaj w loop(): wypisuje co się wydarzyło i czyści bufory ISR
void dumpIsrTrace();
