#include "memory_input_output.h"
#include "log.h"
#include "sound.h"

#include <stdint.h>

#include "3rdParty/z80/z80.h"
#include "3rdParty/ym/ym2610.h"

uint16_t io_read_byte_8(uint16_t port)
{
//	LOG(LOG_DEBUG, "io_read_byte_8 0x%04X\n", port);
	switch (port & 0xFF)
	{
		case 0x00:  // Sound code
			cpu_z80_acknowledge_nmi();
			return z80_command;
			
		case 0x04:  // Status port A
			return ym2610_read(0);
			
		case 0x05:  // Read port A
			return ym2610_read(1);
			
		case 0x06:  // Status port B
			return ym2610_read(2);
			
		case 0x07: // Read port B
			return ym2610_read(3);
			
		case 0x08:
			cpu_z80_set_bank_offset(0, port >> 8);
			break;
		case 0x09:
			cpu_z80_set_bank_offset(1, port >> 8);
			break;
		case 0x0A:
			cpu_z80_set_bank_offset(2, port >> 8);
			break;
		case 0x0B:
			cpu_z80_set_bank_offset(3, port >> 8);
			break;
		default:
			LOG(LOG_ERROR, "unknown z80 io_read_byte_8 0x%04X\n", port);
			break;
	}

    return 0;
}

void io_write_byte_8(uint16_t port, uint16_t value)
{
//	LOG(LOG_DEBUG, "io_write_byte_8 0x%04X - 0x%04X\n", port, value);
	switch (port & 0xFF)
	{
		case 0x00: // Clear sound code
		case 0xC0:
			z80_command = 0;
			break;
			
		case 0x04:  // Control port A
			ym2610_write(0, (uint8_t)value);
			break;
			
		case 0x05:  // Data port A
			ym2610_write(1, (uint8_t)value);
			break;
			
		case 0x06:  // Control port B
			ym2610_write(2, (uint8_t)value);
			break;
			
		case 0x07:  // Data port B
			ym2610_write(3, (uint8_t)value);
			break;
			
		case 0x08: // NMI Enable
		case 0x09:
		case 0x0A:
		case 0x0B:
			z80NMIDisabled = false;
			break;
			
		case 0x0C:   // Set audio result
			z80_result = value;
			break;
			
		case 0x18: // NMI Disable
			z80NMIDisabled = true;
			break;
		default:
			LOG(LOG_ERROR, "unknown z80 io_write_byte_8 0x%04X - 0x%04X\n", port, value);
			break;
	}
}

uint8_t program_read_byte_8(uint16_t addr)
{
    return cpu_z80_read(addr);
}

void program_write_byte_8(uint16_t addr, uint8_t value)
{
	cpu_z80_write(addr, value);
}

int z80_irq_callback(int parameter)
{
//	LOG(LOG_DEBUG, "z80_irq_callback %d\n", parameter);
    return 0x38;
}
