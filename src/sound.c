#include "cartridge.h"
#include "log.h"
#include "memory_mapping.h"
#include "neogeo.h"
#include "sound.h"
#include "timer.h"
#include "timers_group.h"
#include "3rdParty/musashi/m68k.h"
#include "3rdParty/ym/ym2610.h"
#include "3rdParty/z80/z80.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

void YM2610IrqHandler(int irq);
void YM2610TimerHandler(int channel, int count, double steptime);


/// Buffer for the generated audio
FMSAMPLE *audioBuffer;
size_t audio_buffer_size;

/// How many samples to generate this frame (this can vary because of rounding)
double samplesThisFrameF;

/// How many samples to generate this frame (this can vary because of rounding)
uint32_t samplesThisFrame;

/// The index of the sample corresponding to the current emulated time
uint32_t currentSample;

/// Write index for audio data
uint32_t audioWritePointer;

bool z80NMIDisabled = true;

#pragma mark - Z80 memory map

uint32_t z80_bank_0_offset;
uint32_t z80_bank_1_offset;
uint32_t z80_bank_2_offset;
uint32_t z80_bank_3_offset;

rom_region_t z80_work_ram;

#pragma mark - YM2610 ROMS

rom_region_t pcm_rom_a;
rom_region_t pcm_rom_b;

#pragma mark - Lifecycle

void sound_init() {
	z80_bank_0_offset = 0;
	z80_bank_1_offset = 0;
	z80_bank_2_offset = 0;
	z80_bank_3_offset = 0;
	
	z80_work_ram.data = malloc(Z80_RAM_SIZE);
	z80_work_ram.size = Z80_RAM_SIZE;
	
	audio_buffer_size = sizeof(FMSAMPLE) * 2 * ((AUDIO_SAMPLE_RATE / FRAME_RATE) + 1);
	audioBuffer = malloc(audio_buffer_size);
	
	z80_init(0, Z80_CLOCK, NULL, z80_irq_callback);
}

void sound_reset() {
	z80_bank_0_offset = 0xF000;
	z80_bank_1_offset = 0xE000;
	z80_bank_2_offset = 0xC000;
	z80_bank_3_offset = 0x8000;
	
	memset(z80_work_ram.data, 0, Z80_RAM_SIZE);
	
	samplesThisFrameF = 0;
	samplesThisFrame = 0;
	currentSample = 0;
	audioWritePointer = 0;
	memset(audioBuffer, 0, audio_buffer_size);
	
	z80NMIDisabled = true;
	
	z80_reset();
	
	if (pcm_rom_a.data != NULL) {
		free(pcm_rom_a.data);
		pcm_rom_a.size = 0;
	}
	if (pcm_rom_b.data != NULL) {
		free(pcm_rom_b.data);
		pcm_rom_b.size = 0;
	}
	pcm_rom_a = cartridge_create_pcm_rom(0);
	LOG(LOG_INFO, "sound_reset: found %d KB of PCM A\n", pcm_rom_a.size / 1024);
	pcm_rom_b = cartridge_create_pcm_rom(1);
	LOG(LOG_INFO, "sound_reset: found %d KB of PCM B\n", pcm_rom_b.size / 1024);
	
	ym2610_init(YM2610_CLOCK, AUDIO_SAMPLE_RATE, pcm_rom_a.data, pcm_rom_a.size, pcm_rom_b.data, pcm_rom_b.size, &YM2610TimerHandler, &YM2610IrqHandler);
}

void sound_start_one_frame()
{
	samplesThisFrameF += (double)AUDIO_SAMPLE_RATE / (double)FRAME_RATE;
	samplesThisFrame = (uint32_t)ceil(samplesThisFrameF);
	samplesThisFrameF -= samplesThisFrame;
	audioWritePointer = 0;
}

void sound_update_current_sample()
{
	int32_t remaining_cycles = cpu_68k_get_remaining_master_cycles();
	uint32_t positive_remaining_cycles = remaining_cycles > 0 ? remaining_cycles : 0;
	currentSample = (uint32_t)(round((double)(MASTER_CYCLES_PER_FRAME - positive_remaining_cycles) * (samplesThisFrame - 1) / MASTER_CYCLES_PER_FRAME));
}

void sound_finalize_one_frame()
{
//	LOG(LOG_DEBUG, "sound_finalize_one_frame current samples = %u, already wrote %u\n", currentSample, audioWritePointer);
	// Generate YM2610 samples
	if (audioWritePointer < samplesThisFrame)
		ym2610_update(samplesThisFrame - audioWritePointer);
	
//	LOG(LOG_DEBUG, "sound_finalize_one_frame %u samples this frame vs %u audio write pointer\n", samplesThisFrame, audioWritePointer);
}

