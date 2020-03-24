#ifndef common_tools_h
#define common_tools_h

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

void byte_swap_p_rom_if_needed(uint8_t * mem, size_t length);
bool is_p_rom_init_vector(uint8_t *rom);

#endif /* common_tools_h */
