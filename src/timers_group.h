#ifndef timers_group_h
#define timers_group_h

#include "timer.h"

typedef struct timers_group {
	timer_t*  watchdog;
	timer_t*  video_timer;
	timer_t*  drawline;
	timer_t*  ym2610TimerA;
	timer_t*  ym2610TimerB;
	timer_t*  audioCommandTimer;
	
} timers_group_t;

extern timers_group_t timers;

void timers_group_init(void);
void timers_group_reset(void);
uint32_t timer_group_cycles_before_next_event(void);
void timer_group_consume_cycles(uint32_t cycles);

uint32_t timer_group_get_current_y_scanline(void);

#endif /* timers_group_h */
