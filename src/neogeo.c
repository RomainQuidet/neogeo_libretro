#include "aux_inputs.h"
#include "endian.h"
#include "cartridge.h"
#include "common_tools.h"
#include "log.h"
#include "memory_backup_ram.h"
#include "memory_input_output.h"
#include "memory_mapping.h"
#include "memory_palettes_ram.h"
#include "memory_region.h"
#include "memory_work_ram.h"
#include "neogeo.h"
#include "rom_region.h"
#include "sound.h"
#include "timer.h"
#include "timers_group.h"
#include "video.h"

#include "3rdParty/musashi/m68kcpu.h"
#include "3rdParty/pd4990a/pd4990a.h"
#include "3rdParty/z80/z80.h"

#include <string.h>

#pragma mark - System ROM regions

rom_region_t system_y_zoom_rom;		// L0_ROM - https://wiki.neogeodev.org/index.php?title=L0_ROM
rom_region_t system_fix_rom;		// SFIX ROM - https://wiki.neogeodev.org/index.php?title=SFIX_ROM

#pragma mark - 68K CPU BUS / Memory regions

memory_region_t p_rom_bank1_vector;
memory_region_t system_rom_vector;

memory_region_t input_output;

memory_region_t memory_card;
memory_region_t system_rom;			// SYSTEM ROM - https://wiki.neogeodev.org/index.php?title=System_ROM
memory_region_t system_rom_mirror;


int32_t remainingCyclesThisFrame;
int32_t m68kCyclesThisFrame;
uint8_t pending_interrupts;
int32_t z80_remaining_cycles;
double currentTimeSeconds;

#pragma mark -

static void input_output_init(void);
static void system_rom_init(rom_region_t rom);
static void memory_card_init(void);

#pragma mark - Components

memory_region_t *current_palette_ram;
rom_region_t *current_fix_rom;

#pragma mark - Public

void neogeo_initialize() {
	// 68K CPU init
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_init();
	
	// SYSTEM ROMs init
	memset(&system_y_zoom_rom, 0, sizeof(rom_region_t));
	memset(&system_fix_rom, 0, sizeof(rom_region_t));
	
	// 68K regions
	cartridge_init();
	work_ram_init();
	input_output_init();
	palettes_rams_init();
	memory_card_init();
	memset(&system_rom, 0, sizeof(memory_region_t));
	memset(&system_rom_mirror, 0, sizeof(memory_region_t));
	backup_ram_init();
	
	// HARDWARE
	video_init();
	sound_init();
	timers_group_init();
	pd4990a_init();
}

void neogeo_deinitialize() {
	
}

void neogeo_reset() {
	neogeo_use_board_p_rom();
	if (cartridge_plugged_in()) {
		neogeo_use_cartridge_fix_rom();
	}
	else {
		neogeo_use_board_fix_rom();
	}
	
	video_reset();
	sound_reset();
	
	palettes_rams_reset();
	current_palette_ram = &palettes_ram1;
	
	timers_group_reset();
	remainingCyclesThisFrame = 0;
	m68kCyclesThisFrame = 0;
	z80_remaining_cycles = 0;
	currentTimeSeconds = 0;
	
	LOG(LOG_DEBUG, "neogeo_reset pulse\n");
	m68k_pulse_reset();
	cpu_68k_set_interrupt(Reset);
}

void neogeo_runOneFrame() {
	remainingCyclesThisFrame += MASTER_CYCLES_PER_FRAME;
	LOG(LOG_DEBUG, "neogeo_runOneFrame for %lld cycles \n", remainingCyclesThisFrame);
	
	sound_start_one_frame();
	
	while (remainingCyclesThisFrame > 0) {
		uint32_t next_event_cycles = timer_group_cycles_before_next_event();
		uint32_t cycles_slice = next_event_cycles < remainingCyclesThisFrame ? next_event_cycles : remainingCyclesThisFrame;
		
//		PROFILE(p_m68k, ProfilingCategory::CpuM68K);
		uint32_t elapsed_cycles = m68kToMaster(m68k_execute(masterToM68k(cycles_slice)));
//		PROFILE_END(p_m68k);

		z80_remaining_cycles += elapsed_cycles;
		if (z80_remaining_cycles > 0) {
			uint32_t z80_elapsed;
			//PROFILE(p_z80, ProfilingCategory::CpuZ80);
			z80_elapsed = z80ToMaster(z80_execute(masterToZ80(z80_remaining_cycles)));
			z80_remaining_cycles -= z80_elapsed;
		}
		
		remainingCyclesThisFrame -= elapsed_cycles;
		
		currentTimeSeconds += masterToSeconds(elapsed_cycles);

//		PROFILE(p_videoIRQ, ProfilingCategory::VideoAndIRQ);
		timer_group_consume_cycles(elapsed_cycles);
//		PROFILE_END(p_videoIRQ);
	}
	LOG(LOG_DEBUG, "68k cycles remaining: %d - z80 cycles remaining %d\n", remainingCyclesThisFrame, z80_remaining_cycles);
	sound_finalize_one_frame();
}

