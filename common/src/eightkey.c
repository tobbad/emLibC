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

static xpad_pins_t _eight_p = {
		.spalten_cnt = 0,
		.spalte = {{}},
		.zeilen_cnt = 8,
		.zeile = {
			{ .port = GPIOA, .pin = GPIO_PIN_0,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOA, .pin = GPIO_PIN_4,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOB, .pin = GPIO_PIN_0,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOC, .pin = GPIO_PIN_1,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOB, .pin = GPIO_PIN_10, .conf = { .Mode = GPIO_MODE_INPUT, .Pull =	GPIO_PULLUP } },
			{ .port = GPIOB, .pin = GPIO_PIN_5,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOB, .pin = GPIO_PIN_3,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOA, .pin = GPIO_PIN_10, .conf = { .Mode = GPIO_MODE_INPUT, .Pull =	GPIO_PULLUP } },
		},
		.dev = EIGHTKEY,
};

xpad_t* _peight[KYBD_CNT];
xpad_t _eight[KYBD_CNT];
mkey_t reset_key ={0,0,0, true};
void eight_init(kybdh_t dev, xpad_pins_t *pins){
	_peight[dev] = &_eight[dev];
	if (pins != NULL){
		_eight[dev].pins = pins;
	} else {
		_eight[dev].pins = &_eight_p;
	}
	for (uint8_t idx= 0; idx<EIGHT_BUTTON_CNT; idx++){
		memcpy(_eight[dev].key, &reset_key, sizeof(key_state_e));
		if (idx<_eight[dev].key_cnt){
			_eight[dev].value[idx] = idx;
			_eight[dev].state[idx] = OFF;
		}else {
			_eight[dev].value[idx] = -1;
			_eight[dev].state[idx] = -1;
		}

	}
}

uint16_t eight_scan(kybdh_t dev){
	if (dev==0){
		printf("%010ld: No valid handle on eight_scan"NL,HAL_GetTick());
		return false;
	}
	uint16_t res=0;
	for (uint8_t index=0;index<EIGHT_BUTTON_CNT; index++)	{
		bool pin=0;
		GpioPinRead(&_eight[dev].pins->zeile[index], &pin);
		pin =!pin;
		_eight[dev].key[index].current=pin;
		bool this = _eight[dev].key[index].current^_eight[dev].key[index].last;
		if (this){
			_eight[dev].key[index].cnt=0;
		}
		_eight[dev].key[index].unstable =pin || _eight[dev].key[index].unstable;
		if (_eight[dev].key[index].unstable){
			_eight[dev].key[index].cnt++;
		}
		if (_eight[dev].key[index].cnt>STABLE_CNT){
            if (index<_eight[dev].first) continue;
            if (index>_eight[dev].first+_eight[dev].key_cnt) continue;
            res = res | (pin<<index);
			_eight[dev].key[index].stable = pin;
			_eight[dev].dirty=true;
			if ((pin==false) && (index<_eight[dev].key_cnt)){
				_eight[dev].state[index] = ((_eight[dev].state[index]+1)%KEY_STAT_CNT);
				_eight[dev].dirty |= true;
			}
		}
	}
	return res;
};

void eight_reset(kybdh_t dev, bool hard){
	if (dev==0){
		printf("%010ld: No valid handle on reset"NL,HAL_GetTick());
		return;
	}
	for (uint8_t idx=0; idx<EIGHT_BUTTON_CNT; idx++){
		_eight[dev].key[idx] = reset_key;
	}
	_eight[dev].dirty=  false;

};
void eight_state(kybdh_t dev, kybd_r_t *ret){
	for (uint8_t i=0;i<EIGHT_BUTTON_CNT;i++){
		ret->value[i] = _eight[dev].value[i]+1;
		ret->state[i] = _eight[dev].state[i];
	}
	ret->key_cnt =  _eight[dev].key_cnt;
	return;
};

void  eight_iprint(xpad_t *state, char* start){

};

kybd_t eight_dev_o ={
	.init = &eight_init,
	.scan = &eight_scan,
	.reset = &eight_reset,
	.state = &eight_state,
	.dev_type = EIGHTKEY,
};

