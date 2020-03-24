#include "endian.h"
#include "cartridge.h"
#include "common_tools.h"
#include "log.h"
#include "memory_mapping.h"
#include "rom_region.h"

#include "3rdParty/miniz/miniz.h"

#include <string.h>

// Cartridge ROMS - https://wiki.neogeodev.org/index.php?title=Cartridges

typedef struct cartridge {
	rom_region_t p_roms[4];		// 68K programs
	rom_region_t c_roms[8];		// Sprites tiles
	rom_region_t s_roms[2];		// Fix sprite tiles
	rom_region_t m1_rom;		// Z80 program
	rom_region_t v1_roms[4];	// Sound samples
	rom_region_t v2_roms[4];	// Sound samples
} cartridge_t;


cartridge_t plugged_cartridge;
memory_region_t p_rom_bank1;
memory_region_t p_rom_bank2;
memory_region_t serialized_c_roms;
memory_region_t m1_rom;

static void init_cartridge_p_rom(void);
static void init_cartridge_p_rom2(void);
static void init_cartridge_m1_rom(void);
static uint16_t cartridge_game_ngh(void);
static bool cartridge_p_rom_check(void);
static void cartridge_serialize_c_rom(void);

#pragma mark - Public

void cartridge_init() {
	memset(&plugged_cartridge.p_roms[0], 0, sizeof(memory_region_t));
	memset(&plugged_cartridge.p_roms[1], 0, sizeof(memory_region_t));
	init_cartridge_p_rom();
	init_cartridge_p_rom2();
	init_cartridge_m1_rom();
}