uint8_t cpu_z80_read(uint32_t address) {
//	LOG(LOG_DEBUG, "cpu_z80_read at 0x%08X\n", address);
	uint32_t offset = 0;
	if (address < Z80_BANK3_OFFSET) {
		offset = address;
	}
	else if (address >= Z80_BANK3_OFFSET && address < Z80_BANK3_OFFSET + Z80_BANK3_SIZE) {
		offset = z80_bank_3_offset + (address - Z80_BANK3_OFFSET);
	}
	else if (address >= Z80_BANK2_OFFSET && address < Z80_BANK2_OFFSET + Z80_BANK2_SIZE) {
		offset = z80_bank_2_offset + (address - Z80_BANK2_OFFSET);
	}
	else if (address >= Z80_BANK1_OFFSET && address < Z80_BANK1_OFFSET + Z80_BANK1_SIZE) {
		offset = z80_bank_1_offset + (address - Z80_BANK1_OFFSET);
	}
	else if (address >= Z80_BANK0_OFFSET && address < Z80_BANK0_OFFSET + Z80_BANK0_SIZE) {
		offset = z80_bank_0_offset + (address - Z80_BANK0_OFFSET);
	}
	else if (address >= Z80_RAM_OFFSET && address < Z80_RAM_OFFSET + Z80_RAM_SIZE) {
		offset = (address - Z80_RAM_OFFSET);
		return z80_work_ram.data[offset];
	}
	else {
		LOG(LOG_ERROR, "cpu_z80_read outside mapping 0x%08X\n", address);
		assert(0);
		return 0;
	}
	
	return m1_rom.handlers.read_byte(offset);
}

void cpu_z80_write(uint32_t address, uint8_t data) {
//	LOG(LOG_DEBUG, "cpu_z80_write at 0x%08X - 0x%02X\n", address, data);
	if (address >= Z80_RAM_OFFSET && address < Z80_RAM_OFFSET + Z80_RAM_SIZE) {
		uint32_t offset = (address - Z80_RAM_OFFSET);
		z80_work_ram.data[offset] = data;
	}
	else {
		LOG(LOG_ERROR, "cpu_z80_write outside RAM 0x%08X - 0x%02X\n", address, data);
		assert(0);
	}
}

void cpu_z80_set_bank_offset(uint8_t bank, uint8_t offset) {
	switch (bank) {
		case 0:
			z80_bank_0_offset = offset * 0x800;
			break;
		case 1:
			z80_bank_1_offset = offset * 0x1000;
			break;
		case 2:
			z80_bank_2_offset = offset * 0x2000;
			break;
		case 3:
			z80_bank_3_offset = offset * 0x4000;
			break;
		default:
			break;
	}
}

void cpu_z80_trigger_sound_command_nmi() {
	if (z80NMIDisabled) {
		return;
	}
	z80_set_irq_line(INPUT_LINE_NMI, ASSERT_LINE);
	m68k_end_timeslice();
}

void cpu_z80_acknowledge_nmi() {
	z80_set_irq_line(INPUT_LINE_NMI, CLEAR_LINE);
}

#pragma mark - YM2610 callbacks

void ym2610_update_request(void)
{
	sound_update_current_sample();
//	LOG(LOG_DEBUG, "YM2610UpdateRequest currentSample %u - writeptr %u \n", currentSample, audioWritePointer);
	if (currentSample > audioWritePointer + 4)
		ym2610_update(currentSample - audioWritePointer);
}

void ym2610_update_audio_buffer(FMSAMPLE lt, FMSAMPLE rt)
{
	assert(audioWritePointer < samplesThisFrame);
//	LOG(LOG_DEBUG, "YM2610UpdateAudioBuffer lt 0x%08X - rt 0x%08X\n", lt, rt);
	audioBuffer[audioWritePointer * 2] = lt;
	audioBuffer[audioWritePointer * 2 + 1] = rt;
	audioWritePointer++;
}

void YM2610TimerHandler(int channel, int count, double clock)
{
	double      time_seconds;
	uint32_t    time_cycles;

	if (count == 0)
	{
		if (!channel)
			timers.ym2610TimerA->active = false;
		else
			timers.ym2610TimerB->active = false;
	}
	else
	{
		time_seconds = (double)count / clock;
		time_cycles = (uint32_t)secondsToMaster(time_seconds);

		if (!channel)
			timer_arm(timers.ym2610TimerA, time_cycles);
		else
			timer_arm(timers.ym2610TimerB, time_cycles);
	}
}

void YM2610IrqHandler(int irq)
{
	if (irq)
		z80_set_irq_line(0, ASSERT_LINE);
	else
		z80_set_irq_line(0, CLEAR_LINE);
}
