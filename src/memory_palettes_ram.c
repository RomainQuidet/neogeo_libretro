#include "memory_mapping.h"
#include "memory_palettes_ram.h"
#include "log.h"
#include "neogeo.h"
#include "video.h"

#include <string.h>


memory_region_t palettes_ram1;
memory_region_t palettes_ram2;

memory_region_t palettes_ram_mirror;

#pragma mark - Palettes access

static uint8_t palettes_ram_read_byte(uint32_t offset) {
	return current_palette_ram->data[offset];
}

static uint16_t palettes_ram_read_word(uint32_t offset) {
	return *((uint16_t *)(current_palette_ram->data + offset));
}

static uint32_t palettes_ram_read_dword(uint32_t offset) {
	uint16_t * word_p = (uint16_t *)(current_palette_ram->data + offset);
	return (*(word_p) << 16) + *(word_p + 1);
}

static void palettes_ram_write_byte(uint32_t offset, uint8_t data) {
	current_palette_ram->data[offset] = data;
	current_palette_ram->data[offset+1] = data;
//	LOG(LOG_DEBUG, "palettes_ram_write_byte at offset 0x%08X - 0x%04X\n", offset, data);
	video_convert_current_palette_color(offset/2);
}

static void palettes_ram_write_word(uint32_t offset, uint16_t data) {
	*((uint16_t *)(current_palette_ram->data + offset)) = data;
//	LOG(LOG_DEBUG, "palettes_ram_write_word at offset 0x%08X - 0x%04X\n", offset, data);
	video_convert_current_palette_color(offset/2);
}

static void palettes_ram_write_dword(uint32_t offset, uint32_t data) {
	uint16_t * word_p = (uint16_t *)(current_palette_ram->data + offset);
	*(word_p) = (uint16_t)(data >> 16);
	*(word_p + 1) = (uint16_t)data;
//	LOG(LOG_DEBUG, "palettes_ram_write_dword at offset 0x%08X - 0x%08X\n", offset, data);
	video_convert_current_palette_color(offset/2);
	video_convert_current_palette_color(offset/2 + 1);
}

#pragma mark - Palettes mirror access

static uint8_t palettes_mirror_ram_read_byte(uint32_t offset) {
	LOG(LOG_DEBUG, "palettes_mirror_ram_read_byte at offset 0x%08X\n", offset);
	while (offset >= PALETTES_RAM_SIZE) {
		offset -= PALETTES_RAM_SIZE;
	}
	return current_palette_ram->data[offset];
}

static uint16_t palettes_mirror_ram_read_word(uint32_t offset) {
	LOG(LOG_DEBUG, "palettes_mirror_ram_read_word at offset 0x%08X\n", offset);
	while (offset >= PALETTES_RAM_SIZE) {
		offset -= PALETTES_RAM_SIZE;
	}
	return *((uint16_t *)(current_palette_ram->data + offset));
}

static uint32_t palettes_mirror_ram_read_dword(uint32_t offset) {
	LOG(LOG_DEBUG, "palettes_mirror_ram_read_dword at offset 0x%08X\n", offset);
	while (offset >= PALETTES_RAM_SIZE) {
		offset -= PALETTES_RAM_SIZE;
	}
	uint16_t * word_p = (uint16_t *)(current_palette_ram->data + offset);
	return (*(word_p) << 16) + *(word_p + 1);
}

static void palettes_mirror_ram_write_byte(uint32_t offset, uint8_t data) {
//	current_palette_ram->data[offset] = data;
//	current_palette_ram->data[offset+1] = data;
	LOG(LOG_DEBUG, "palettes_mirror_ram_write_byte at offset 0x%08X - 0x%04X\n", offset, data);
//	video_convert_current_palette_color(offset/2);
}

static void palettes_mirror_ram_write_word(uint32_t offset, uint16_t data) {
//	*((uint16_t *)(current_palette_ram->data + offset)) = data;
	LOG(LOG_DEBUG, "palettes_mirror_ram_write_word at offset 0x%08X - 0x%04X\n", offset, data);
//	video_convert_current_palette_color(offset/2);
}

static void palettes_mirror_ram_write_dword(uint32_t offset, uint32_t data) {
//	uint16_t * word_p = (uint16_t *)(current_palette_ram->data + offset);
//	*(word_p) = (uint16_t)(data >> 16);
//	*(word_p + 1) = (uint16_t)data;
	LOG(LOG_DEBUG, "palettes_mirror_ram_write_dword at offset 0x%08X - 0x%08X\n", offset, data);
//	video_convert_current_palette_color(offset/2);
//	video_convert_current_palette_color(offset/2 + 1);
}


#pragma mark palette RAMs

void palettes_rams_init(void) {
	memset(&palettes_ram1, 0, sizeof(memory_region_t));
	memset(&palettes_ram2, 0, sizeof(memory_region_t));
	
	memory_region_access_handlers_t memory_palette_ram_handlers = {
		&palettes_ram_read_byte,
		&palettes_ram_read_word,
		&palettes_ram_read_dword,
		&palettes_ram_write_byte,
		&palettes_ram_write_word,
		&palettes_ram_write_dword
	};
	
	palettes_ram1.data = malloc(PALETTES_RAM_SIZE);
	palettes_ram1.size = PALETTES_RAM_SIZE;
	palettes_ram1.start_address = PALETTES_RAM_START;
	palettes_ram1.end_address = PALETTES_RAM_END;
	palettes_ram1.handlers = memory_palette_ram_handlers;
	
	palettes_ram2.data = malloc(PALETTES_RAM_SIZE);
	palettes_ram2.size = PALETTES_RAM_SIZE;
	palettes_ram2.start_address = PALETTES_RAM_START;
	palettes_ram2.end_address = PALETTES_RAM_END;
	palettes_ram2.handlers = memory_palette_ram_handlers;
	
	palettes_ram_mirror.start_address = PALETTES_RAM_MIRROR_START;
	palettes_ram_mirror.end_address = PALETTES_RAM_MIRROR_END;
	palettes_ram_mirror.handlers.read_byte = &palettes_mirror_ram_read_byte;
	palettes_ram_mirror.handlers.read_word = &palettes_mirror_ram_read_word;
	palettes_ram_mirror.handlers.read_dword = &palettes_mirror_ram_read_dword;
	palettes_ram_mirror.handlers.write_byte = &palettes_mirror_ram_write_byte;
	palettes_ram_mirror.handlers.write_word = &palettes_mirror_ram_write_word;
	palettes_ram_mirror.handlers.write_dword = &palettes_mirror_ram_write_dword;
}

void palettes_rams_reset(void) {
	memset(palettes_ram1.data, 0, PALETTES_RAM_SIZE);
	memset(palettes_ram2.data, 0, PALETTES_RAM_SIZE);
}