bool cartridge_load_roms(const char *path) {
	mz_zip_archive zip_archive;
	mz_zip_zero_struct(&zip_archive);
	mz_bool status = mz_zip_reader_init_file(&zip_archive, path, 0);
	if (!status) {
		LOG(LOG_ERROR, "cartridge_load_roms: can't open game file at %s - %s \n", path, mz_zip_get_error_string(zip_archive.m_last_error));
		return false;
	}
		
	mz_uint files_count = mz_zip_reader_get_num_files(&zip_archive);
	
	for (mz_uint file_index = 0; file_index < files_count; file_index++) {
		char file_name[128];
		mz_zip_reader_get_filename(&zip_archive, file_index, file_name, 128);
		
		void *p;
		size_t pSize;
		p = mz_zip_reader_extract_to_heap(&zip_archive, file_index, &pSize, MZ_ZIP_FLAG_IGNORE_PATH);
		if (!p) {
			LOG(LOG_ERROR, "cartridge_load_roms: can't extract game rom file %s\n", file_name);
			mz_zip_reader_end(&zip_archive);
			return false;
		}
		
		bool found = false;
		//P_ROM
		for (uint8_t i = 1; i <= 2; i++) {
			char element[4];
			sprintf(element, "p%d.", i);
			if (strcasestr(file_name, element) != NULL) {
				LOG(LOG_DEBUG, "cartridge_load_roms found P_ROM %d %s - loaded at %p\n", i, file_name, p);
				void *prom = p;
				size_t prom_size = pSize;
				if (prom_size > ROM_BANK1_SIZE) {
					LOG(LOG_ERROR, "cartridge_load_roms P_ROM is too big: splitting\n");
					// why MAME, why???
					if (i == 1)
					{
						uint32_t offset = ROM_BANK1_SIZE;
						if (prom_size == 2 * ROM_BANK1_SIZE) {
							prom_size = pSize - offset;
							prom = malloc(prom_size);
							memcpy(prom, p + offset, prom_size);
							
							void *prom_2 = malloc(ROM_BANK1_SIZE);
							memcpy(prom_2, p, ROM_BANK1_SIZE);
							byte_swap_p_rom_if_needed(prom_2, ROM_BANK1_SIZE);
							plugged_cartridge.p_roms[i].data = prom_2;
							plugged_cartridge.p_roms[i].size = ROM_BANK1_SIZE;
							free(p);
							byte_swap_p_rom_if_needed(prom, prom_size);
							plugged_cartridge.p_roms[i-1].data = prom;
							plugged_cartridge.p_roms[i-1].size = prom_size;
							found = true;
							break;
						}
						else {
							free(p);
							break;
						}
					}
					else
					{
						uint32_t offset = 0;
						uint8_t rom_offset = 0;
						while (offset < prom_size)
						{
							prom = malloc(ROM_BANK1_SIZE);
							memcpy(prom, p + offset, ROM_BANK1_SIZE);
							byte_swap_p_rom_if_needed(prom, ROM_BANK1_SIZE);
							plugged_cartridge.p_roms[i-1 + rom_offset].data = prom;
							plugged_cartridge.p_roms[i-1 + rom_offset].size = ROM_BANK1_SIZE;
							offset += ROM_BANK1_SIZE;
							rom_offset++;
						}
						found = true;
						break;
					}
				}
				else
				{
					byte_swap_p_rom_if_needed(prom, prom_size);
					plugged_cartridge.p_roms[i-1].data = prom;
					plugged_cartridge.p_roms[i-1].size = prom_size;
					found = true;
					break;
				}
			}
		}
		if (found == true) {
			continue;
		}
		
		//S_ROM
		for (uint8_t i = 1; i <= 2; i++) {
			char element[4];
			sprintf(element, "s%d.", i);
			if (strcasestr(file_name, element) != NULL) {
				LOG(LOG_DEBUG, "cartridge_load_roms found S_ROM %d %s\n", i, file_name);
				plugged_cartridge.s_roms[i-1].data = p;
				plugged_cartridge.s_roms[i-1].size = pSize;
				found = true;
				break;
			}
		}
		if (found == true) {
			continue;
		}
		
		//C_ROM
		for (uint8_t i = 1; i <= 8; i++) {
			char element[4];
			sprintf(element, "c%d.", i);
			if (strcasestr(file_name, element) != NULL) {
				LOG(LOG_DEBUG, "cartridge_load_roms found C_ROM %d %s %lld bytes\n", i, file_name, pSize);
				plugged_cartridge.c_roms[i-1].data = p;
				plugged_cartridge.c_roms[i-1].size = pSize;
				found = true;
				break;
			}
		}
		if (found == true) {
			continue;
		}
		
		// M ROM
		if (strcasestr(file_name, "m1.") != NULL) {
			LOG(LOG_DEBUG, "cartridge_load_roms found M_ROM 1 %s %lld bytes\n", file_name, pSize);
			plugged_cartridge.m1_rom.data = p;
			plugged_cartridge.m1_rom.size = pSize;
			continue;
		}
		
		//V1_ROM
		if (strcasestr(file_name, "v1.") != NULL) {
			LOG(LOG_DEBUG, "cartridge_load_roms found V1_ROM 1 %s %lld bytes\n", file_name, pSize);
			plugged_cartridge.v1_roms[0].data = p;
			plugged_cartridge.v1_roms[0].size = pSize;
			continue;
		}
		
		for (uint8_t i = 1; i <= 4; i++) {
			char element[5];
			sprintf(element, "v1%d.", i);
			if (strcasestr(file_name, element) != NULL) {
				LOG(LOG_DEBUG, "cartridge_load_roms found V1_ROM %d %s %lld bytes\n", i, file_name, pSize);
				plugged_cartridge.v1_roms[i-1].data = p;
				plugged_cartridge.v1_roms[i-1].size = pSize;
				found = true;
				break;
			}
		}
		if (found == true) {
			continue;
		}
		
		//V2_ROM
		if (strcasestr(file_name, "v2.") != NULL) {
			LOG(LOG_DEBUG, "cartridge_load_roms found V2_ROM 1 %s %lld bytes\n", file_name, pSize);
			plugged_cartridge.v2_roms[0].data = p;
			plugged_cartridge.v2_roms[0].size = pSize;
			continue;
		}
		
		for (uint8_t i = 1; i <= 4; i++) {
			char element[5];
			sprintf(element, "v2%d.", i);
			if (strcasestr(file_name, element) != NULL) {
				LOG(LOG_DEBUG, "cartridge_load_roms found V2_ROM %d %s %lld bytes\n", i, file_name, pSize);
				plugged_cartridge.v2_roms[i-1].data = p;
				plugged_cartridge.v2_roms[i-1].size = pSize;
				found = true;
				break;
			}
		}
		if (found == true) {
			continue;
		}
		
		// Nothing catched ?
		mz_free(p);
		LOG(LOG_DEBUG, "cartridge_load_roms: unused file %s\n", file_name);
	}
	
	if (plugged_cartridge.p_roms[0].data == NULL
		|| plugged_cartridge.s_roms[0].data == NULL
		|| plugged_cartridge.c_roms[0].data == NULL
		|| plugged_cartridge.c_roms[1].data == NULL) {
		LOG(LOG_DEBUG, "cartridge_load_roms: seems that minimum roms are not found\n");
		mz_zip_reader_end(&zip_archive);
		return false;
	}
	
	if (cartridge_p_rom_check() == false) {
		LOG(LOG_DEBUG, "cartridge_load_roms: P ROM header is missing NEO-GEO ref\n");
		mz_zip_reader_end(&zip_archive);
		return false;
	}
	
	mz_zip_reader_end(&zip_archive);
	
	// Post treatment for internal architecture
	
	memset(p_rom_bank1.data, 0, ROM_BANK1_SIZE);
	uint32_t p1Offset = 0;
	for (int i = 0; i < 4; i++) {
		if (plugged_cartridge.p_roms[i].data != NULL
			&& p1Offset < ROM_BANK1_SIZE) {
			memcpy(p_rom_bank1.data + p1Offset, plugged_cartridge.p_roms[i].data, plugged_cartridge.p_roms[i].size);
			p1Offset += plugged_cartridge.p_roms[i].size;
		}
	}
	memset(p_rom_bank2.data, 0, ROM_BANK1_SIZE);
	memcpy(p_rom_bank2.data, plugged_cartridge.p_roms[1].data, plugged_cartridge.p_roms[1].size);
	
	m1_rom.data = plugged_cartridge.m1_rom.data;
	m1_rom.size = plugged_cartridge.m1_rom.size;
	m1_rom.end_address = (uint32_t)m1_rom.size - 1;
	
	uint16_t ngh = cartridge_game_ngh();
	LOG(LOG_INFO, "Cartridge NGH: %04d\n", ngh);
	
	cartridge_serialize_c_rom();
	
	return true;
}

