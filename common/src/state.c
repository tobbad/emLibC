/*
 * state.c
 *
 *  Created on: Jan 22, 2025
 *      Author: TBA
 */

#include "common.h"
#include "state.h"


int8_t state_ch2idx(state_t *state, char ch){
    for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
        if (state->label[i]==ch){
            return i;
        }
    }
    return -1;
}


void  state_clear(state_t *state){
    state->dirty = false;
    memcpy(&state->label, &"0123456789ABCDEF", MAX_BUTTON_CNT);
    for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
        state->state[i] = OFF;
    }

}

void state_reset_label(state_t * state, char ch){
    if (isalpha(ch)){
        state->clabel = 0;
        return;
    }
    uint8_t nr = state_ch2idx(state, ch);
    if (nr>=0){
        if (state->state[nr]!=OFF){
            state->state[nr] = OFF;
            state->dirty=true;
        }
    }
    return;
};

void state_set_label(state_t * state, char ch){
    if (isalpha(ch)){
        state->clabel = toupper(ch);
        return;
    }
    uint8_t nr = state_ch2idx(state, ch);
    if (nr>=0){
        if (state->state[nr]==OFF){
            state->state[nr] = ON;
            state->dirty=true;
        }
    }
    return;
};

bool state_propagate(state_t *state, char ch){
    if (isalpha(ch)){
        return false;
    }
    uint8_t nr = state_ch2idx(state, ch);
    if (nr<0){
        printf("%08ld: Cannot propagate key %c"NL, HAL_GetTick(),ch);
        return false;
    };
    state->state[nr] = (state->state[nr]+1)%KEY_STAT_CNT;
    state->dirty=true;
    return true;

}
key_state_e state_get_state(state_t * state, char ch){
    uint8_t nr = state_ch2idx(state, ch);
    if (nr<0){
        printf("%08ld: Cannot get key %c"NL, HAL_GetTick(),ch);
        return KEY_STAT_CNT;
    };
    return state->state[nr];
}


bool state_is_same(state_t *last, state_t *this){
    bool isTheSame= true;
    for (uint8_t i=this->first;i<this->first+this->cnt;i++){
    	isTheSame &= (last->state[i]==this->state[i]);
    }
    return isTheSame;
}

bool state_merge(state_t *inState, state_t *outState){
    assert(outState->cnt == inState->cnt);
    printf("inState.cnt: %d, outState.cnt: %d"NL, inState->cnt,outState->cnt);
    outState->dirty=false;
    if (inState->state[0]!=OFF) {
        outState->clabel=inState->label[0];
        inState->first=0;
    }
    for (uint8_t inr=inState->first,onr=outState->first;
            inr<inState->first+inState->cnt;inr++, onr++){
        if (inState->state[inr]!=outState->state[onr]) {
            outState->dirty=true;
            outState->state[onr] = inState->state[inr];
        }
    }
    return outState->dirty;

}
void  state_copy(state_t *from, state_t *to ){
    memcpy(to, from, sizeof(state_t));

}

void state_print(state_t *state,  char *title ){
    if (title!=NULL){
        printf("%s"NL, title);
    }
    if ((state->first>16) ||(state->cnt>16)){
        printf("Do not print corrupted payload"NL);
        return;
    }
    printf("first : %d"NL, state->first);
    printf("cnt   : %d"NL, state->cnt);
    printf("clabel: 0x%1x"NL, state->clabel);
    printf("Label : ");
    for (uint8_t i = 0; i<MAX_BUTTON_CNT; i++){
        char c = state->label[i];
        if (isprint(c)){
            printf("%c", state->label[i]);
        } else{
            printf(".");
        }
    }
    printf(NL"State : ");
    for (uint8_t i = 0; i<MAX_BUTTON_CNT;i++){
        printf("%01x", state->state[i]);
    }
    printf(NL);
    (state->dirty) ? printf("Dirty"NL): printf("Not Dirty"NL);
}

