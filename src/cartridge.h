#ifndef cartridge_h
#define cartridge_h

#include "memory_region.h"
#include "rom_region.h"

static const uint8_t CHARACTER_TILE_BYTES = 128;

extern memory_region_t p_rom_bank1;		// Init Vector table - https://wiki.neogeodev.org/index.php?title=68k_vector_table
										// + program ROM - https://wiki.neogeodev.org/index.php?title=P_ROM
extern memory_region_t p_rom_bank2;
extern memory_region_t serialized_c_roms;	// serialized sprites from C ROMs ready for display, half byte per pixel

extern memory_region_t m1_rom;	// Music ROM - https://wiki.neogeodev.org/index.php?title=M1_ROM

void cartridge_init(void);
bool cartridge_load_roms(const char *path);
void cartridge_unload(void);
bool cartridge_plugged_in(void);

rom_region_t * cartridge_get_first_fix_rom(void);
rom_region_t cartridge_create_pcm_rom(int index);

#endif /* cartridge_h */