void cartridge_unload(void) {
	for (uint8_t i = 0; i < 2; i++) {
		if (plugged_cartridge.p_roms[i].data != NULL) {
			mz_free(plugged_cartridge.p_roms[i].data);
		}
		if (plugged_cartridge.s_roms[i].data != NULL) {
			mz_free(plugged_cartridge.s_roms[i].data);
			plugged_cartridge.s_roms[i].size = 0;
		}
	}
	
	memset(p_rom_bank1.data, 0, ROM_BANK1_SIZE);
	memset(p_rom_bank2.data, 0, ROM_BANK1_SIZE);

	for (uint8_t i = 0; i < 8; i++) {
		if (plugged_cartridge.c_roms[i].data != NULL) {
			mz_free(plugged_cartridge.c_roms[i].data);
			plugged_cartridge.c_roms[i].size = 0;
		}
	}
	
	free(serialized_c_roms.data);
}

bool cartridge_plugged_in() {
	return plugged_cartridge.c_roms[0].data != NULL;
}

rom_region_t * cartridge_get_first_fix_rom() {
	return &plugged_cartridge.s_roms[0];
}

rom_region_t cartridge_create_pcm_rom(int index) {
	rom_region_t *source = plugged_cartridge.v1_roms;
	if (index > 0) {
		source = plugged_cartridge.v2_roms;
	}
	rom_region_t result;
	result.data = NULL;
	result.size = 0;
	for (int i = 0; i < 4; i++) {
		if (source[i].data != NULL) {
			result.size += source[i].size;
		}
	}
	if (result.size > 0) {
		result.data = malloc(result.size);
		size_t offset = 0;
		for (int i = 0; i < 4; i++) {
			if (source[i].data != NULL) {
				memcpy(result.data + offset, source[i].data, source[i].size);
				offset += source[i].size;
			}
		}
	}
	return result;
}

