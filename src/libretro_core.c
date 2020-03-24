#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "aux_inputs.h"
#include "joypads.h"
#include "libretro_core.h"
#include "log.h"
#include "neogeo.h"
#include "3rdParty/miniz/miniz.h"

libretro_callbacks_t libretroCallbacks;

static const uint8_t joypads_map[] = {
	RETRO_DEVICE_ID_JOYPAD_LEFT, JOYPAD_PORT_MASK_LEFT,
	RETRO_DEVICE_ID_JOYPAD_UP, JOYPAD_PORT_MASK_UP,
	RETRO_DEVICE_ID_JOYPAD_DOWN, JOYPAD_PORT_MASK_DOWN,
	RETRO_DEVICE_ID_JOYPAD_RIGHT, JOYPAD_PORT_MASK_RIGHT,
	RETRO_DEVICE_ID_JOYPAD_B, JOYPAD_PORT_MASK_A,
	RETRO_DEVICE_ID_JOYPAD_A, JOYPAD_PORT_MASK_B,
	RETRO_DEVICE_ID_JOYPAD_Y, JOYPAD_PORT_MASK_C,
	RETRO_DEVICE_ID_JOYPAD_X, JOYPAD_PORT_MASK_D
};

#pragma mark - private defines

bool load_system_roms(const char *path);

#pragma mark - Public

void retro_core_init_log(void) {
	if (libretroCallbacks.environment != NULL) {
		struct retro_log_callback log;
		if (libretroCallbacks.environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
			libretroCallbacks.log = log.log;
		else
			libretroCallbacks.log = NULL;
	}
}

void retro_core_create_neogeo(const char *systemDirectory) {
	neogeo_initialize();
	
	char *full_path;
	full_path = malloc(strlen(systemDirectory) + strlen("/neogeo/") + strlen("neogeo.zip") + 2);
	sprintf(full_path, "%s/neogeo/neogeo.zip", systemDirectory);
	load_system_roms(full_path);
	free(full_path);
}

void retro_core_poll_joypad_1(void) {
	joypad_port1 = JOYPAD_INIT;
	for (uint8_t i = 0; i < sizeof(joypads_map); i += 2) {
		unsigned retro_button_id = joypads_map[i];
		if (libretroCallbacks.inputState(0, RETRO_DEVICE_JOYPAD, 0, retro_button_id)) {
			uint8_t mask = joypads_map[i+1];
			joypad_port1 &= ~mask;
		}
	}
	int16_t pressed = libretroCallbacks.inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	aux_input_start_player(Player1, pressed);
	pressed = libretroCallbacks.inputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);
	aux_input_select_player(Player1, pressed);
}

void retro_core_poll_joypad_2(void) {
	joypad_port2 = JOYPAD_INIT;
	for (uint8_t i = 0; i < sizeof(joypads_map); i += 2) {
		unsigned retro_button_id = joypads_map[i];
		if (libretroCallbacks.inputState(1, RETRO_DEVICE_JOYPAD, 0, retro_button_id)) {
			uint8_t mask = joypads_map[i+1];
			joypad_port2 &= ~mask;
		}
	}
	int16_t pressed = libretroCallbacks.inputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	aux_input_start_player(Player2, pressed);
	pressed = libretroCallbacks.inputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);
	aux_input_select_player(Player2, pressed);
}

#pragma mark - Private

