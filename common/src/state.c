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
char key2char[][4] = { "OFF", "BLI", "ON ", "NA ", };

em_msg state_init(state_t *state) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	memset(state, 0, sizeof(state_t));
	state->dirty = false;
	state->first = 0;
	state->cnt = MAX_STATE_CNT;
	state->clabel.cmd = 0;
	memcpy(&state->label, &"0123456789ABCDEF", MAX_STATE_CNT);
	for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
		state_set_key_by_idx(state, i, OFF);
	}
	res = EM_OK;
	return res;
}

em_msg state_reset(state_t *state) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
		state_set_key_by_idx(state, i, OFF);
	}
	state->clabel.cmd = 0;
	state->dirty = false;
	res = EM_OK;
	return res;

}

em_msg state_check(state_t *state) {
	em_msg res = EM_ERR;
	if (state == NULL) return res;
	if (state->first > MAX_STATE_CNT) return res;
	if (state->cnt > MAX_STATE_CNT) return res;
	if (state->first > state->cnt) return res;
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

key_state_e state_key_diff(key_state_e state1, key_state_e state2){
    if (state1 == state2) return OFF;
    if (state2 > state1) return (state2-state1);
    return (state2+STATE_CNT-state1); //state2 < state1
}

em_msg state_set_key_by_idx(state_t *state, uint8_t nr, key_state_e new_state) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	if (new_state >= STATE_CNT)
		return res;
	if ((nr < state->first) || (nr >= (state->first + state->cnt)))
		return res;
	state->state[nr] = new_state;
	res = EM_OK;
	return res;
}

em_msg state_set_key_by_lbl(state_t *state, char lbl, key_state_e new_state) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	uint8_t nr = state_ch2idx(state, lbl);
	if (nr < 0)
		return res;
	if ((nr >= state->first) && (nr <= state->cnt)) {
		if (state->state[nr] != new_state) {
			state->state[nr] = new_state;
			state->dirty = true;
			printf("Set %c to %d\n", lbl, new_state);
		}
	}
	res = EM_OK;
	return res;
}


key_state_e state_get_key_by_lbl(state_t *state, char ch) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	int8_t idx = state_ch2idx(state, ch);
	if (idx < 0)
		return res;
	res = (state->state[idx] & (0x03));
	return res;
}

key_state_e state_get_key_by_idx(state_t *state, uint8_t idx) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	if ((idx < state->first) || (idx >= (state->first + state->cnt)))
		return res;
	res = (state->state[idx] & (0x03));
	return res;
}

em_msg state_propagate_by_lbl(state_t *state, char ch) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	uint8_t idx = state_ch2idx(state, ch);
	if (idx == EM_ERR)
		return res;
	state->state[idx] = (state->state[idx] + 1) % STATE_CNT;
	state->dirty = true;
	res = state->dirty;
	return res;

}
em_msg state_propagate_by_idx(state_t *state, uint8_t idx) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	if ((idx < state->first) || (idx >= (state->first + state->cnt)))
		return res;
	state->state[idx] = (state->state[idx] + 1) % STATE_CNT;
	state->dirty = true;
	res = state->dirty;
	return res;
}

int8_t state_ch2idx(state_t *state, char ch) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	for (uint8_t idx = state->first; idx < state->first + state->cnt; idx++) {
		if (ch == state->label[idx]) {
			return idx;
		}
	}
	//printf("'%c' -> Error\n", ch);
	return EM_ERR;
}

em_msg state_set_u32(state_t *state, uint32_t u32) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	uint32_t mask;
	state_init(state);
	for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
		if ((i > state->first) && (i <= state->first + state->cnt)) {
			uint8_t shift = 2 * i;
			mask = (0x3 << shift);
			uint32_t x = (u32 & mask);
			x >>= shift;
			state->state[i] = x;
		}
	}
	res = EM_OK;
	return res;

}

uint32_t state_get_u32(state_t *state) {
	em_msg res = EM_ERR;
	if (state == NULL)
		return res;
	res = 0;
	uint32_t u32state = 0;
	for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
		if ((i >= state->first) && (i <= state->first + state->cnt)) {
			u32state = (state->state[i] & 0x3);
			res |= ((u32state) << (2 * i));
		}
		//printf("nr %02d, state= x%02x, %s), res= 0x%08x\n", i, u32state,  key2char[u32state], res);
	}
	return res;
}
em_msg state_copy(state_t *from, state_t *to) {
	em_msg res = EM_ERR;
	if (state_check(from))
		return res;
	if (state_check(to))
		return res;
	memcpy(to, from, sizeof(state_t));
	res = EM_OK;
	return res;
}

