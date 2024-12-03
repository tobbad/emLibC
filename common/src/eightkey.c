/*
 * 8key.c
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#include "common.h"
#include "keyboard.h"
#include "eightkey.h"
#include "xpad.h"

eight_t default_eight ={
	.bttn_pin={ //Inputs
		{.port = GPIOA,  .pin=GPIO_PIN_0 },
		{.port = GPIOA,  .pin=GPIO_PIN_4 },
		{.port = GPIOB,  .pin=GPIO_PIN_0 },
		{.port = GPIOC,  .pin=GPIO_PIN_1 },
		{.port = GPIOB,  .pin=GPIO_PIN_10},
		{.port = GPIOB,  .pin=GPIO_PIN_5 },
		{.port = GPIOB,  .pin=GPIO_PIN_3 },
		{.port = GPIOA,  .pin=GPIO_PIN_10},
	},
	.key = {
	        { false, false, false, 0, true }, //1
	        { false, false, false, 0, true }, //2
	        { false, false, false, 0, true },//3
	        { false, false, false, 0, true },//4
	        { false, false, false, 0, true },//5
	        { false, false, false, 0, true },//6
	        { false, false, false, 0, true },//6
	        { false, false, false, 0, true }//8
	 },
	.state = {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},
	.value = {1, 2, 3, 4,  5, 6, 7, 8 },
	.dirty = false,
	.key_cnt =EIGHT_BUTTON_CNT,
	.first =0,

};
eight_t* _eight[KYBD_CNT];
mkey_t reset_key ={0,0,0, true};

void eight_init(kybd_h dev, void *data){
	if (data != NULL){
		_eight[dev] = (eight_t *)data;
	} else {
		_eight[dev] = &default_eight;
	}
	for (uint8_t idx= 0; idx<BUTTON_CNT; idx++){
		memcpy(_eight[dev]->key, &reset_key, sizeof(key_state_e));
		if (idx<_eight[dev]->key_cnt){
			_eight[dev]->value[idx] = idx;
			_eight[dev]->state[idx] = OFF;
		}else {
			_eight[dev]->value[idx] = -1;
			_eight[dev]->state[idx] = -1;
		}

	}
}

uint16_t eight_scan(kybd_h dev){
	if (_eight[dev]==NULL){
		printf("%010ld: No valid handle on read_row"NL,HAL_GetTick());
		return false;
	}
	uint16_t res=0;
	for (uint8_t index=0;index<EIGHT_BUTTON_CNT; index++)	{
		bool pin=0;
		GpioPinRead(&_eight[dev]->bttn_pin[index], &pin);
		pin =!pin;
		_eight[dev]->key[index].current=pin;
		bool this = _eight[dev]->key[index].current^_eight[dev]->key[index].last;
		if (this){
			_eight[dev]->key[index].cnt=0;
		}
		_eight[dev]->key[index].unstable =pin || _eight[dev]->key[index].unstable;
		if (_eight[dev]->key[index].unstable){
			_eight[dev]->key[index].cnt++;
		}
		if (_eight[dev]->key[index].cnt>STABLE_CNT){
            if (index<_eight[dev]->first) continue;
            if (index>_eight[dev]->first+_eight[dev]->key_cnt) continue;
            res = res | (pin<<index);
			_eight[dev]->key[index].stable = pin;
			_eight[dev]->dirty=true;
			if ((pin==false) && (index<_eight[dev]->key_cnt)){
				_eight[dev]->state[index] = ((_eight[dev]->state[index]+1)%KEY_STAT_CNT);
				_eight[dev]->dirty |= true;
			}
		}
	}
	return res;
};

void eight_reset(kybd_h dev, bool hard){
	if (_eight[dev]==NULL){
		printf("%010ld: No valid handle on reset"NL,HAL_GetTick());
		return;
	}
	for (uint8_t idx=0; idx<EIGHT_BUTTON_CNT; idx++){
		_eight[dev]->key[idx] = reset_key;
	}
	_eight[dev]->dirty=  false;

};
void eight_state(kybd_h dev, kybd_r_t *ret){
	for (uint8_t i=0;i<EIGHT_BUTTON_CNT;i++){
		ret->value[i] = _eight[dev]->value[i]+1;
		ret->state[i] = _eight[dev]->state[i];
	}
	ret->key_cnt =  _eight[dev]->key_cnt;
	return;
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

