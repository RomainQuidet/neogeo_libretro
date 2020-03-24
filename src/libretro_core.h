#ifndef libretro_core_h
#define libretro_core_h

#include "libretro.h"

typedef struct libretro_callbacks
{
	retro_log_printf_t log;
	retro_video_refresh_t video;
	retro_input_poll_t inputPoll;
	retro_input_state_t inputState;
	retro_environment_t environment;
	retro_audio_sample_t audio;
	retro_audio_sample_batch_t audioBatch;
	struct retro_perf_callback perf;
} libretro_callbacks_t;

extern libretro_callbacks_t libretroCallbacks;

void retro_core_init_log(void);
void retro_core_create_neogeo(const char *);
void retro_core_poll_joypad_1(void);
void retro_core_poll_joypad_2(void);

#pragma mark - Debug

void retro_core_draw_mire(const uint16_t *frameBuffer, uint16_t width, uint16_t height);

#endif /* libretro_core_h */
