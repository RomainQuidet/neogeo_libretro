// license:GPL-2.0+
// copyright-holders:Jarek Burczynski,Tatsuyuki Satoh
/*
 File: fm.h -- header file for software emulation for FM sound generator
 
 */

#ifndef _YM2610_H_
#define _YM2610_H_

#include <stdint.h>

typedef void(*FM_TIMERHANDLER) (int channel, int count, double stepTime);
typedef void(*FM_IRQHANDLER) (int irq);

void ym2610_init(int baseclock, int rate, void *pcmroma, size_t pcmsizea, void *pcmromb, size_t pcmsizeb, FM_TIMERHANDLER TimerHandler, FM_IRQHANDLER IRQHandler);
void ym2610_reset(void);
void ym2610_update(int length);
int ym2610_write(int addr, uint8_t value);
uint8_t ym2610_read(int addr);
int ym2610_timerOver(int channel);

typedef int16_t FMSAMPLE;

/* You need to implement those methods */
/*
 for busy flag emulation , function YM2610_FM_GET_TIME_NOW() should be
 return the present time in second unit with (double) value
 in timer.c
 */
extern double ym2610_fm_get_time_now(void);
extern void ym2610_update_request(void);
extern void ym2610_update_audio_buffer(FMSAMPLE lt, FMSAMPLE rt);

#endif /* _YM2610_H_ */
