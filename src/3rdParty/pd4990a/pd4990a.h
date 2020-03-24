/*
 *	Header file for the PD4990A Serial I/O calendar & clock.
 */

#include <stdint.h>

void pd4990a_init(void);
void pd4990a_addretrace(void);				// To be call at 60Hz
int pd4990a_read_testbit(void);				// TP out
int pd4990a_read_databit(void);				// Data out
void pd4990a_write_control(uint8_t data);	// C2 C1 C0 command
void pd4990a_increment_day(void);
void pd4990a_increment_month(void);
