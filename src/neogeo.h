#ifndef neogeo_h
#define neogeo_h

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include "memory_region.h"
#include "rom_region.h"

typedef enum cpu_68k_irq {
	VBlank = 0x04,
	Timer = 0x02,
	Reset = 0x01
} cpu_68k_irq_m;

#pragma mark - Components

extern memory_region_t *current_palette_ram;
void neogeo_use_palette_bank_1(void);
void neogeo_use_palette_bank_2(void);

extern rom_region_t system_y_zoom_rom;

extern rom_region_t *current_fix_rom;
void neogeo_use_board_fix_rom(void);
void neogeo_use_cartridge_fix_rom(void);

void neogeo_use_board_p_rom(void);
void neogeo_use_cartridge_p_rom(void);

#pragma mark - Lifecycle

void neogeo_initialize(void);
void neogeo_deinitialize(void);

void neogeo_reset(void);
void neogeo_runOneFrame(void);

#pragma mark - System ROM

bool neogeo_set_system_Y_zoom_ROM(rom_region_t rom);
bool neogeo_set_system_fix_ROM(rom_region_t rom);
bool neogeo_set_system_ROM(rom_region_t rom);

bool neogeo_is_system_ready(void);

#pragma mark - 68K CPU bus access

const memory_region_t* cpu_68k_memory_region_for_address(uint32_t address);
void cpu_68k_set_interrupt(cpu_68k_irq_m irq);
void cpu_68k_ack_interrupt(cpu_68k_irq_m irq);
int32_t cpu_68k_get_remaining_master_cycles(void);

#endif /* neogeo_h */
