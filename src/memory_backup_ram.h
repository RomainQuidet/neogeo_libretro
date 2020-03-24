#ifndef memory_backup_ram_h
#define memory_backup_ram_h

#include "memory_region.h"

extern memory_region_t backup_ram;			// BACKUP RAM - https://wiki.neogeodev.org/index.php?title=Backup_RAM
extern memory_region_t backup_ram_mirror;

void backup_ram_init(void);

#endif /* memory_backup_ram_h */
