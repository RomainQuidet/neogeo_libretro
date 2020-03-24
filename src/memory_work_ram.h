#ifndef memory_work_ram_h
#define memory_work_ram_h

#include "memory_region.h"

extern memory_region_t work_ram;			// WORK RAM - https://wiki.neogeodev.org/index.php?title=68k_user_RAM
extern memory_region_t work_ram_mirror;

void work_ram_init(void);

#endif /* memory_work_ram_h */
