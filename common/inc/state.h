/*
 * state.h
 *
 *  Created on: Jan 22, 2025
 *      Author: TBA
 */

#ifndef INC_STATE_H_
#define INC_STATE_H_
#include "common.h"

typedef enum{
    OFF,
    BLINKING,
    ON,
    KEY_STAT_CNT
}key_state_e;

typedef struct state_s{
    uint8_t first;
    uint8_t cnt;
    bool dirty;
	uint8_t cstate;// state of the label= true/false
    key_state_e state[MAX_BUTTON_CNT];
    char  label[MAX_BUTTON_CNT];
    char  clabel[MAX_BUTTON_CNT/2]; // Control label
} state_t; // Size is 2*MAX_BUTTON_CNT + MAX_BUTTON_CNT/2 + 4=  44 Byte

void state_clear(state_t *state);
void state_reset_key(state_t * state, uint8_t nr);
bool state_propagate(state_t *state, uint8_t nr);
bool state_is_same(state_t *last, state_t *this);
bool state_merge(state_t *inState, state_t *outState);
void state_print(state_t *state,  char *title );
void state_copy(state_t *from, state_t *to );

#endif /* INC_STATE_H_ */
