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


void  state_init(state_t *state){
    state->dirty = false;
    state->first = 0;
    state->cnt = MAX_BUTTON_CNT;
    state->clabel = 0;
    memcpy(&state->label, &"0123456789ABCDEF", MAX_BUTTON_CNT);
    for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
        state->state[i] = OFF;
    }

}

void state_clear_state(state_t * state, char ch){
    if (isalpha(ch)){
        state->clabel = 0;
        return;
    }
    uint8_t nr = state_ch2idx(state, ch);
    if((nr>=state->first)&&(nr<=state->cnt)){
        if (state->state[nr]!=OFF){
            state->state[nr] = OFF;
            state->dirty=false;
        }
    }
    return;
}

void state_set_value(state_t * state, uint8_t nr, key_state_e new_state ){
    if ((nr>=state->first)&&(nr<=state->cnt)){
        if (state->state[nr]!=new_state){
            state->state[nr] = new_state;
            state->dirty=true;
        }
    }
};


void state_set_label(state_t * state, char ch, key_state_e new_state){
    if (isalpha(ch)){
        state->clabel = toupper(ch);
        return;
    }
    uint8_t nr = state_ch2idx(state, ch);
    if ((nr>=state->first)&&(nr<=state->cnt)){
        if (state->state[nr]!=new_state){
            state->state[nr] = new_state;
            state->dirty=true;
        }
    }
    return;
};

void state_set_u32(state_t * state, uint32_t u32){
	uint32_t mask;
    for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
    	uint8_t shift=2*i ;
    	mask = (0x3<<shift);
    	uint32_t x = (u32&mask);
    	x >>= shift;
        state->state[i] = x;
    }
}

uint32_t state_get_u32(state_t * state){
    uint32_t res =0;
    for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
        res |= (state->state[i]&0x3)<<(2*i);
    }
    return res;
}

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

void state_set_state(state_t * state,uint8_t nr, key_state_e ks){
	assert(ks<KEY_STAT_CNT);
	assert(nr<MAX_BUTTON_CNT);
	if ((nr<state->first)||(nr>=state->first+state->cnt)){
		printf("Try to modify unused state %d"NL, nr);
		return;
	}
	state->state[nr] = ks;

}


bool state_is_same(state_t *last, state_t *this){
    bool isTheSame= true;
    for (uint8_t i=this->first;i<this->first+this->cnt;i++){
    	isTheSame &= (last->state[i]==this->state[i]);
    }
    return isTheSame;
}

bool state_merge(state_t *inState, state_t *outState){
    if (inState->cnt != outState->cnt){
        printf("Payload cnt=(%d, %d) can not be merged"NL,inState->cnt,inState->cnt);
        return false;
    }
    printf("inState.cnt: %d, outState.cnt: %d"NL, inState->cnt,outState->cnt);
    outState->dirty=false;
    if (inState->state[0]!=OFF) {
        outState->clabel=inState->label[0];
        inState->first=0;
    }
    // FIXME
    for (uint8_t inr=inState->first,onr=outState->first;
            inr<inState->first+inState->cnt;inr++, onr++){
        // FIXME
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

uint8_t state_cnt(state_t *state){
	return state->cnt;
}

uint8_t state_first(state_t *state){
	return state->first;
}

uint8_t state_last(state_t *state){
	return state->first+state->cnt-1;
}

uint8_t state_get_dirty(state_t *state, uint8_t nr ){
	return state->state[nr];
}
void    state_set_dirty(state_t *state, uint8_t nr ){
	state->dirty=true;
}