bool neogeo_is_system_ready() {
	if (system_rom.data == NULL || system_rom.size == 0)
		return false;
	if (system_fix_rom.data == NULL || system_fix_rom.size == 0)
		return false;
	if (system_y_zoom_rom.data == NULL || system_y_zoom_rom.size == 0)
		return false;
	
	return true;
}

#pragma mark Palette RAM

void neogeo_use_palette_bank_1() {
	LOG(LOG_DEBUG, "neogeo_use_palette_bank_1\n");
	current_palette_ram = &palettes_ram1;
	video_convert_current_palette_bank();
}

void neogeo_use_palette_bank_2() {
	LOG(LOG_DEBUG, "neogeo_use_palette_bank_2\n");
	current_palette_ram = &palettes_ram2;
	video_convert_current_palette_bank();
}

#pragma mark Fix ROM

void neogeo_use_board_fix_rom() {
	LOG(LOG_DEBUG, "neogeo_use_board_fix_rom\n");
	current_fix_rom = &system_fix_rom;
	//TODO: M1 ROM too
}

void neogeo_use_cartridge_fix_rom() {
	LOG(LOG_DEBUG, "neogeo_use_cartridge_fix_rom\n");
	if (cartridge_plugged_in() == false) {
		LOG(LOG_ERROR, "neogeo_use_cartridge_fix_rom when cartridge is not plugged in\n");
		return;
	}
	current_fix_rom = cartridge_get_first_fix_rom();
	//M1
}

#pragma mark Vectors

void neogeo_use_board_p_rom() {
	LOG(LOG_DEBUG, "neogeo_use_board_p_rom\n");
	p_rom_bank1_vector = system_rom;
	p_rom_bank1_vector.start_address = p_rom_bank1.start_address;
	
	system_rom_vector = p_rom_bank1;
	system_rom_vector.start_address = system_rom.start_address;
}

void neogeo_use_cartridge_p_rom() {
	LOG(LOG_DEBUG, "neogeo_use_cartridge_p_rom\n");
	if (cartridge_plugged_in() == false) {
		LOG(LOG_ERROR, "neogeo_use_cartridge_p_rom when cartridge is not plugged in\n");
		return;
	}
	p_rom_bank1_vector = p_rom_bank1;
	system_rom_vector = system_rom;
}

#pragma mark System ROMs

bool neogeo_set_system_Y_zoom_ROM(rom_region_t rom) {
	system_y_zoom_rom = rom;
	return true;
}

bool neogeo_set_system_fix_ROM(rom_region_t rom) {
	system_fix_rom = rom;
	current_fix_rom = &system_fix_rom;
	return true;
}

bool neogeo_set_system_ROM(rom_region_t rom) {
	if (rom.size > SYSTEM_ROM_SIZE) {
		LOG(LOG_ERROR, "neogeo system ROM is too big (128KB max) : %ld\n", rom.size);
		free(rom.data);
		return false;
	}
	LOG(LOG_DEBUG, "neogeo_set_system_ROM %p - %ld bytes\n", rom.data, rom.size);
	byte_swap_p_rom_if_needed(rom.data, rom.size);
	system_rom_init(rom);
	return true;
}

#pragma mark 68K CPU bus access

const memory_region_t* cpu_68k_memory_region_for_address(uint32_t address) {
	if (address <= ROM_BANK1_END) {
		if ((address - ROM_BANK1_START) < ROM_VECTOR_TABLE_SIZE) {
			return &p_rom_bank1_vector;
		}
		return &p_rom_bank1;
	}
	else if (address <= WORK_RAM_END && address >= WORK_RAM_START) {
		return &work_ram;
	}
	else if (address <= WORK_RAM_MIRROR_END && address >= WORK_RAM_MIRROR_START) {
		return &work_ram_mirror;
	}
	else if (address <= ROM_BANK2_END && address >= ROM_BANK2_START) {
		return &p_rom_bank2;
	}
	else if (address <= IO_PORTS_END && address >= IO_PORTS_START) {
		return &input_output;
	}
	else if (address <= PALETTES_RAM_END && address >= PALETTES_RAM_START) {
		return current_palette_ram;
	}
	else if (address <= PALETTES_RAM_MIRROR_END && address >= PALETTES_RAM_MIRROR_START) {
		return &palettes_ram_mirror;
	}
	else if (address <= MEMCARD_END && address >= MEMCARD_START) {
		return &memory_card;
	}
	else if (address <= SYSTEM_ROM_END && address >= SYSTEM_ROM_START) {
		if ((address - SYSTEM_ROM_START) < ROM_VECTOR_TABLE_SIZE) {
			return &system_rom_vector;
		}
		return &system_rom;
	}
	else if (address <= SYSTEM_ROM_MIRROR_END && address >= SYSTEM_ROM_MIRROR_START) {
		return &system_rom_mirror;
	}
	else if (address <= BACKUP_RAM_END && address >= BACKUP_RAM_START) {
		return &backup_ram;
	}
	else if (address <= BACKUP_RAM_MIRROR_END && address >= BACKUP_RAM_MIRROR_START) {
		return &backup_ram;
	}
	
	LOG(LOG_ERROR, "68K CPU bus access outside known address 0x%08X\n", address);
	return NULL;
}

