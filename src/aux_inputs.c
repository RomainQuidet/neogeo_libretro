#include "aux_inputs.h"


/*
 Button are active low
 b0: start p1
 b1: select p1
 b2: start p2
 b3: select p2
 b4-5: memory card inserted if 00
 b6: memory card protected if 1
 b7: 0 = AES, 1 = MVS
 */

uint8_t aux_inputs = 0xBF;

#define P1_START_MASK	0x01
#define P1_SELECT_MASK	0x02
#define P2_START_MASK	0x04
#define P2_SELECT_MASK	0x08

void aux_input_start_player(player_m player, bool pressed) {
	switch (player) {
		case Player1:
			if (pressed == true) {
				aux_inputs &= ~P1_START_MASK;
			} else {
				aux_inputs |= P1_START_MASK;
			}
			break;
		case Player2:
			if (pressed == true) {
				aux_inputs &= ~P2_START_MASK;
			} else {
				aux_inputs |= P2_START_MASK;
			}
			break;
	}
}

bool aux_input_start_player_pressed(player_m player) {
	switch (player) {
		case Player1:
			return (aux_inputs & P1_START_MASK) == 0;
			break;
		case Player2:
			return (aux_inputs & P2_START_MASK) == 0;
			break;
	}
}

void aux_input_select_player(player_m player, bool pressed) {
	switch (player) {
		case Player1:
			if (pressed == true) {
				aux_inputs &= ~P1_SELECT_MASK;
			} else {
				aux_inputs |= P1_SELECT_MASK;
			}
			break;
		case Player2:
			if (pressed == true) {
				aux_inputs &= ~P2_SELECT_MASK;
			} else {
				aux_inputs |= P2_SELECT_MASK;
			}
			break;
	}
}

bool aux_input_select_player_pressed(player_m player) {
	switch (player) {
		case Player1:
			return (aux_inputs & P1_SELECT_MASK) == 0;
			break;
		case Player2:
			return (aux_inputs & P2_SELECT_MASK) == 0;
			break;
	}
}

void aux_input_insert_memory_card(void) {
	aux_inputs &= 0xCF;
}

void aux_input_remove_memory_card(void) {
	aux_inputs |= 0x30;
}

void aux_input_lock_memory_card(void) {
	aux_inputs |= 0x40;
}

void aux_input_unlock_memory_card(void) {
	aux_inputs &= 0xBF;
}

void aux_input_set_hardware(BoardType_m board) {
	switch (board) {
		case AES:
			aux_inputs &= ~0x80;
			break;
		case MVS:
			aux_inputs |= 0x80;
			break;
	}
}
