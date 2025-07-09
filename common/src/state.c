/*
 * state.c
 *
 *  Created on: Jan 22, 2025
 *      Author: TBA
 */

#include "common.h"
#include "state.h"

int8_t clable2type(clabel_u *lbl){
    int8_t res=-1;
    char *stopstring = NULL;
    lbl->str[CMD_LEN-1]=0;
    res = strtol(lbl->str, &stopstring, 10);
    if (strlen(stopstring)==0) {
        lbl->cmd = res;
        res= ISNUM;
    }
    bool itIs=false;
    for (uint8_t i=0;i<CMD_LEN;i++){
        itIs &= isascii(lbl->str[i]);
    }
    if (itIs){
        res = ISASCISTR;
    }
    return res;
}


void  state_init(state_t *state){
    state->dirty = false;
    state->first = 0;
    state->cnt = MAX_BUTTON_CNT;
    state->clabel.cmd =0;
    memcpy(&state->label, &"0123456789ABCDEF", MAX_BUTTON_CNT);
    for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
    	state_set_index(state, i, OFF);
    }

}

void state_clear(state_t * state){
    for (uint8_t nr=state->first;nr<=state->first+state->cnt;nr++){
    	state_set_index(state, nr, OFF);
    }
    state->dirty=false;
}


void state_undirty(state_t * state){
    if (state!=NULL){
        state->dirty=false;
    }
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
        state->clabel.str[0] = toupper(ch);
        return;
    }
    uint8_t nr = state_ch2idx(state, ch);
    if (nr<0) return;
    if ((nr>=state->first)&&(nr<=state->cnt)){
        if (state->state[nr]!=new_state){
            state->state[nr] = new_state;
            state->dirty=true;
        }
    }
    return;
};

void state_set_index(state_t * state, uint8_t  nr, key_state_e new_state){
    if (nr<0) return;
    if (nr>=MAX_BUTTON_CNT) return;
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
        printf("Cannot propagate key %c"NL, ch);
        return false;
    };
    state->state[nr] = (state->state[nr]+1)%STATE_CNT;
    state->dirty=true;
    return true;

}
bool state_propagate_index(state_t *state, uint8_t idx){
  if ((idx<state->first)||(idx>state->first+state->cnt)){
        printf("Cannot propagate key %c"NL, idx);
        return false;
    };
    state->state[idx] = (state->state[idx]+1)%STATE_CNT;
    state->dirty=true;
    return true;

}

key_state_e state_get_state(state_t * state, char ch){
    uint8_t nr = state_ch2idx(state, ch );
    if (nr<0){
        printf("%08ld: Cannot get key %c"NL, HAL_GetTick(),ch);
        return STATE_CNT;
    };
    return state->state[nr];
}

void state_set_state(state_t * state,uint8_t nr, key_state_e ks){
	assert(ks<STATE_CNT);
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
        printf("Payload cnt=(%d, %d) can not be merged"NL,inState->cnt,outState->cnt);
        return false;
    }
    printf("inState.cnt: %d, outState.cnt: %d"NL, inState->cnt,outState->cnt);
    outState->dirty=false;
    if (outState->clabel.cmd!= inState->clabel.cmd){
    	outState->clabel.cmd= inState->clabel.cmd;
    	outState->dirty=true;
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
    if (strlen(state->clabel.str)<CMD_LEN){
    	printf("clabel: %s"NL, state->clabel.str );
    } else{
    	printf("clabel: 0x%08lx"NL, state->clabel.cmd);
    }
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
    (state->dirty&&0x01) ? printf("Dirty"NL): printf("Not Dirty"NL);
    if ((state->dirty>>6)==1){
        printf("clable is cmd"NL);
    } else if ((state->dirty>>6)==2) {
        printf("clable is str"NL);
    }
}

uint8_t state_get_dirty(state_t *state ){
	return state->dirty;
}
void    state_set_dirty(state_t *state ){
	state->dirty=true;
}

uint8_t state_get_cnt(state_t *state) {
    return state->cnt;
};
uint8_t state_get_first(state_t *state){
    return state->first;
};
void state_set_cnt(state_t *state, uint8_t nr){
    state->cnt=nr;
};

void state_set_first(state_t *state, uint8_t nr){
    state->first=nr;
};

