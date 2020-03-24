#ifndef joypads_h
#define joypads_h

#include <stdint.h>

#define JOYPAD_PORT_MASK_UP		0x01
#define JOYPAD_PORT_MASK_DOWN	0x02
#define JOYPAD_PORT_MASK_LEFT	0x04
#define JOYPAD_PORT_MASK_RIGHT	0x08
#define JOYPAD_PORT_MASK_A		0x10
#define JOYPAD_PORT_MASK_B		0x20
#define JOYPAD_PORT_MASK_C		0x40
#define JOYPAD_PORT_MASK_D		0x80

#define JOYPAD_INIT					0xFF

extern int8_t joypad_port1;
extern int8_t joypad_port2;

#endif /* joypads_h */