#pragma mark - Private
#pragma mark P_ROM1

static uint8_t cartridge_p_rom_read_byte(uint32_t offset) {
//	LOG(LOG_DEBUG, "cartridge_p_rom_read_byte 0x%08X\n", offset);
	return p_rom_bank1.data[offset];
}

static uint16_t cartridge_p_rom_read_word(uint32_t offset) {
//	LOG(LOG_DEBUG, "cartridge_p_rom_read_word 0x%08X\n", offset);
	return BIG_ENDIAN_WORD(*((uint16_t *)(p_rom_bank1.data + offset)));
}

static uint32_t cartridge_p_rom_read_dword(uint32_t offset) {
//	LOG(LOG_DEBUG, "cartridge_p_rom_read_dword 0x%08X\n", offset);
	return BIG_ENDIAN_DWORD(*((uint32_t *)(p_rom_bank1.data + offset)));
}

static void init_cartridge_p_rom() {
	p_rom_bank1.data = malloc(ROM_BANK1_SIZE);
	memset(p_rom_bank1.data, 0, ROM_BANK1_SIZE);
	p_rom_bank1.start_address = ROM_BANK1_START;
	p_rom_bank1.end_address = ROM_BANK1_END;
	p_rom_bank1.size = ROM_BANK1_SIZE;
	p_rom_bank1.handlers.read_byte = &cartridge_p_rom_read_byte;
	p_rom_bank1.handlers.read_word = &cartridge_p_rom_read_word;
	p_rom_bank1.handlers.read_dword = &cartridge_p_rom_read_dword;
}

#pragma mark P_ROM2

static uint8_t cartridge_p_rom2_read_byte(uint32_t offset) {
//	LOG(LOG_DEBUG, "cartridge_p_rom2_read_byte 0x%08X\n", offset);
	return p_rom_bank2.data[offset];
}

static uint16_t cartridge_p_rom2_read_word(uint32_t offset) {
//	LOG(LOG_DEBUG, "cartridge_p_rom2_read_word 0x%08X\n", offset);
	return BIG_ENDIAN_WORD(*((uint16_t *)(p_rom_bank2.data + offset)));
}

static uint32_t cartridge_p_rom2_read_dword(uint32_t offset) {
//	LOG(LOG_DEBUG, "cartridge_p_rom2_read_dword 0x%08X\n", offset);
	return BIG_ENDIAN_DWORD(*((uint32_t *)(p_rom_bank2.data + offset)));
}

static void cartridge_p_rom2_write_byte(uint32_t offset, uint8_t data) {
//	LOG(LOG_DEBUG, "cartridge_p_rom2_write_byte at 0x%08X - 0x%02X\n", offset, data);
	switch (data) {
		case 0:
		case 1:
		case 2:
		case 3:
			LOG(LOG_DEBUG, "cartridge_p_rom2_write_byte bank switch #%u\n", data);
			memset(p_rom_bank2.data, 0, ROM_BANK1_SIZE);
			memcpy(p_rom_bank2.data, plugged_cartridge.p_roms[data+1].data, plugged_cartridge.p_roms[data+1].size);
			break;
		default:
			LOG(LOG_DEBUG, "cartridge_p_rom2_write_byte unknown bank switch\n");
			break;
	}
}

static void cartridge_p_rom2_write_word(uint32_t offset, uint16_t data) {
//	LOG(LOG_DEBUG, "cartridge_p_rom2_write_word at 0x%08X - 0x%04X\n", offset, data);
	cartridge_p_rom2_write_byte(offset, data);
}

static void cartridge_p_rom2_write_dword(uint32_t offset, uint32_t data) {
//	LOG(LOG_DEBUG, "cartridge_p_rom2_write_dword at 0x%08X - 0x%08X\n", offset, data);
	cartridge_p_rom2_write_word(offset, data);
}

