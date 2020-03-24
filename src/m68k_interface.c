#include "memory_region.h"
#include "neogeo.h"
#include "rom_region.h"
#include "log.h"
#include "3rdParty/musashi/m68kcpu.h"

#include <stdint.h>

void m68ki_exception_bus_error(void) {
     LOG(LOG_ERROR, "Bus Error @ PC=%X.\n", REG_PPC);

     uint32_t sr = m68ki_init_exception();

     /* If we were processing a bus error, address error, or reset,
     * this is a catastrophic failure.
     * Halt the CPU
     */
     if (CPU_RUN_MODE == RUN_MODE_BERR_AERR_RESET)
     {
 //          m68k_read_memory_8(0x00ffff01);
         CPU_STOPPED = STOP_LEVEL_HALT;
         return;
     }
     CPU_RUN_MODE = RUN_MODE_BERR_AERR_RESET;

     /* Note: This is implemented for 68000 only! */
     m68ki_stack_frame_buserr(sr);

     m68ki_jump_vector(EXCEPTION_BUS_ERROR);

     /* Use up some clock cycles and undo the instruction's cycles */
     USE_CYCLES(CYC_EXCEPTION[EXCEPTION_BUS_ERROR] - CYC_INSTRUCTION[REG_IR]);

     SET_CYCLES(CYC_INSTRUCTION[REG_IR]);
}

uint32_t m68k_read_memory_8(uint32_t address) {
	const memory_region_t *memory_region = cpu_68k_memory_region_for_address(address);
	if (!memory_region) {
		LOG(LOG_DEBUG, "m68k_read_memory_8 missing region for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return 0xFF;
	}
	
	if (memory_region->handlers.read_byte == NULL) {
		LOG(LOG_DEBUG, "m68k_read_memory_8 read function missing for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return 0xFF;
	}
	
	return memory_region->handlers.read_byte(address - memory_region->start_address);
}

void m68k_write_memory_8(uint32_t address, uint32_t data) {
	const memory_region_t *memory_region = cpu_68k_memory_region_for_address(address);
	if (!memory_region) {
		LOG(LOG_DEBUG, "m68k_write_memory_8 missing region for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return;
	}

	if (memory_region->handlers.write_byte == NULL) {
		LOG(LOG_DEBUG, "m68k_write_memory_8 write function missing for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return;
	}
	memory_region->handlers.write_byte(address - memory_region->start_address, (uint8_t)data);
}

uint32_t m68k_read_memory_16(uint32_t address) {
	const memory_region_t *memory_region = cpu_68k_memory_region_for_address(address);
	if (!memory_region) {
		LOG(LOG_DEBUG, "m68k_read_memory_16 missing region for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return 0xFFFF;
	}

	if (memory_region->handlers.read_word == NULL) {
		LOG(LOG_DEBUG, "m68k_read_memory_16 read function missing for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return 0xFFFF;
	}
	
	return memory_region->handlers.read_word(address - memory_region->start_address);
}

void m68k_write_memory_16(uint32_t address, uint32_t data) {
	const memory_region_t *memory_region = cpu_68k_memory_region_for_address(address);
	if (!memory_region) {
		LOG(LOG_DEBUG, "m68k_write_memory_16 missing region for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return;
	}
	
	if (memory_region->handlers.write_word == NULL) {
		LOG(LOG_DEBUG, "m68k_write_memory_16 write function missing for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return;
	}
	memory_region->handlers.write_word(address - memory_region->start_address, (uint16_t)data);
}

uint32_t m68k_read_memory_32(uint32_t address) {
	const memory_region_t *memory_region = cpu_68k_memory_region_for_address(address);
	if (!memory_region) {
		LOG(LOG_DEBUG, "m68k_read_memory_32 missing region for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return 0xFFFF;
	}
	
	if (memory_region->handlers.read_dword == NULL) {
		LOG(LOG_DEBUG, "m68k_read_memory_32 read function missing for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return 0xFFFF;
	}
	
	return memory_region->handlers.read_dword(address - memory_region->start_address);
}

void m68k_write_memory_32(uint32_t address, uint32_t data) {
	const memory_region_t *memory_region = cpu_68k_memory_region_for_address(address);
	if (!memory_region) {
		LOG(LOG_DEBUG, "m68k_write_memory_32 missing region for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return;
	}
	
	if (memory_region->handlers.write_dword == NULL) {
		LOG(LOG_DEBUG, "m68k_write_memory_32 write function missing for address 0x%08X\n", address);
		m68ki_exception_bus_error();
		return;
	}
	memory_region->handlers.write_dword(address - memory_region->start_address, data);
}
