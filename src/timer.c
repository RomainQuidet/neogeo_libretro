#include "timer.h"


#pragma mark - Private

static void check_timeout(timer_t* timer) {
	if (timer->active == false || (timer->remaining_cycles > 0))
		return;
	
	timer->active = false;
	
	if (timer->callback)
		timer->callback();
}

#pragma mark - Public

void timer_arm(timer_t* timer, const int32_t master_cycles) {
	timer->remaining_cycles = master_cycles;
	timer->active = true;
	check_timeout(timer);
}

void timer_arm_relative(timer_t* timer, const int32_t master_cycles) {
	timer->remaining_cycles += master_cycles;
	timer->active = true;
	check_timeout(timer);
}

void timer_run(timer_t* timer, const int32_t master_cycles) {
	if (timer->active == false) {
		return;
	}
	
	timer->remaining_cycles -= master_cycles;
	check_timeout(timer);
}