bool load_system_roms(const char *path) {
	mz_zip_archive zip_archive;
	mz_zip_zero_struct(&zip_archive);
	mz_bool status = mz_zip_reader_init_file(&zip_archive, path, 0);
	if (!status) {
		LOG(LOG_ERROR, "retro core: can't open bios file at %s - %s \n", path, mz_zip_get_error_string(zip_archive.m_last_error));
		return false;
	}
	
	// Y Zoom ROM
	int file_index = mz_zip_reader_locate_file(&zip_archive, "000-lo.lo", NULL, MZ_ZIP_FLAG_IGNORE_PATH);
	if (file_index == -1) {
		LOG(LOG_ERROR, "retro core: can't locate YZoom file %s\n", "000-lo.lo");
		return false;
	}
	
	void *p;
	size_t pSize;
	p = mz_zip_reader_extract_to_heap(&zip_archive, file_index, &pSize, MZ_ZIP_FLAG_IGNORE_PATH);
	if (!p) {
		LOG(LOG_ERROR, "retro core: can't extract YZoom file %s\n", "000-lo.lo");
		return false;
	}
	
	rom_region_t y_zoom_rom;
	y_zoom_rom.data = p;
	y_zoom_rom.size = pSize;
	bool neo_res = neogeo_set_system_Y_zoom_ROM(y_zoom_rom);
	if (!neo_res) {
		LOG(LOG_ERROR, "retro core: invalid YZoom file %s\n", "000-lo.lo");
		return false;
	}
	
	LOG(LOG_INFO, "retro core: found 000-lo.lo data %lld bytes\n", pSize);
	
	// MVS SFIX ROM
	char * sfix_rom_name = "sfix.sfx";
	file_index = mz_zip_reader_locate_file(&zip_archive, sfix_rom_name, NULL, MZ_ZIP_FLAG_IGNORE_PATH);
	if (file_index == -1) {
		sfix_rom_name = "sfix.sfix";
		file_index = mz_zip_reader_locate_file(&zip_archive, sfix_rom_name, NULL, MZ_ZIP_FLAG_IGNORE_PATH);
		if (file_index == -1) {
			LOG(LOG_ERROR, "retro core: can't locate MVS SFIX RROM files %s or %s\n", "sfix.sfx", "sfix.sfix");
			return -1;
		}
	}
	
	p = mz_zip_reader_extract_to_heap(&zip_archive, file_index, &pSize, MZ_ZIP_FLAG_IGNORE_PATH);
	if (!p) {
		LOG(LOG_ERROR, "retro core: can't extract sfix file %s\n", sfix_rom_name);
		return false;
	}
	
	rom_region_t sfix_rom;
	sfix_rom.data = p;
	sfix_rom.size = pSize;
	neo_res = neogeo_set_system_fix_ROM(sfix_rom);
	
	// SYSTEM ROM
	char *system_rom_name = "neo-epo.bin";//"asia-s3.rom";"uni-bios.rom";"diagnostic.bin";"neo-epo.bin";
	file_index = mz_zip_reader_locate_file(&zip_archive, system_rom_name, NULL, MZ_ZIP_FLAG_IGNORE_PATH);
	if (file_index == -1) {
		LOG(LOG_ERROR, "retro core: can't locate system rom file %s\n", system_rom_name);
		return false;
	}
	
	p = mz_zip_reader_extract_to_heap(&zip_archive, file_index, &pSize, MZ_ZIP_FLAG_IGNORE_PATH);
	if (!p) {
		LOG(LOG_ERROR, "retro core: can't extract system rom file %s\n", system_rom_name);
		return false;
	}
	
	rom_region_t system_rom;
	system_rom.data = p;
	system_rom.size = pSize;
	neo_res = neogeo_set_system_ROM(system_rom);

	mz_zip_reader_end(&zip_archive);
	
	return neo_res;
}

#pragma mark - Debug

void retro_core_draw_mire(const uint16_t *frameBuffer, uint16_t width, uint16_t height) {
	// test to draw a mire on screen, format is RGB565
	uint16_t white = 0xFFFF;
	uint16_t black = 0x0000;
	uint16_t red = 0xF800;
	uint16_t green = 0x07E0;
	uint16_t blue = 0x001F;
	
	if (width % 5 != 0) {
		LOG(LOG_DEBUG, "retro_core_draw_mire width must be % 5 !\n");
		return;
	}
	
	uint16_t same_pixels = width / 5;
	uint16_t *pixelsBuffer = (uint16_t *)frameBuffer;
	for (uint16_t scanline = 0; scanline < height; scanline++) {
		for (uint8_t i = 0; i < same_pixels; i++) {
			*pixelsBuffer = white;
			pixelsBuffer++;
		}
		for (uint8_t i = 0; i < same_pixels; i++) {
			*pixelsBuffer = black;
			pixelsBuffer++;
		}
		for (uint8_t i = 0; i < same_pixels; i++) {
			*pixelsBuffer = red;
			pixelsBuffer++;
		}
		for (uint8_t i = 0; i < same_pixels; i++) {
			*pixelsBuffer = green;
			pixelsBuffer++;
		}
		for (uint8_t i = 0; i < same_pixels; i++) {
			*pixelsBuffer = blue;
			pixelsBuffer++;
		}
	}
}