em_msg state_add(state_t *ref, state_t *add) {
    em_msg res = EM_ERR;
    if (state_check(ref)) return res;
    if (state_check(add)) return res;
    ref->dirty = false;
    if (ref->clabel.cmd != add->clabel.cmd) {
        ref->clabel.cmd = add->clabel.cmd;
        ref->dirty = true;
    }
    for (uint8_t i = ref->first;i < ref->first + ref->cnt; i++) {
        if (add->state[i]==BLINKING) {
            state_propagate_by_idx(ref, i);
        }else if (add->state[i]==ON){
            state_propagate_by_idx(ref, i);
            state_propagate_by_idx(ref, i);
        }
    }
    return ref->dirty;

}

em_msg state_is_same(state_t *last, state_t *this){
	em_msg res =EM_ERR;
    if (state_check(last)) return res;
    if (state_check(this)) return res;
	res = true;
    for (uint8_t i=this->first;i<this->first+this->cnt;i++){
    	res &= (last->state[i]==this->state[i]);
    }
    return res;
}

em_msg state_diff(state_t *ref, state_t *state, state_t *diff) {
    em_msg res = EM_ERR;
    if (state_check(ref)) return res;
    if (state_check(state)) return res;
    if (state_check(diff)) return res;
    //printf("inState.cnt: %d, outState.cnt: %d"NL, inState->cnt,outState->cnt);
    diff->dirty = false;
    if (ref->clabel.cmd != state->clabel.cmd) {
        ref->clabel.cmd = state->clabel.cmd;
        diff->clabel.cmd = state->clabel.cmd;
        diff->dirty = true;
    }
    for (uint8_t ri = ref->first, si = state->first;
            ri < ref->first + ref->cnt,si < state->first + state->cnt;
            ri++,si++) {
        if (ref->state[ri] != state->state[si]) {
            diff->dirty = true;
            diff->state[ri] = state_key_diff(ref->state[ri], state->state[si]);
        }
    }
    return diff->dirty;

}
em_msg state_merge(state_t *inState, state_t *outState){
	em_msg res =EM_ERR;
	if (state_check(inState)) return res;
	if (state_check(outState)) return res;
	//printf("inState.cnt: %d, outState.cnt: %d"NL, inState->cnt,outState->cnt);
	outState->dirty = false;
	if (outState->clabel.cmd != inState->clabel.cmd) {
		outState->clabel.cmd = inState->clabel.cmd;
		outState->dirty = true;
	}
	for (uint8_t i = inState->first;i < inState->first + inState->cnt; i++) {
		if (inState->state[i] != outState->state[i]) {
			outState->dirty = true;
			outState->state[i] = inState->state[i];
		}
	}
	return outState->dirty;
}

em_msg state_print(state_t *state, char *title) {
	em_msg res = EM_ERR;
	if (state_check(state))
		return res;
	if (title != NULL) {
		printf("%s"NL, title);
	}
	if ((state->first > 16) || (state->cnt > 16)) {
		printf("Do not print corrupted payload"NL);
		return res;
	}
	printf("first : %d"NL, state->first);
	printf("cnt   : %d"NL, state->cnt);
	printf("clabel: 0x%08lx"NL, state->clabel.cmd);
	printf("label : ");
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		char c = state->label[i];
		if (isprint(c)) {
			printf(" %c  ", state->label[i]);
		} else {
			printf("....");
		}
	}
	printf(NL"State : ");
	for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
		printf("%s ", key2char[state->state[i]]);
	}
	printf(NL);
	(state->dirty && 0x01) ? printf("Dirty"NL) : printf("Not Dirty"NL);
	if ((state->dirty >> 6) == 1) {
		printf("clable is cmd"NL);
	} else if ((state->dirty >> 6) == 2) {
		printf("clable is str"NL);
	}
	res = EM_OK;
	return res;

}

em_msg state_get_dirty(state_t *state) {
	em_msg res = EM_ERR;
	if (state_check(state))
		return res;
	return state->dirty;
}

em_msg state_set_dirty(state_t *state) {
    em_msg res = EM_ERR;
    state->dirty = true;
    res = EM_OK;
    return res;
}

uint8_t state_get_cnt(state_t *state) {
	em_msg res = EM_ERR;
    if (state_check(state)) return res;
	return state->cnt;
}
;
uint8_t state_get_first(state_t *state) {
	em_msg res = EM_ERR;
	res = state->first;
	return res;

}
;
em_msg state_set_cnt(state_t *state, uint8_t nr) {
	em_msg res = EM_ERR;
    if (state_check(state)) return res;
	state->cnt = nr;
	res = EM_OK;
	return res;
}

em_msg state_set_first(state_t *state, uint8_t nr) {
	em_msg res = EM_ERR;
	if (state_check(state))
		return res;
	state->first = nr;
	res = EM_OK;
	return res;
}