static void init_cartridge_p_rom2() {
	p_rom_bank2.data = malloc(ROM_BANK1_SIZE);
	memset(p_rom_bank2.data, 0, ROM_BANK1_SIZE);
	p_rom_bank2.start_address = ROM_BANK2_START;
	p_rom_bank2.end_address = ROM_BANK2_END;
	p_rom_bank2.size = ROM_BANK1_SIZE;
	p_rom_bank2.handlers.read_byte = &cartridge_p_rom2_read_byte;
	p_rom_bank2.handlers.read_word = &cartridge_p_rom2_read_word;
	p_rom_bank2.handlers.read_dword = &cartridge_p_rom2_read_dword;
	// write for bankswitching - https://wiki.neogeodev.org/index.php?title=Bankswitching
	p_rom_bank2.handlers.write_byte = &cartridge_p_rom2_write_byte;
	p_rom_bank2.handlers.write_word = &cartridge_p_rom2_write_word;
	p_rom_bank2.handlers.write_dword = &cartridge_p_rom2_write_dword;
}

#pragma mark M1_ROM

static uint8_t cartridge_m1_rom_read_byte(uint32_t offset) {
//	LOG(LOG_DEBUG, "cartridge_m1_rom_read_byte 0x%08X\n", offset);
	return m1_rom.data[offset];
}

static uint16_t cartridge_m1_rom_read_word(uint32_t offset) {
	//	LOG(LOG_DEBUG, "cartridge_p_rom_read_word 0x%08X\n", offset);
	return BIG_ENDIAN_WORD(*((uint16_t *)(m1_rom.data + offset)));
}

static uint32_t cartridge_m1_rom_read_dword(uint32_t offset) {
	//	LOG(LOG_DEBUG, "cartridge_p_rom_read_dword 0x%08X\n", offset);
	return BIG_ENDIAN_DWORD(*((uint32_t *)(m1_rom.data + offset)));
}

static void init_cartridge_m1_rom() {
	m1_rom.start_address = 0;
	m1_rom.end_address = 0;
	m1_rom.size = 0;
	m1_rom.handlers.read_byte = &cartridge_m1_rom_read_byte;
	m1_rom.handlers.read_word = &cartridge_m1_rom_read_word;
	m1_rom.handlers.read_dword = &cartridge_m1_rom_read_dword;
}

#pragma mark Util

static uint16_t cartridge_game_ngh() {
	uint16_t bcd = p_rom_bank1.handlers.read_word(0x108);
	LOG(LOG_DEBUG, "cartridge_game_ngh 0x%04X\n", bcd);
	
	uint16_t ngh = 0;
	ngh += ((bcd >> 12) & 0x000F) * 1000;
	ngh += ((bcd >> 8) & 0x000F) * 100;
	ngh += ((bcd >> 4) & 0x000F) * 10;
	ngh += bcd & 0x000F;
	
	return ngh;
}

static bool cartridge_p_rom_check() {
	char *p = (char *)plugged_cartridge.p_roms[0].data;
	if (p == NULL) {
		return false;
	}
	p += 0x100;
	char ref[8] = "NEO-GEO";
	for (uint8_t i = 0; i < 7; ++i) {
		char c = p[i];
		if (ref[i] != c) {
			return false;
		}
	}
	
	return true;
}

/*
 *	Prepare all sprites to be easily displayed on framebuffer
 *	Unit data will be half byte pixel color index
 *	Each scanline is 8 bytes
 */

static const uint8_t ROM_TILE_BLOCK_BYTES = 16;		// 16 bytes per block per rom ( x 4 blocks x 2 ROMs = 128 bytes per tile)

