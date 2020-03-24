#ifndef rom_region_h
#define rom_region_h

typedef struct rom_region {
	uint8_t* data;
	size_t size;
} rom_region_t;

#endif /* rom_region_h */
