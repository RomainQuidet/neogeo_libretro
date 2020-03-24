#ifndef video_h
#define video_h

#include "memory_region.h"

#include <stdint.h>

static const uint32_t FRAMEBUFFER_WIDTH = 320;
static const uint32_t FRAMEBUFFER_HEIGHT = 224;
static const float ASPECT_RATIO = 4.0f / 3.0f;
static const uint16_t MAX_SPRITES_PER_SCREEN = 381;
static const uint16_t MAX_SPRITES_PER_LINE = 96;

#define TIMER_CTRL_IRQ_ENABLED_MASK			0x10
#define TIMER_CTRL_RELOAD_LOW_WRITE_MASK	0x20
#define TIMER_CTRL_RELOAD_FRAME_START_MASK	0x40
#define TIMER_CTRL_RELOAD_EMPTY_MASK		0x80

typedef struct video {
	uint16_t* palettes_colors;
	uint8_t* fixUsageMap;
	uint16_t* frameBuffer;
	memory_region_t vram;		// VRAM - https://wiki.neogeodev.org/index.php?title=VRAM
	uint32_t timer_counter;
	uint8_t auto_animation_speed;
	bool auto_animation_disabled;
	uint32_t auto_animation_counter;
	uint32_t auto_animation_frame_counter;
	uint8_t timer_control;
} video_t;

extern video_t video;

#pragma mark - Lifecycle

void video_init(void);
void video_reset(void);
uint32_t video_reload_timer(void);

#pragma mark - Drawing

void video_draw_empty_line(uint32_t scanline);
void video_draw_fix(uint32_t scanline);

void video_create_sprites_list(uint32_t scanline);
void video_draw_sprites(uint32_t scanline);

#pragma mark - Palettes helpers

void video_convert_current_palette_bank(void);
void video_convert_current_palette_color(uint32_t index);

#endif /* video */
