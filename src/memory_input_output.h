#ifndef memory_input_output_h
#define memory_input_output_h

#include "memory_region.h"

extern uint8_t z80_command;
extern uint8_t z80_result;

extern const memory_region_access_handlers_t memory_input_output_handlers;

#endif /* memory_input_output_h */
