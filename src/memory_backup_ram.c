#include "memory_mapping.h"
#include "memory_backup_ram.h"

#include <string.h>


memory_region_t backup_ram;			// BACKUP RAM - https://wiki.neogeodev.org/index.php?title=Backup_RAM
memory_region_t backup_ram_mirror;

static uint8_t backup_ram_read_byte(uint32_t offset) {
	return backup_ram.data[offset];
}

static uint16_t backup_ram_read_word(uint32_t offset) {
	return *((uint16_t *)(backup_ram.data + offset));
}

static uint32_t backup_ram_read_dword(uint32_t offset) {
	return *((uint32_t *)(backup_ram.data + offset));
}

static void backup_ram_write_byte(uint32_t offset, uint8_t data) {
	backup_ram.data[offset] = data;
}

static void backup_ram_write_word(uint32_t offset, uint16_t data) {
	*((uint16_t *)(backup_ram.data + offset)) = data;
}

static void backup_ram_write_dword(uint32_t offset, uint32_t data) {
	*((uint32_t *)(backup_ram.data + offset)) = data;
}

void backup_ram_init(void) {
	memset(&backup_ram, 0, sizeof(memory_region_t));
	memset(&backup_ram_mirror, 0, sizeof(memory_region_t));
	
	backup_ram.data = malloc(BACKUP_RAM_SIZE);
	backup_ram.size = BACKUP_RAM_SIZE;
	backup_ram.start_address = BACKUP_RAM_START;
	backup_ram.end_address = BACKUP_RAM_END;
	backup_ram.handlers.read_byte = &backup_ram_read_byte;
	backup_ram.handlers.read_word = &backup_ram_read_word;
	backup_ram.handlers.read_dword = &backup_ram_read_dword;
	backup_ram.handlers.write_byte = &backup_ram_write_byte;
	backup_ram.handlers.write_word = &backup_ram_write_word;
	backup_ram.handlers.write_dword = &backup_ram_write_dword;
	
	backup_ram_mirror.data = backup_ram.data;
	backup_ram_mirror.size = backup_ram.size;
	backup_ram_mirror.start_address = BACKUP_RAM_MIRROR_START;
	backup_ram_mirror.end_address = BACKUP_RAM_MIRROR_END;
	backup_ram_mirror.handlers = backup_ram.handlers;
}
