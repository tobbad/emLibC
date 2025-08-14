/*
 * state.c
 *
 *  Created on: Jan 22, 2025
 *      Author: TBA
 */

#include "common.h"
#include "state.h"
#include "assert.h"

#ifdef UNIT_TEST
uint32_t HAL_GetTick(){return 1;};
#endif
char key2char[][4] = {
	"OFF",
	"BLI",
	"ON ",
	"NA ",
};



em_msg  state_init(state_t *state){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    state->dirty = false;
    state->first = 0;
    state->cnt = MAX_STATE_CNT;
    state->clabel.cmd =0;
    memcpy(&state->label, &"0123456789ABCDEF", MAX_STATE_CNT);
    for (uint8_t i=0;i<MAX_STATE_CNT;i++){
    	state_set_key_by_idx(state, i, OFF);
    }
    res = EM_OK;
    return res;
}

em_msg state_reset(state_t * state){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
     for (uint8_t i = 0; i < MAX_STATE_CNT; i++){
    	 state_set_key_by_idx(state, i, OFF);
    }
    state->dirty=false;
    res = EM_OK;
    return res;

}


em_msg state_undirty(state_t * state){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
 	state->dirty=false;
    res = EM_OK;
    return res;
}


em_msg state_set_key_by_idx(state_t * state,uint8_t nr, key_state_e new_state){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
	if (new_state>=STATE_CNT) return res;
    if ((nr<state->first)||(nr>=(state->first+state->cnt))) return res;
	state->state[nr] = new_state;
    res = EM_OK;
    return res;
}


em_msg state_set_key_by_lbl(state_t * state, char lbl, key_state_e new_state){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    uint8_t nr = state_ch2idx(state, lbl);
    if (nr<0) return res;
    if ((nr>=state->first)&&(nr<=state->cnt)){
        if (state->state[nr]!=new_state){
            state->state[nr] = new_state;
            state->dirty=true;
            printf("Set %c to %d\n", lbl, new_state);
        }
    }
    res = EM_OK;
    return res;
};


key_state_e state_get_key_by_lbl(state_t * state, char ch){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    int8_t idx = state_ch2idx(state, ch );
    if (idx<0) return res;
    res =(state->state[idx]&(0x03));
    return res;
}

key_state_e state_get_key_by_idx(state_t * state, uint8_t idx){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    if ((idx<state->first)||(idx>=(state->first+state->cnt))) return res;
    res =(state->state[idx]&(0x03));
    return res;
}

em_msg state_propagate_by_lbl(state_t *state, char ch){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    uint8_t idx = state_ch2idx(state, ch);
	if (idx == EM_ERR) return res;
	state->state[idx] = (state->state[idx]+1)%STATE_CNT;
	state->dirty=true;
	res =state->dirty;
    return res;

}
em_msg state_propagate_by_idx(state_t *state, uint8_t idx){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    if ((idx<state->first)||(idx>=(state->first+state->cnt))) return res;
    state->state[idx] = (state->state[idx]+1)%STATE_CNT;
    state->dirty=true;
    res =state->dirty;
    return res;
}

int8_t state_ch2idx(state_t *state, char ch){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
	for (uint8_t idx=state->first;idx<state->first+state->cnt;idx++){
		if (ch==state->label[idx]){
			return idx;
		}
	}
	//printf("'%c' -> Error\n", ch);
	return EM_ERR;
}

em_msg state_set_u32(state_t * state, uint32_t u32){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
	uint32_t mask;
	state_init(state);
    for (uint8_t i=0;i<MAX_STATE_CNT;i++){
    	if ((i>state->first)&&(i<=state->first+state->cnt)){
			uint8_t shift=2*i ;
			mask = (0x3<<shift);
			uint32_t x = (u32&mask);
			x >>= shift;
			state->state[i] = x;
    	}
    }
    res = EM_OK;
    return res;

}

uint32_t state_get_u32(state_t * state){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    res =0;
    uint32_t u32state=0;
    for (uint8_t i=0;i<MAX_STATE_CNT;i++){
    	if ((i>=state->first)&&(i<=state->first+state->cnt)){
			u32state = (state->state[i]&0x3);
			res |= ((u32state)<<(2*i));
    	}
        //printf("nr %02d, state= x%02x, %s), res= 0x%08x\n", i, u32state,  key2char[u32state], res);
    }
    return res;
}
em_msg state_copy(state_t *from, state_t *to){
    em_msg res =EM_ERR;
    if (from==NULL) return res;
    if (to==NULL) return res;
    memcpy(to, from, sizeof(state_t));
    res = EM_OK;
    return res;
}

em_msg state_is_same(state_t *last, state_t *this){
	em_msg res =EM_ERR;
	if (last==NULL) return res;
	if (this==NULL) return res;
	res = true;
    for (uint8_t i=this->first;i<this->first+this->cnt;i++){
    	res &= (last->state[i]==this->state[i]);
    }
    return res;
}

em_msg state_merge(state_t *inState, state_t *outState){
	em_msg res =EM_ERR;
	if (inState==NULL) return res;
	if (outState==NULL) return res;
	if (inState->cnt != outState->cnt) return res;
    if (inState->dirty==0) return res;
    //printf("inState.cnt: %d, outState.cnt: %d"NL, inState->cnt,outState->cnt);
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

em_msg state_print(state_t *state,  char *title ){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    if (title!=NULL){
        printf("%s"NL, title);
    }
    if ((state->first>16) ||(state->cnt>16)){
        printf("Do not print corrupted payload"NL);
        return res;
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
    for (uint8_t i = 0; i<MAX_STATE_CNT;i++){
        printf("%01x", state->state[i]);
    }
    printf(NL);
    (state->dirty&&0x01) ? printf("Dirty"NL): printf("Not Dirty"NL);
    if ((state->dirty>>6)==1){
        printf("clable is cmd"NL);
    } else if ((state->dirty>>6)==2) {
        printf("clable is str"NL);
    }
    res = EM_OK;
    return res;

}

em_msg state_get_dirty(state_t *state ){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
	return state->dirty;
}
em_msg    state_set_dirty(state_t *state ){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
	state->dirty=true;
}

uint8_t state_get_cnt(state_t *state) {
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    return state->cnt;
};
uint8_t state_get_first(state_t *state){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    res = state->first;
    return res;

};
em_msg state_set_cnt(state_t *state, uint8_t nr){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    state->cnt=nr;
    res = EM_OK;
     return res;
};

em_msg state_set_first(state_t *state, uint8_t nr){
	em_msg res =EM_ERR;
	if (state==NULL) return res;
    state->first=nr;
    res = EM_OK;
     return res;

};

