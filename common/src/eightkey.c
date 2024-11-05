/*
 * 8key.c
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#include "main.h"

eight_t _eight ={
	.bttn_pin={ //Inputs
		{.port = PORTB, .pin = PIN_10, .conf = {.mode=INPUT, .pin =PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port = PORTB, .pin = PIN_4,  .conf = {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port = PORTB, .pin = PIN_5,  .conf = {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port = PORTB, .pin = PIN_3,  .conf = {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port = PORTA, .pin = PIN_0,  .conf = {.mode=INPUT, .pin=PIN_OD,   .speed=s_HIGH,  .pupd = PULL_UP}},
		{.port = PORTA, .pin = PIN_1,  .conf = {.mode=INPUT, .pin=PIN_OD,   .speed=s_HIGH,  .pupd = PULL_UP}},
		{.port = PORTA, .pin = PIN_4,  .conf = {.mode=INPUT, .pin=PIN_OD,   .speed=s_HIGH,  .pupd = PULL_UP}},
		{.port = PORTB, .pin = PIN_0,  .conf = {.mode=INPUT, .pin=PIN_OD,   .speed=s_HIGH,  .pupd = PULL_UP}},
	},
	.key = { {false,false ,0,false}, {false,false,0,false}, {false,false,0,false}, {false,false,0,false}, {false,false,0,false},  {false,false,0,false},  {false,false,0,false},  {false,false,0,false}},
	.state = {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF,},
	.label = {0xd, 0xe, 0xf, 0,  0xc, 9, 8, 7 },
	.dirty = false,
	.key_cnt = 8,

};
eight_t* M_eight[KYBD_CNT];
mkey_t reset_key ={0,0,0, true};

void eight_init(kybd_h dev, void *data){
	if (data != NULL){
		M_eight[dev] = (eight_t *)data;
	} else {
		M_eight[dev] = &_eight;
	}
	for (uint8_t idx= 0; idx<BUTTON_CNT; idx++){
		GpioPinInit(&M_eight[dev]->bttn_pin[idx]);
		memcpy(M_eight[dev]->key, &reset_key, sizeof(key_state_e));
		if (idx<M_eight[dev]->key_cnt){
			M_eight[dev]->label[idx] = idx;
			M_eight[dev]->state[idx] = OFF;
		}else {
			M_eight[dev]->label[idx] = -1;
			M_eight[dev]->state[idx] = -1;
		}

	}

}
bool eight_scan(kybd_h dev){
	if (M_eight[dev]==NULL){
		printf("%010ld: No valid handle on read_row"NL,HAL_GetTick());
		return false;
	}
	uint8_t res=0;
	for (uint8_t index=0;index<BUTTON_CNT; index++)	{
		bool pin=0;
		GpioPinRead(&M_eight[dev]->bttn_pin[index], &pin);
		pin =!pin;
		res = res | (pin<<index);
		M_eight[dev]->key[index].current=pin;
		bool this = M_eight[dev]->key[index].current^M_eight[dev]->key[index].last;
		if (this){
			M_eight[dev]->key[index].cnt=0;
		}
		M_eight[dev]->key[index].unstable =pin || M_eight[dev]->key[index].unstable;
		if (M_eight[dev]->key[index].unstable){
			M_eight[dev]->key[index].cnt++;
		}
		if (M_eight[dev]->key[index].cnt>STABLE_CNT){
	    M_eight[dev]->key[index].stable = pin;
			M_eight[dev]->dirty=true;
			if ((pin==false) && (index<M_eight[dev]->key_cnt)){
				M_eight[dev]->state[index] = ((M_eight[dev]->state[index]+1)%KEY_STAT_CNT);
				M_eight[dev]->dirty |= true;
			}
		}
	}
	return M_eight[dev]->dirty;
};
void eight_reset(kybd_h dev){
	if (M_eight[dev]==NULL){
		printf("%010ld: No valid handle on reset"NL,HAL_GetTick());
		return;
	}
	M_eight[dev]->dirty=  false;
	for (uint8_t idx=0; idx<BUTTON_CNT; idx++){
		M_eight[dev]->key[idx] = reset_key;
	}

};
kybd_r_t* eight_state(kybd_h dev){
	static kybd_r_t res;
	for (uint8_t i=0;i<BUTTON_CNT;i++){
		res.label[i] = M_eight[dev]->label[i]+1;
		res.state[i] = M_eight[dev]->state[i];
	}
	res.key_cnt =  M_eight[dev]->key_cnt;
	return &res;
};
void  eight_iprint(xpad_t *state, char* start){

};

kybd_t eight_dev ={
	.init = &eight_init,
	.scan = &eight_scan,
	.reset = &eight_reset,
	.state = &eight_state,
	.dev_type = EIGHTKEY,
};

