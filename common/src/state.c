/*
 * state.c
 *
 *  Created on: Jan 22, 2025
 *      Author: TBA
 */

#include "common.h"
#include "state.h"

void  state_init(state_t *state){
    memcpy(state->label, "123456789ABCDEF", MAX_BUTTON_CNT);
    for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
        state->state[i] = OFF;
    }
}

bool state_is_different(state_t *last, state_t *this){
    bool dirty= true;
    for (uint8_t i=last->first;i<last->first+state->cnt;i++){
        dirty &= (last->state[i]==this->state[i]);
    }
    return dirty;
}

bool state_merge(state_t *inState, state_t *outState){
    memset(outState->state, OFF, MAX_BUTTON_CNT);
    for (uint8_t nr=inState->first; nr<inState->first+inState->cnt+1;nr++){
        if (inState->state[nr]!=outState->state[nr]) {
            outState->dirty=true;
            outState->state[nr] = inState->state[nr];
        }
    }
    return outState->dirty;

}

void state_clear(state_t *state, uint8_t nr){
    assert(nr<MAX_BUTTON_CNT+1);
    nr--;
    if (state->state[nr] != OFF){
        state->state[nr] = OFF;
        state->dirty=true;
    }
    state_print(state, "Clear");
}

void state_print(state_t *state,  char *title ){
    if (title!=NULL){
        printf("%08ld: State %s on"NL, HAL_GetTick(), title);
    }
    printf("Label: ");
    for (uint8_t i = state->first; i<state->first+state->cnt; i++){
        printf("%01x", i);
    }
    printf(NL"Label: ");
    for (uint8_t i = state->first; i<state->first+state->cnt;i++){
        printf("%01x", state->state[i]);
    }
    printf(NL);
    (state->dirty) ? printf("%08ld: Dirty"NL, HAL_GetTick()): printf("%08ld: Not Dirty"NL, HAL_GetTick());
}

bool state_propagate(state_t *state, uint8_t nr){
    if (nr<1) {
        (void)0; //NOP
    } if ((nr<1)||(nr>=MAX_BUTTON_CNT+1)){
        printf("%08ld: Cannot propagate key %02x"NL, HAL_GetTick(),nr);
        return false;
    };
    state->state[nr] = (state->state[nr]+1)%KEY_STAT_CNT;
    state->dirty=true;
    state_print(state, "Propagate");
    return true;

}
void state_reset(state_t * state, uint8_t nr){
    assert(nr<MAX_BUTTON_CNT+1);
    nr--;
    if (state->state[nr]!=OFF){
        state->state[nr] = OFF;
        state->dirty=true;
    }
    return;

};
bool state_set(state_t * state, uint8_t nr){
    bool dirty = false;
    assert(nr<MAX_BUTTON_CNT+1);
    nr--;
    if (state->state[nr]!=ON){
        state->state[nr] = ON;
        state->dirty=true;
        dirty=true;
    }
    return dirty;
}
bool state_toggle(state_t * state, uint8_t nr){
    assert(nr<MAX_BUTTON_CNT+1);
    nr--;
    state->state[nr]= !state->state[nr];
    return true;
}