static void cartridge_serialize_c_rom() {
	size_t characters_ram_size = 0;
	uint8_t rom_pairs_count = 0;
	for (uint8_t i = 0; i < 8; i++) {
		if (plugged_cartridge.c_roms[i].data != NULL) {
			characters_ram_size += plugged_cartridge.c_roms[i].size;
			rom_pairs_count++;
		}
	}
	
	rom_pairs_count /= 2;
	
	if (serialized_c_roms.data != NULL) {
		free(serialized_c_roms.data);
	}
	
	serialized_c_roms.data = malloc(characters_ram_size);
	serialized_c_roms.size = characters_ram_size;
	LOG(LOG_DEBUG, "cartridge_serialize_c_rom allocating %lld MB at %p\n", characters_ram_size / (1024*1024), serialized_c_roms.data);
	
	uint8_t *serialized_data_p = serialized_c_roms.data;
	
	for (uint8_t pair = 0; pair < rom_pairs_count; ++pair) {
		LOG(LOG_DEBUG, "cartridge_serialize_c_rom serializing C ROM pair %u - %u\n", pair * 2 + 1, pair * 2 + 2);
		uint8_t *odd_data = plugged_cartridge.c_roms[pair * 2].data;
		uint8_t *even_data = plugged_cartridge.c_roms[pair * 2 + 1].data;
		
		size_t roms_size = plugged_cartridge.c_roms[pair * 2].size;
		if (roms_size != plugged_cartridge.c_roms[pair * 2 + 1].size) {
			LOG(LOG_ERROR, "cartridge_serialize_c_rom %d and %d C ROMS are not even\n",  pair * 2 + 1, pair * 2 + 2);
		}
		
		size_t tiles_to_serialize = roms_size * 2 / CHARACTER_TILE_BYTES;
		LOG(LOG_DEBUG, "cartridge_serialize_c_rom will serializing %u tiles\n", tiles_to_serialize);
		
		for (size_t tile_index = 0; tile_index < tiles_to_serialize; ++tile_index) {
			uint8_t *odd_tile_base = odd_data + (tile_index * CHARACTER_TILE_BYTES/2);
			uint8_t *even_tile_base = even_data + (tile_index * CHARACTER_TILE_BYTES/2);
			
			uint8_t left_block = 3;
			uint8_t right_block = 1;
			for (uint8_t vertical_block_pass = 0; vertical_block_pass < 2; ++vertical_block_pass) {
				// blocks 3/1 then 4/2
				left_block += vertical_block_pass;
				right_block += vertical_block_pass;
				for (uint8_t scanline = 0; scanline < 8; scanline++) {
					// 8 scanlines per block
					for (uint8_t horizontal_block_pass = 0; horizontal_block_pass < 2; ++horizontal_block_pass) {
						// draw left then right block
						uint8_t block_index = horizontal_block_pass == 0 ? left_block - 1 : right_block - 1;
						
						uint8_t *odd_left_block_base = odd_tile_base + (block_index * ROM_TILE_BLOCK_BYTES) + (scanline * 2);
						uint8_t *even_left_block_base = even_tile_base + (block_index * ROM_TILE_BLOCK_BYTES) + (scanline * 2);
						
						uint8_t plane_0 = odd_left_block_base[0];
						uint8_t plane_1 = odd_left_block_base[1];
						uint8_t plane_2 = even_left_block_base[0];
						uint8_t plane_3 = even_left_block_base[1];
						
						for (uint8_t row = 0; row < 8; row++) {
							uint8_t pixel_color_index = 0;
							pixel_color_index |= (plane_0 >> row) & 0x01;
							pixel_color_index |= ((plane_1 >> row) & 0x01) << 1;
							pixel_color_index |= ((plane_2 >> row) & 0x01) << 2;
							pixel_color_index |= ((plane_3 >> row) & 0x01) << 3;
							if (row & 1) {
								*serialized_data_p |= pixel_color_index << 4;
								++serialized_data_p;
							}
							else {
								*serialized_data_p |= pixel_color_index;
							}
						}
					}
				}
			}
		}
	}
	
	uint64_t bytes = serialized_data_p - serialized_c_roms.data + 1;
	uint64_t tiles_count = bytes / CHARACTER_TILE_BYTES;
	LOG(LOG_DEBUG, "cartridge_serialize_c_rom parsed %u tiles\n", tiles_count);
}
