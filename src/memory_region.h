#ifndef memory_region_h
#define memory_region_h

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct memory_region_access_handlers {
	uint8_t (*read_byte)(uint32_t offset);
	uint16_t (*read_word)(uint32_t offset);
	uint32_t (*read_dword)(uint32_t offset);
	void (*write_byte)(uint32_t offset, uint8_t data);
	void (*write_word)(uint32_t offset, uint16_t data);
	void (*write_dword)(uint32_t offset, uint32_t data);
} memory_region_access_handlers_t;

typedef struct memory_region {
	uint8_t *data;
	size_t size;
	uint32_t start_address;
	uint32_t end_address;
	memory_region_access_handlers_t handlers;
} memory_region_t;

#endif /* memory_region_h */
