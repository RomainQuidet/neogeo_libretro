#include "common_tools.h"
#include "endian.h"
#include "log.h"

void byte_swap_p_rom_if_needed(uint8_t * rom, size_t length)
{
	int i, j;
	
	uint32_t initVector0 = ((uint32_t*)rom)[0];
	LOG(LOG_DEBUG, "P ROM init vector 0 0x%08X\n", initVector0);
	
	if (rom[1] != 0x10 && rom[2] != 0xF3) {
		LOG(LOG_DEBUG, "P ROM init vector needs byte swap\n");
		// swap bytes in each word
		for (i = 0; i < length - 1; i += 2) {
			j = rom[i];
			rom[i] = rom[i + 1];
			rom[i + 1] = j;
		}
		
		initVector0 = ((uint32_t*)rom)[0];
		LOG(LOG_DEBUG, "P ROM init vector 0 0x%08X\n", initVector0);
	}
}

bool is_p_rom_init_vector(uint8_t *rom)
{
	static uint32_t vector0 = 0x100000F3;
	static uint32_t vector1 = 0xC0000204;
	uint32_t *p = (uint32_t *)rom;
	LOG(LOG_DEBUG, "vector 0 0x%08X - 1 0x%08X\n", p[0], p[1]);
	return p[0] == BIG_ENDIAN_DWORD(vector0) && p[1] == BIG_ENDIAN_DWORD(vector1);
}
