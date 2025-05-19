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
    char clabel; // Control label or 0
    key_state_e state[MAX_BUTTON_CNT];
    char  label[MAX_BUTTON_CNT];
} state_t; // Size is 2*MAX_BUTTON_CNT + 4=  36 Byte (MAX_BUTTON_CNT = 16)

int8_t state_ch2idx(state_t *state, char ch);
void state_init(state_t *state);
void state_reset_label(state_t * state, char ch);
key_state_e state_get_state(state_t * state, char ch);
void state_set_label(state_t * state, char ch);
bool state_propagate(state_t *state, char ch);
bool state_is_same(state_t *last, state_t *this);
bool state_merge(state_t *inState, state_t *outState);
void state_print(state_t *state,  char *title );
void state_copy(state_t *from, state_t *to );

#endif /* INC_STATE_H_ */
