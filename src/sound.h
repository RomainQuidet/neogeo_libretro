#ifndef sound_h
#define sound_h

#include "memory_region.h"
#include "rom_region.h"

#include <stdio.h>


static uint32_t AUDIO_SAMPLE_RATE = 44100;

extern bool z80NMIDisabled;
extern int16_t *audioBuffer;
extern uint32_t samplesThisFrame;

void sound_init(void);
void sound_reset(void);

void sound_start_one_frame(void);
void sound_finalize_one_frame(void);

uint8_t cpu_z80_read(uint32_t address);
void cpu_z80_write(uint32_t address, uint8_t data);
void cpu_z80_set_bank_offset(uint8_t bank, uint8_t offset);
void cpu_z80_trigger_sound_command_nmi(void);
void cpu_z80_acknowledge_nmi(void);

#endif /* sound_h */
