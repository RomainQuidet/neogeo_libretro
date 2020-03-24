#include "timers_group.h"
#include "log.h"
#include "neogeo.h"
#include "video.h"

#include "3rdParty/musashi/m68kcpu.h"
#include "3rdParty/pd4990a/pd4990a.h"
#include "3rdParty/ym/ym2610.h"


timers_group_t timers;

static timer_t  watchdog;
static timer_t  video_timer;
static timer_t  drawline;
static timer_t  ym2610TimerA;
static timer_t  ym2610TimerB;
//static timer_t  audioCommandTimer;

static const double PD4990A_CLOCK = 60;	// Custom code, need 60Hz update
static timer_t pd4990a;		// RTC clock custom code

timer_t* all_timers[7];

#pragma mark - Private

static void watchdog_callback(void) {
	LOG(LOG_ERROR, "WARNING: Watchdog timer triggered (PC=%06X, SR=%04X); Machine reset.\n",
		m68k_get_reg(NULL, M68K_REG_PPC),
		m68k_get_reg(NULL, M68K_REG_SR));
	
	m68k_pulse_reset();
}

static void vblank_callback(void) {
	LOG(LOG_DEBUG, "vblank_callback\n");
	if (video.timer_control & TIMER_CTRL_RELOAD_FRAME_START_MASK) {
		uint32_t counter = video_reload_timer();
		timer_arm(&video_timer, pixelToMaster(counter));
	}
	
	cpu_68k_set_interrupt(VBlank);

	if (!video.auto_animation_frame_counter)
	{
		video.auto_animation_frame_counter = video.auto_animation_speed;
		video.auto_animation_counter++;
	}
	else
		video.auto_animation_frame_counter--;
}

static void video_timer_callback(void) {
	LOG(LOG_DEBUG, "video_timer_callback\n");
	if (video.timer_control & TIMER_CTRL_IRQ_ENABLED_MASK) {
		cpu_68k_set_interrupt(Timer);
	}
	if (video.timer_control & TIMER_CTRL_RELOAD_EMPTY_MASK) {
		uint32_t counter = video_reload_timer();
		timer_arm(&video_timer, pixelToMaster(counter));
	}
}

static uint32_t scanline = 0;
static void draw_line_callback(void) {
//	LOG(LOG_DEBUG, "draw_line_callback #%u\n", scanline);
	if (scanline == VBLANK_LINE) {
		vblank_callback();
	}
	if (scanline >= FIRST_ACTIVE_LINE && scanline < VBLANK_LINE) {
		video_draw_empty_line(scanline);
		
		video_create_sprites_list(scanline);
		video_draw_sprites(scanline);
		
		video_draw_fix(scanline);
	}
	
	scanline++;
	if (scanline == VERTICAL_PIXELS) {
		LOG(LOG_DEBUG, "draw_line_callback all line done, up to 0\n");
		scanline = 0;
	}
	timer_arm_relative(&drawline, pixelToMaster(HORIZONTAL_PIXELS));
}

static void ym2610TimerACallback(void)
{
	ym2610_timerOver(0);
}

static void ym2610TimerBCallback(void)
{
	ym2610_timerOver(1);
}

static void pd4990a_callback(void) {
//	LOG(LOG_DEBUG, "pd4990a_callback\n");
	pd4990a_addretrace();
	timer_arm_relative(&pd4990a, MASTER_CLOCK / PD4990A_CLOCK);
}

#pragma mark - Public

void timers_group_init() {
	uint8_t index = 0;
	watchdog.callback = &watchdog_callback;
	timers.watchdog = &watchdog;
	all_timers[index] = &watchdog;
	index++;
	
	timers.video_timer = &video_timer;
	video_timer.callback = &video_timer_callback;
	all_timers[index] = &video_timer;
	index++;
	
	timers.drawline = &drawline;
	drawline.callback = &draw_line_callback;
	all_timers[index] = &drawline;
	index++;
	
	timers.ym2610TimerA = &ym2610TimerA;
	ym2610TimerA.callback = &ym2610TimerACallback;
	all_timers[index] = &ym2610TimerA;
	index++;
	
	timers.ym2610TimerB = &ym2610TimerB;
	ym2610TimerB.callback = &ym2610TimerBCallback;
	all_timers[index] = &ym2610TimerB;
	index++;
	
	pd4990a.callback = &pd4990a_callback;
}

void timers_group_reset() {
	timer_arm(&watchdog, WATCHDOG_DELAY);
	watchdog.active = false;
	
	timer_arm(&drawline, pixelToMaster(HORIZONTAL_PIXELS));
	video_timer.active = false;
	timer_arm(&pd4990a, MASTER_CLOCK / PD4990A_CLOCK);
	
	ym2610TimerA.active = false;
	ym2610TimerB.active = false;
}

uint32_t timer_group_cycles_before_next_event() {
	uint32_t cycles = MASTER_CYCLES_PER_FRAME;
	for (uint8_t index = 0; index < 7; index++) {
		timer_t *timer = all_timers[index];
		if (timer != NULL && timer->active == true) {
			cycles = cycles < timer->remaining_cycles ? cycles : timer->remaining_cycles;
		}
	}
	return cycles;
}

void timer_group_consume_cycles(uint32_t cycles) {
	for (uint8_t index = 0; index < 7; index++) {
		timer_t *timer = all_timers[index];
		if (timer != NULL) {
			timer_run(timer, cycles);
		}
	}
	if (video_timer.active == true) {
		video.timer_counter -= masterToPixel(cycles);
	}
	timer_run(&pd4990a, cycles);
}

uint32_t timer_group_get_current_y_scanline() {
	return scanline;
}
