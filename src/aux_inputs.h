#ifndef aux_inputs_h
#define aux_inputs_h

#include <stdint.h>
#include <stdbool.h>

extern uint8_t aux_inputs;

typedef enum Player {
	Player1,
	Player2
} player_m;

typedef enum BoardType {
	AES,
	MVS
} BoardType_m;

void aux_input_start_player(player_m player, bool pressed);
bool aux_input_start_player_pressed(player_m player);
void aux_input_select_player(player_m player, bool pressed);
bool aux_input_select_player_pressed(player_m player);
void aux_input_insert_memory_card(void);
void aux_input_remove_memory_card(void);
void aux_input_lock_memory_card(void);
void aux_input_unlock_memory_card(void);
void aux_input_set_hardware(BoardType_m hardware);

#endif /* aux_inputs_h */
