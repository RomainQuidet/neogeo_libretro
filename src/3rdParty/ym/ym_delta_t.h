//
//  ym_delta_t.h
//  neogeo_libretro
//
//  Created by Romain Quidet on 28/07/2019.
//

#ifndef ym_delta_t_h
#define ym_delta_t_h

#include <stdint.h>

typedef uint8_t (*FM_READBYTE)(void *device, uint32_t offset);
typedef void (*FM_WRITEBYTE)(void *device, uint32_t offset, uint8_t data);
typedef void (*STATUS_CHANGE_HANDLER)(void *chip, uint8_t status_bits);

/* DELTA-T (adpcm type B) struct */
typedef struct {
	uint8_t *read_byte;
	uint32_t read_byte_size;
	FM_WRITEBYTE write_byte;
	int32_t   *output_pointer;/* pointer of output pointers   */
	int32_t   *pan;           /* pan : &output_pointer[pan]   */
	double  freqbase;
	uint32_t  memory_size;
	int     output_range;
	uint32_t  now_addr;       /* current address      */
	uint32_t  now_step;       /* correct step         */
	uint32_t  step;           /* step                 */
	uint32_t  start;          /* start address        */
	uint32_t  limit;          /* limit address        */
	uint32_t  end;            /* end address          */
	uint32_t  delta;          /* delta scale          */
	int32_t   volume;         /* current volume       */
	int32_t   acc;            /* shift Measurement value*/
	int32_t   adpcmd;         /* next Forecast        */
	int32_t   adpcml;         /* current value        */
	int32_t   prev_acc;       /* leveling value       */
	uint8_t   now_data;       /* current rom data     */
	uint8_t   CPU_data;       /* current data from reg 08 */
	uint8_t   portstate;      /* port status          */
	uint8_t   control2;       /* control reg: SAMPLE, DA/AD, RAM TYPE (x8bit / x1bit), ROM/RAM */
	uint8_t   portshift;      /* address bits shift-left:
							   ** 8 for YM2610,
							   ** 5 for Y8950 and YM2608 */
	
	uint8_t   DRAMportshift;  /* address bits shift-right:
							   ** 0 for ROM and x8bit DRAMs,
							   ** 3 for x1 DRAMs */
	
	uint8_t   memread;        /* needed for reading/writing external memory */
	
	/* handlers and parameters for the status flags support */
	STATUS_CHANGE_HANDLER   status_set_handler;
	STATUS_CHANGE_HANDLER   status_reset_handler;
	
	/* note that different chips have these flags on different
	 ** bits of the status register
	 */
	void *  status_change_which_chip;   /* this chip id */
	uint8_t   status_change_EOS_bit;      /* 1 on End Of Sample (record/playback/cycle time of AD/DA converting has passed)*/
	uint8_t   status_change_BRDY_bit;     /* 1 after recording 2 datas (2x4bits) or after reading/writing 1 data */
	uint8_t   status_change_ZERO_bit;     /* 1 if silence lasts for more than 290 milliseconds on ADPCM recording */
	
	/* neither Y8950 nor YM2608 can generate IRQ when PCMBSY bit changes, so instead of above,
	 ** the statusflag gets ORed with PCM_BSY (below) (on each read of statusflag of Y8950 and YM2608)
	 */
	uint8_t   PCM_BSY;        /* 1 when ADPCM is playing; Y8950/YM2608 only */

	uint8_t   reg[16];        /* adpcm registers      */
} YM_DELTAT;


uint8_t ADPCMB_read(YM_DELTAT *adpcmb);
void ADPCMB_write(YM_DELTAT *adpcmb, int r, int v);
void ADPCMB_reset(int panidx, YM_DELTAT *adpcmb);
void ADPCMB_postload(YM_DELTAT *adpcmb, uint8_t *regs);
void ADPCMB_CALC(YM_DELTAT *adpcmb);

#endif /* ym_delta_t_h */
