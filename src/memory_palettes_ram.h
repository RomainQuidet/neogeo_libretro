#ifndef memory_palette_ram_h
#define memory_palette_ram_h

#include "memory_region.h"

extern memory_region_t palettes_ram1;		// PALETTES RAM - https://wiki.neogeodev.org/index.php?title=Palette_RAM
extern memory_region_t palettes_ram2;

extern memory_region_t palettes_ram_mirror;

void palettes_rams_init(void);
void palettes_rams_reset(void);

#endif /* memory_palette_ram_h */
