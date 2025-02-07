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
    key_state_e state[MAX_BUTTON_CNT];
    char  label[MAX_BUTTON_CNT];
    bool dirty;
} state_t;

void state_init(state_t *state);
bool state_is_different(state_t *last, state_t *this);
bool state_merge(state_t *inState, state_t *outState);
void state_clear(state_t *state, uint8_t nr);
void state_print(state_t *state,  char *title );
bool state_propagate(state_t *state, uint8_t nr);
void  state_reset(state_t * state, uint8_t nr);
bool state_set(state_t * state, uint8_t nr);
bool state_toggle(state_t * state, uint8_t nr);
void  state_copy(state_t *from, state_t *to );

#endif /* INC_STATE_H_ */
