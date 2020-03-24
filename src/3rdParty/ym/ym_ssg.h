//
//  ym_ssg.h
//  neogeo_libretro
//
//  Created by Romain Quidet on 28/07/2019.
//

#ifndef ym_ssg_h
#define ym_ssg_h

#include <stdint.h>
#include <stdbool.h>

#pragma once

#define ALL_8910_CHANNELS -1

/* Internal resistance at Volume level 7. */

#define AY8910_INTERNAL_RESISTANCE  (356)
#define YM2149_INTERNAL_RESISTANCE  (353)

/*
 * The following is used by all drivers not reviewed yet.
 * This will like the old behavior, output between
 * 0 and 7FFF
 */
#define AY8910_LEGACY_OUTPUT        (0x01)

/*
 * Specifying the next define will simulate the special
 * cross channel mixing if outputs are tied together.
 * The driver will only provide one stream in this case.
 */
#define AY8910_SINGLE_OUTPUT        (0x02)

/*
 * The following define is the default behavior.
 * Output level 0 is 0V and 7ffff corresponds to 5V.
 * Use this to specify that a discrete mixing stage
 * follows.
 */
#define AY8910_DISCRETE_OUTPUT      (0x04)

/*
 * The following define causes the driver to output
 * resistor values. Intended to be used for
 * netlist interfacing.
 */

#define AY8910_RESISTOR_OUTPUT      (0x08)

/*
 * This define specifies the initial state of YM2149
 * pin 26 (SEL pin). By default it is set to high,
 * compatible with AY8910.
 */
/* TODO: make it controllable while it's running (used by any hw???) */
#define YM2149_PIN26_HIGH           (0x00) /* or N/C */
#define YM2149_PIN26_LOW            (0x10)

typedef struct
{
	double r_up;
	double r_down;
	int    res_count;
	double res[32];
} ay_ym_param;

static const uint8_t NUM_CHANNELS = 3;

typedef struct {
//	psg_type_t m_type;
	int m_streams;
	int m_ioports;
	int m_ready;
//	sound_stream *m_channel;
	bool m_active;
	int32_t m_register_latch;
	uint8_t m_regs[16];
	int32_t m_last_enable;
	int32_t m_count[NUM_CHANNELS];
	uint8_t m_output[NUM_CHANNELS];
	uint8_t m_prescale_noise;
	int32_t m_count_noise;
	int32_t m_count_env;
	int8_t m_env_step;
	uint32_t m_env_volume;
	uint8_t m_hold,m_alternate,m_attack,m_holding;
	int32_t m_rng;
	uint8_t m_mode;
	uint8_t m_env_step_mask;
	/* init parameters ... */
	int m_step;
	int m_zero_is_off;
	uint8_t m_vol_enabled[NUM_CHANNELS];
	const ay_ym_param *m_par;
	const ay_ym_param *m_par_env;
	int32_t m_vol_table[NUM_CHANNELS][16];
	int32_t m_env_table[NUM_CHANNELS][32];
	int32_t *m_vol3d_table;
	int m_flags;          /* Flags */
	int m_res_load[3];    /* Load on channel in ohms */
//	devcb_read8 m_port_a_read_cb;
//	devcb_read8 m_port_b_read_cb;
//	devcb_write8 m_port_a_write_cb;
//	devcb_write8 m_port_b_write_cb;
} SSG;

void ssg_init(SSG *device);
void ssg_reset(SSG *device);
uint8_t ssg_read(SSG *device);
void ssg_write(SSG *device, int addr, uint8_t data);
void ssg_set_clock(SSG *device, int clock);
int32_t ssg_update_one_sample(SSG *device);

extern void ssg_needs_update(void);

#endif /* ym_ssg_h */
