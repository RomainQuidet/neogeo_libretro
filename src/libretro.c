#include <stdio.h>

#include "libretro.h"
#include "cartridge.h"
#include "libretro_core.h"
#include "neogeo.h"
#include "log.h"
#include "sound.h"
#include "video.h"

#pragma mark - Properties



#pragma mark - libretro Interface

void retro_set_environment(retro_environment_t cb) {
	libretroCallbacks.environment = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
	libretroCallbacks.video = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
	libretroCallbacks.audio = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
	libretroCallbacks.audioBatch = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
	libretroCallbacks.inputPoll = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
	libretroCallbacks.inputState = cb;
}

void retro_init(void) {
	retro_core_init_log();
	LOG(LOG_DEBUG, "retro_init call\n");
	
	char* systemDirectory;
	libretroCallbacks.environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &systemDirectory);
	retro_core_create_neogeo(systemDirectory);
	
	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
	if (!libretroCallbacks.environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
		LOG(LOG_ERROR, "retro_init: RGB565 support is required!\n");
	}
	
//	bool no_content = true;
//	if (!libretroCallbacks.environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content)) {
//		LOG(LOG_ERROR, "retro_init: can't set no game required\n");
//	}
}

void retro_deinit(void) {

}

unsigned retro_api_version(void) {
	return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info *info) {
	info->library_name = "Neogeo";
	info->library_version = "0.1";
	info->valid_extensions = "zip";
	info->need_fullpath = true;
	info->block_extract = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
	info->timing.fps = 60;
	info->timing.sample_rate = 44100;
	info->geometry.base_width = 320;
	info->geometry.base_height = 224;
	info->geometry.max_width = 320;
	info->geometry.max_height = 224;
	info->geometry.aspect_ratio = 320/224;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
	LOG(LOG_DEBUG, "retro_set_controller_port_device plugging device %u into port %u.\n", device, port);
}


void retro_reset(void) {
	LOG(LOG_DEBUG, "retro_reset\n");
	neogeo_reset();
}

void retro_run(void) {
	static uint64_t frame_count = 0;
	LOG(LOG_DEBUG, "--------------------------- run %u ---------------------------\n", frame_count);
	libretroCallbacks.inputPoll();
	retro_core_poll_joypad_1();
	retro_core_poll_joypad_2();
	
	neogeo_runOneFrame();
//	retro_core_draw_mire(video.frameBuffer, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
	libretroCallbacks.audioBatch(audioBuffer, samplesThisFrame);
	libretroCallbacks.video(video.frameBuffer, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, FRAMEBUFFER_WIDTH * sizeof(uint16_t));
	frame_count++;
}

size_t retro_serialize_size(void) {
	return 0;
}

bool retro_serialize(void *data, size_t size) {
	return false;
}

bool retro_unserialize(const void *data, size_t size) {
	return false;
}

void retro_cheat_reset(void) {

}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {

}

bool retro_load_game(const struct retro_game_info *game) {
	bool is_hardware_ready = neogeo_is_system_ready();
	if (is_hardware_ready == false) {
		LOG(LOG_ERROR, "retro_load_game: system not ready\n");
		return false;
	}
	
	if (game == NULL) {
		LOG(LOG_DEBUG, "retro_load_game is NULL \n");
		neogeo_reset();
		return true;
	}
	LOG(LOG_INFO, "loading game from %s\n", game->path);
	bool cartridge_valid = cartridge_load_roms(game->path);
	if (cartridge_valid == false) {
		LOG(LOG_ERROR, "invalid game from %s\n", game->path);
		return false;
	}
	neogeo_reset();
	return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) {
    return false;
}

void retro_unload_game(void) {

}

unsigned retro_get_region(void) {
	return RETRO_REGION_NTSC;
}

void *retro_get_memory_data(unsigned id) {
	return NULL;
}

size_t retro_get_memory_size(unsigned id) {
	return 0;
}

