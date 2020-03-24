#include "endian.h"
#include "memory_mapping.h"
#include "memory_work_ram.h"

#include <string.h>


memory_region_t work_ram;			// WORK RAM - https://wiki.neogeodev.org/index.php?title=68k_user_RAM
memory_region_t work_ram_mirror;

static uint8_t work_ram_read_byte(uint32_t offset) {
	return work_ram.data[offset];
}

static uint16_t work_ram_read_word(uint32_t offset) {
	return BIG_ENDIAN_WORD(*((uint16_t *)(work_ram.data + offset)));
}

static uint32_t work_ram_read_dword(uint32_t offset) {
	return BIG_ENDIAN_DWORD(*((uint32_t *)(work_ram.data + offset)));
}

static void work_ram_write_byte(uint32_t offset, uint8_t data) {
	work_ram.data[offset] = data;
}

static void work_ram_write_word(uint32_t offset, uint16_t data) {
	*((uint16_t *)(work_ram.data + offset)) = BIG_ENDIAN_WORD(data);
}

static void work_ram_write_dword(uint32_t offset, uint32_t data) {
	*((uint32_t *)(work_ram.data + offset)) = BIG_ENDIAN_DWORD(data);
}

void work_ram_init(void) {
	memset(&work_ram, 0, sizeof(memory_region_t));
	memset(&work_ram_mirror, 0, sizeof(memory_region_t));
	
	work_ram.data = malloc(WORK_RAM_SIZE);
	memset(work_ram.data, 0, WORK_RAM_SIZE);
	work_ram.size = WORK_RAM_SIZE;
	work_ram.start_address = WORK_RAM_START;
	work_ram.end_address = WORK_RAM_END;
	work_ram.handlers.read_byte = &work_ram_read_byte;
	work_ram.handlers.read_word = &work_ram_read_word;
	work_ram.handlers.read_dword = &work_ram_read_dword;
	work_ram.handlers.write_byte = &work_ram_write_byte;
	work_ram.handlers.write_word = &work_ram_write_word;
	work_ram.handlers.write_dword = &work_ram_write_dword;
	
	work_ram_mirror.data = work_ram.data;
	work_ram_mirror.size = work_ram.size;
	work_ram_mirror.start_address = WORK_RAM_MIRROR_START;
	work_ram_mirror.end_address = WORK_RAM_MIRROR_END;
	work_ram_mirror.handlers = work_ram.handlers;
}
