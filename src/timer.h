#ifndef timer_h
#define timer_h

#include <stdint.h>
#include <math.h>
#include <stdbool.h>

//CLOCKS - https://wiki.neogeodev.org/index.php?title=Clock

//FRAME RATE - https://wiki.neogeodev.org/index.php?title=Framerate
/*
 NTSC AES
 	24.167829MHz main clock / 4 = 6.041957MHz pixel clock
 	6.041957MHz / 384 pixels per line = 15.734kHz horizontal rate
 	15.734kHz / 264 lines = 59.599 frames/second
 
 PAL AES
 	24.167829MHz main clock / 4 = 6.041957MHz pixel clock
 	6.041957MHz / 384 pixels per line = 15.734kHz horizontal rate
 	15.734kHz / 312 lines = 50.429 frames/second
 
 MVS
 	24.000000MHz main clock / 4 = 6.000000MHz pixel clock
 	6MHz / 384 pixels per line = 15.625kHz horizontal rate
 	15.625kHz / 264 lines = 59.1856 frames/second
 */

//DISPLAY TIMING - https://wiki.neogeodev.org/index.php?title=Display_timing
/*
 NTSC: 384 * 264 pixels.
 PAL: 384 * 312 pixels.
 mclk refers to the 24MHz master clock. A pixel lasts 4 mclk.
 There are 264 scanlines per frame:
 	8 scanlines vertical sync pulse
 	16 scanlines top border (active in PAL, blanked in NTSC)
 	224 scanlines active display
 	16 scanlines bottom border (active in PAL, blanked in NTSC)
 */

//FRAME SIZE - https://wiki.neogeodev.org/index.php?title=Frame_size
/*
 Active video NTSC: 320 x 224 pixels. Ratio: 1.428:1 (10/7)
 Active video PAL: 320 x 256 pixels (16 pixels more on top and bottom). Ratio: 1.25:1 (5/4)
 */

static const double MASTER_CLOCK = 24000000.0; //24167828.0;
static const double M68K_CLOCK = MASTER_CLOCK / 2;
static const double Z80_CLOCK = MASTER_CLOCK / 6;
static const double YM2610_CLOCK = MASTER_CLOCK / 3;
static const double PIXEL_CLOCK = MASTER_CLOCK / 4;		// 4 mckl per pixel
static const int32_t HORIZONTAL_PIXELS = 384;
static const int32_t VERTICAL_PIXELS = 264;
static const int32_t FIRST_ACTIVE_LINE = 16;
static const int32_t VBLANK_LINE = FIRST_ACTIVE_LINE + 224; // 240
static const int32_t WATCHDOG_DELAY = (int32_t)(MASTER_CLOCK * 0.13516792);
static const int32_t MASTER_CYCLES_PER_FRAME = (uint32_t)((MASTER_CLOCK / PIXEL_CLOCK) * HORIZONTAL_PIXELS * VERTICAL_PIXELS);
static const double FRAME_RATE = PIXEL_CLOCK / (double)(HORIZONTAL_PIXELS * VERTICAL_PIXELS);


static inline const int32_t secondsToMaster(double value) {
	return (int32_t)round(value * MASTER_CLOCK);
}

static inline const double masterToSeconds(int32_t value) {
	return (double)value / MASTER_CLOCK;
}

static inline const int32_t m68kToMaster(int32_t value)
{
	return (int32_t)round((double)value * (MASTER_CLOCK / M68K_CLOCK));
}

static inline const int32_t z80ToMaster(int32_t value)
{
	return (int32_t)round((double)value * (MASTER_CLOCK / Z80_CLOCK));
}

static inline const int32_t pixelToMaster(int32_t value)
{
	return (int32_t)round((double)value * (MASTER_CLOCK / PIXEL_CLOCK));
}

static inline const int32_t masterToM68k(int32_t value)
{
	return (int32_t)round((double)value / (MASTER_CLOCK / M68K_CLOCK));
}

static inline const int32_t masterToZ80(int32_t value)
{
	return (int32_t)round((double)value / (MASTER_CLOCK / Z80_CLOCK));
}

static inline const int32_t masterToPixel(int32_t value)
{
	return (int32_t)round((double)value / (MASTER_CLOCK / PIXEL_CLOCK));
}

typedef void(timer_callback)(void);

typedef struct timer {
	bool active;
	int32_t remaining_cycles;
	timer_callback *callback;
	uint32_t context;
} timer_t;

void timer_arm(timer_t* timer, const int32_t master_cycles);
void timer_arm_relative(timer_t* timer, const int32_t master_cycles);
void timer_run(timer_t* timer, const int32_t master_cycles);

#endif /* timer_h */