void cpu_68k_update_interrupts() {
	unsigned int level = 0;
	if (pending_interrupts & VBlank) {
		level = 1;
	}
	if (pending_interrupts & Timer) {
		level = 2;
	}
	if (pending_interrupts & Reset) {
		level = 3;
	}
	LOG(LOG_DEBUG, "cpu_68k_update_interrupts level %u\n", level);
	m68k_set_irq(level);
}

void cpu_68k_set_interrupt(cpu_68k_irq_m irq) {
	if (pending_interrupts & irq) {
		return;
	}
	pending_interrupts |= irq;
	
	cpu_68k_update_interrupts();
}

void cpu_68k_ack_interrupt(cpu_68k_irq_m irq) {
	pending_interrupts &= ~irq;
}

int32_t cpu_68k_get_remaining_master_cycles() {
	return remainingCyclesThisFrame;
}

#pragma mark - YM2610

double ym2610_fm_get_time_now(void)
{
	return currentTimeSeconds;
}

#pragma mark - Private

#pragma mark system P ROM access

static uint8_t system_rom_read_byte(uint32_t offset) {
	return system_rom.data[offset];
}

static uint16_t system_rom_read_word(uint32_t offset) {
	return BIG_ENDIAN_WORD(*((uint16_t *)(system_rom.data + offset)));
}

static uint32_t system_rom_read_dword(uint32_t offset) {
	return BIG_ENDIAN_DWORD(*((uint32_t *)(system_rom.data + offset)));
}

static void system_rom_init(rom_region_t rom) {
	memset(&system_rom, 0, sizeof(memory_region_t));
	memset(&system_rom_mirror, 0, sizeof(memory_region_t));
	
	system_rom.data = rom.data;
	system_rom.size = rom.size;
	system_rom.start_address = SYSTEM_ROM_START;
	system_rom.end_address = SYSTEM_ROM_END;
	system_rom.handlers.read_byte = &system_rom_read_byte;
	system_rom.handlers.read_word = &system_rom_read_word;
	system_rom.handlers.read_dword = &system_rom_read_dword;
	
	system_rom_mirror.data = system_rom.data;
	system_rom_mirror.size = system_rom.size;
	system_rom_mirror.start_address = SYSTEM_ROM_MIRROR_START;
	system_rom_mirror.end_address = SYSTEM_ROM_MIRROR_END;
	system_rom_mirror.handlers = system_rom.handlers;
		
	uint8_t nationality = system_rom.handlers.read_byte(0x401);
	char *nat_str;
	switch (nationality) {
		case 0:
			nat_str = "Japan";
			break;
		case 1:
			nat_str = "USA";
			break;
		case 2:
			nat_str = "Europe";
			break;
		default:
			nat_str = "Unknown";
			break;
	}
	LOG(LOG_INFO, "BIOS nationality: %s\n", nat_str);
	
	uint8_t board_type = system_rom.handlers.read_byte(0x400);
	BoardType_m board = MVS;
	switch (board_type) {
		case 0:
			board = AES;
			break;
		case 1:
			board = MVS;
			break;
		default:
			break;
	}
	aux_input_set_hardware(board);
	LOG(LOG_INFO, "BIOS board type: %s\n", board == AES ? "AES" : "MVS");
}

#pragma mark I/O

static void input_output_init(void) {
	memset(&input_output, 0, sizeof(memory_region_t));
	
	input_output.data = NULL;
	input_output.size = 0;
	input_output.start_address = IO_PORTS_START;
	input_output.end_address = IO_PORTS_END;
	input_output.handlers = memory_input_output_handlers;
}

#pragma mark memory card

static uint8_t memory_card_read_byte(uint32_t offset) {
	return memory_card.data[offset];
}

static uint16_t memory_card_read_word(uint32_t offset) {
	return *((uint16_t *)(memory_card.data + offset));
}

static uint32_t memory_card_read_dword(uint32_t offset) {
	return *((uint32_t *)(memory_card.data + offset));
}

static void memory_card_write_byte(uint32_t offset, uint8_t data) {
	memory_card.data[offset] = data;
}

static void memory_card_write_word(uint32_t offset, uint16_t data) {
	*((uint16_t *)(memory_card.data + offset)) = data;
}

static void memory_card_write_dword(uint32_t offset, uint32_t data) {
	*((uint32_t *)(memory_card.data + offset)) = data;
}

static void memory_card_init(void) {
	memset(&memory_card, 0, sizeof(memory_region_t));
	
	memory_card.data = malloc(MEMCARD_SIZE);
	memory_card.size = MEMCARD_SIZE;
	memory_card.start_address = MEMCARD_START;
	memory_card.end_address = MEMCARD_END;
	memory_card.handlers.read_byte = &memory_card_read_byte;
	memory_card.handlers.read_word = &memory_card_read_word;
	memory_card.handlers.read_dword = &memory_card_read_dword;
	memory_card.handlers.write_byte = &memory_card_write_byte;
	memory_card.handlers.write_word = &memory_card_write_word;
	memory_card.handlers.write_dword = &memory_card_write_dword;
}
