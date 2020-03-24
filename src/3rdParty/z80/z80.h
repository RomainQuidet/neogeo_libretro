#ifndef _Z80_H_
#define _Z80_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Interrupt line constants */
enum
{
	/* line states */
	CLEAR_LINE = 0,				/* clear (a fired, held or pulsed) line */
	ASSERT_LINE,				/* assert an interrupt immediately */
	HOLD_LINE,					/* hold interrupt line until acknowledged */
	PULSE_LINE,					/* pulse interrupt line for one instruction */
	INPUT_LINE_NMI,
};

#define Uint8 uint8_t
#define Sint8 int8_t
#define Uint16 uint16_t
#define Uint32 uint32_t

typedef union
{
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || MINIZ_X86_OR_X64_CPU
	struct { Uint8	l, h, h2, h3; } b;
	struct { Uint16	l, h; } w;
#else
	struct { Uint8 h3, h2, h, l; } b;
	struct { Uint16 h, l; } w;
#endif
	Uint32 d;
} PAIR;

/****************************************************************************/
/* The Z80 registers. HALT is set to 1 when the CPU is halted, the refresh  */
/* register is calculated as follows: refresh=(Z80.r&127)|(Z80.r2&128)      */
/****************************************************************************/
typedef struct
{
	PAIR	prvpc, pc, sp, af, bc, de, hl, ix, iy;
	PAIR	af2, bc2, de2, hl2;
	Uint8	r, r2, iff1, iff2, halt, im, i;
	Uint8	nmi_state;			/* nmi line state */
	Uint8	nmi_pending;		/* nmi pending */
	Uint8	irq_state;			/* irq line state */
	Uint8	after_ei;			/* are we in the EI shadow? */
	const struct z80_irq_daisy_chain *daisy;
	int(*irq_callback)(int irqline);
}	Z80_Regs;

extern Z80_Regs Z80;

void z80_init ( int index, int clock, const void *config, int ( *irqcallback ) ( int ) );
void z80_reset ( void );
void z80_exit ( void );
int  z80_execute ( int cycles );
void z80_set_irq_line ( int irqline, int state );

#ifdef ENABLE_DEBUGGER
unsigned z80_dasm ( char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram );
#endif

/* ======================================================================== */
/* ====================== FUNCTIONS CALLED BY THE CPU ===================== */
/* ======================================================================== */

/* You will have to implement these functions */

extern uint16_t io_read_byte_8(uint16_t port);
extern void io_write_byte_8(uint16_t port, uint16_t value);
extern uint8_t program_read_byte_8(uint16_t addr);
extern void program_write_byte_8(uint16_t addr, uint8_t value);
extern int z80_irq_callback(int parameter);

#ifdef __cplusplus
}
#endif

#endif

