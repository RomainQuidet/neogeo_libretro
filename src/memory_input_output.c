#include "aux_inputs.h"
#include "joypads.h"
#include "log.h"
#include "memory_input_output.h"
#include "memory_mapping.h"
#include "mvs_dips.h"
#include "neogeo.h"
#include "sound.h"
#include "video.h"

#include "3rdParty/pd4990a/pd4990a.h"


static uint8_t read_io_register(uint32_t address);
static void write_io_register(uint32_t address, uint8_t data);
static void write_system_register(uint32_t address, uint8_t data);

#pragma mark - Private

static uint8_t input_output_read_byte(uint32_t offset) {
	if (offset <= (IO_REG_END - IO_PORTS_START)) {
		return read_io_register(IO_PORTS_START + offset);
	}
	else if (offset >= (VID_REG_START - IO_PORTS_START)
			 && offset <= (VID_REG_END - IO_PORTS_START)) {
		return video.vram.handlers.read_byte(offset);
	}
	LOG(LOG_DEBUG, "input_output_read_byte incorrect address 0x%08X\n", IO_PORTS_START + offset);
	return 0;
}

static uint16_t input_output_read_word(uint32_t offset) {
	if (offset <= (IO_REG_END - IO_PORTS_START)) {
		return (uint16_t)read_io_register(IO_PORTS_START + offset);
	}
	else if (offset >= (VID_REG_START - IO_PORTS_START)
			 && offset <= (VID_REG_END - IO_PORTS_START)) {
		return video.vram.handlers.read_word(offset);
	}
	LOG(LOG_DEBUG, "input_output_read_word incorrect address 0x%08X\n", IO_PORTS_START + offset);
	return 0;
}

static uint32_t input_output_read_dword(uint32_t offset) {
	if (offset <= (IO_REG_END - IO_PORTS_START)) {
		return (uint32_t)read_io_register(IO_PORTS_START + offset);
	}
	else if (offset >= (VID_REG_START - IO_PORTS_START)
			 && offset <= (VID_REG_END - IO_PORTS_START)) {
		return video.vram.handlers.read_dword(offset);
	}
	LOG(LOG_DEBUG, "input_output_read_dword incorrect address 0x%08X\n", IO_PORTS_START + offset);
	return 0;
}

static void input_output_write_byte(uint32_t offset, uint8_t data) {
	if (offset <= (IO_REG_END - IO_PORTS_START)) {
		write_io_register(IO_PORTS_START + offset, data);
	}
	else if (offset <= SYS_REG_END - IO_PORTS_START) {
		write_system_register(IO_PORTS_START + offset, data);
	}
	else if (offset >= (VID_REG_START - IO_PORTS_START)
			 && offset <= (VID_REG_END - IO_PORTS_START)) {
		video.vram.handlers.write_byte(offset, data);
	}
	else {
		LOG(LOG_DEBUG, "input_output_write_byte incorrect address 0x%08X - 0x%02X\n", IO_PORTS_START + offset, data);
	}
}

static void input_output_write_word(uint32_t offset, uint16_t data) {
	if (offset <= (IO_REG_END - IO_PORTS_START)) {
		write_io_register(IO_PORTS_START + offset, data);
		write_io_register(IO_PORTS_START + offset, data >> 8);
	}
	else if (offset <= SYS_REG_END - IO_PORTS_START) {
		write_system_register(IO_PORTS_START + offset, data);
	}
	else if (offset >= (VID_REG_START - IO_PORTS_START)
			 && offset <= (VID_REG_END - IO_PORTS_START)) {
		video.vram.handlers.write_word(offset, data);
	}
	else {
		LOG(LOG_DEBUG, "input_output_write_word incorrect address 0x%08X - 0x%04X\n", IO_PORTS_START + offset, data);
	}
}

static void input_output_write_dword(uint32_t offset, uint32_t data) {
	if (offset <= (IO_REG_END - IO_PORTS_START)) {
		write_io_register(IO_PORTS_START + offset, data);
	}
	else if (offset <= SYS_REG_END - IO_PORTS_START) {
		write_system_register(IO_PORTS_START + offset, data);
	}
	else if (offset >= (VID_REG_START - IO_PORTS_START)
			 && offset <= (VID_REG_END - IO_PORTS_START)) {
		video.vram.handlers.write_dword(offset, data);
	}
	else {
		LOG(LOG_DEBUG, "input_output_write_dword incorrect address 0x%08X - 0x%08X\n", IO_PORTS_START + offset, data);
	}
}

const memory_region_access_handlers_t memory_input_output_handlers = {
	&input_output_read_byte,
	&input_output_read_word,
	&input_output_read_dword,
	&input_output_write_byte,
	&input_output_write_word,
	&input_output_write_dword
};

