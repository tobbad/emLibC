/*
 * 8key.c
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#include "main.h"

eight_t default_eight ={
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

};
eight_t* my_eight[KYBD_CNT];
mkey_t reset_key ={0,0,0, true};

void eight_init(kybd_h dev, void *data){
	if (data != NULL){
		my_eight[dev] = (eight_t *)data;
	} else {
		my_eight[dev] = &default_eight;
	}
	for (uint8_t idx= 0; idx<BUTTON_CNT; idx++){
		GpioPinInit(&my_eight[dev]->bttn_pin[idx]);
		memcpy(my_eight[dev]->key, &reset_key, sizeof(key_state_e));
		my_eight[dev]->label[idx] = idx;
		my_eight[dev]->state[idx] = OFF;

	}

}
bool eight_scan(kybd_h dev){
	if (my_eight[dev]==NULL){
		printf("%010ld: No valid handle on read_row"NL,HAL_GetTick());
		return false;
	}
	uint8_t res=0;
	for (uint8_t index=0;index<BUTTON_CNT; index++)	{
		bool pin=0;
		GpioPinRead(&my_eight[dev]->bttn_pin[index], &pin);
		pin =!pin;
		res = res | (pin<<index);
		my_eight[dev]->key[index].current=pin;
		bool this = my_eight[dev]->key[index].current^my_eight[dev]->key[index].last;
		if (this){
			my_eight[dev]->key[index].cnt=0;
		}
		my_eight[dev]->key[index].unstable =pin || my_eight[dev]->key[index].unstable;
		if (my_eight[dev]->key[index].unstable){
			my_eight[dev]->key[index].cnt++;
		}
		if (my_eight[dev]->key[index].cnt>STABLE_CNT){
	    my_eight[dev]->key[index].stable = pin;
			my_eight[dev]->dirty=true;
			if (pin==false){
				my_eight[dev]->state[index] = ((my_eight[dev]->state[index]+1)%KEY_STAT_CNT);
			}
		}
	}
	return my_eight[dev]->dirty;
};
void eight_reset(kybd_h dev){
	if (my_eight[dev]==NULL){
		printf("%010ld: No valid handle on reset"NL,HAL_GetTick());
		return;
	}
	my_eight[dev]->dirty=  false;
	for (uint8_t idx=0; idx<BUTTON_CNT; idx++){
		my_eight[dev]->key[idx] = reset_key;
	}

};
kybd_r_t* eight_state(kybd_h dev){
	static kybd_r_t res;
	for (uint8_t i=0;i<BUTTON_CNT;i++){
		res.label[i] = my_eight[dev]->label[i];
		res.state[i] = my_eight[dev]->state[i];
	}
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