#pragma mark - internals
#pragma mark I/O

uint8_t z80_command = 0;
uint8_t z80_result = 0;

static uint8_t read_io_register(uint32_t address) {
	uint8_t result = 0xFF;
	switch (address) {
		case REG_P1CNT:
			result = joypad_port1;
			break;
		case REG_DIPSW:
			result = mvs_dips;
			break;
		case REG_SYSTYPE:
			result = 0;
			break;
		case REG_SOUND:
			result = z80_result;
			LOG(LOG_DEBUG, "Z80 read command 0x%02X\n", result);
			break;
		case REG_STATUS_A:
			result = 0x1F;	// Coin in + service button inactive
			result |= (pd4990a_read_testbit() & 0x01) << 6;
			result |= (pd4990a_read_databit() & 0x01) << 7;
			break;
		case REG_P2CNT:
			result = joypad_port2;
			break;
		case REG_STATUS_B:
			result = aux_inputs;
			break;
		case REG_POUTPUT:
		case REG_CRDBANK:
		case REG_SLOT:
		case REG_LEDLATCHES:
		case REG_LEDDATA:
		case REG_RTCCTRL:
		case REG_RESETCC1:
		case REG_RESETCC2:
		case REG_RESETCL1:
		case REG_RESETCL2:
		case REG_SETCC1:
		case REG_SETCC2:
		case REG_SETCL1:
		case REG_SETCL2:
			// no read
			break;
		default:
			LOG(LOG_DEBUG, "read_register unknown register address 0x%08X\n", address);
			break;
	}
//	LOG(LOG_DEBUG, "read_io_register 0x%08X - 0x%02X\n", address, result);
	return result;
}

static void write_io_register(uint32_t address, uint8_t data) {
//	LOG(LOG_DEBUG, "write_io_register 0x%08X - 0x%02\n", address, data);
	switch (address) {
		case REG_P1CNT:
			// nop
			break;
		case REG_DIPSW:
			//TODO: kick watchdog
			break;
		case REG_SYSTYPE:
			// nop
			break;
		case REG_SOUND:
			LOG(LOG_DEBUG, "Z80 command 0x%02X\n", data);
			z80_command = data;
			cpu_z80_trigger_sound_command_nmi();
			break;
		case REG_STATUS_A:
			// nop
			break;
		case REG_P2CNT:
			// nop
			break;
		case REG_STATUS_B:
			// nop
			break;
		case REG_POUTPUT:
			// nothing to emulate yet
			break;
		case REG_CRDBANK:
			//TODO: memory card bank selection
			break;
		case REG_SLOT:
			//TODO: Mirror of REG_POUTPUT on the AES ?
			break;
		case REG_LEDLATCHES:
		case REG_LEDDATA:
			// LED not emulated
			break;
		case REG_RTCCTRL:
			pd4990a_write_control(data);
			break;
		case REG_RESETCC1:
		case REG_RESETCC2:
		case REG_RESETCL1:
		case REG_RESETCL2:
		case REG_SETCC1:
		case REG_SETCC2:
		case REG_SETCL1:
		case REG_SETCL2:
			// nothing to emulate yet
			break;
		default:
			LOG(LOG_DEBUG, "write_io_register unknown register address 0x%08X - 0x%02X\n", address, data);
			break;
	}
}

#pragma mark SYSTEM REG

static void write_system_register(uint32_t address, uint8_t data) {
	LOG(LOG_DEBUG, "write_system_register register address 0x%08X - 0x%02X\n", address, data);
	switch (address) {
		case REG_NOSHADOW:
		case REG_SHADOW:
			break;
		case REG_SWPBIOS:
			neogeo_use_board_p_rom();
			break;
		case REG_SWPROM:
			neogeo_use_cartridge_p_rom();
			break;
		case REG_CRDUNLOCK1:
		case REG_CRDLOCK1:
		case REG_CRDLOCK2:
		case REG_CRDUNLOCK2:
		case REG_CRDREGSEL:
		case REG_CRDNORMAL:
			break;
		case REG_BRDFIX:
			neogeo_use_board_fix_rom();
			break;
		case REG_CRTFIX:
			neogeo_use_cartridge_fix_rom();
			break;
		case REG_SRAMLOCK:
		case REG_SRAMULOCK:
			break;
		case REG_PALBANK1:
			neogeo_use_palette_bank_2();
			break;
		case REG_PALBANK0:
			neogeo_use_palette_bank_1();
			break;
		default:
			LOG(LOG_DEBUG, "write_system_register unkown register address 0x%08X\n", address);
			break;
	}
}
