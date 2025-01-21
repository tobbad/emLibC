/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "common.h"
#include "device.h"
#include "gpio.h"
#include "gpio_port.h"
#include "keyboard.h"
#include "xpad.h"

#define ZEI_SPA_2_INDEX(zeile, spalte ) (uint8_t)(spalte*ZEILEN_CNT+zeile)
#define INDEX_2_ZEI_SPA(index) (  ((uint8_t)(index%ZEILEN_CNT)), ((uint8_t)(index-(index%ZEILEN_CNT)*ZEILEN_CNT) ))
#define INDEX_2_ZEI(index)  ((uint8_t)(index%ZEILEN_CNT))
#define INDEX_2_SPA(index) ((uint8_t)(((index - INDEX_2_ZEI(index)))%ZEILEN_CNT))
#define MINIMAL_LINESTART 16

xpad_t my_xpad[DEVICE_CNT];
xpad_t *mpy_xpad[DEVICE_CNT];



static xpad_dev_t default_xscan_dev= {
	.spalte= {// Output
		.pin={
			{	.port=GPIOA, .pin=GPIO_PIN_0, .conf= {.Mode=GPIO_MODE_OUTPUT_PP, .Pull=GPIO_PULLUP,}}, // spalte 1
			{	.port=GPIOA, .pin=GPIO_PIN_4, .conf= {.Mode=GPIO_MODE_OUTPUT_PP, .Pull=GPIO_PULLUP,}}, // spalte 2
			{	.port=GPIOB, .pin=GPIO_PIN_0, .conf= {.Mode=GPIO_MODE_OUTPUT_PP, .Pull=GPIO_PULLUP,}}, // spalte 3
			{	.port=GPIOC, .pin=GPIO_PIN_1, .conf= {.Mode=GPIO_MODE_OUTPUT_PP, .Pull=GPIO_PULLUP,}}, // spalte 4
		},
		.cnt=4 ,
	},
	.zeile= { // Input
		.cnt=4 ,
		.pin={
			{	.port=GPIOB, .pin= GPIO_PIN_10,  .conf= {.Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP,}}, // zeilen 1
			{	.port=GPIOB, .pin= GPIO_PIN_5 ,  .conf= {.Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP,}}, // zeilen 2
			{	.port=GPIOB, .pin= GPIO_PIN_3 ,  .conf= {.Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP,}}, // zeilen 3
			{	.port=GPIOA, .pin= GPIO_PIN_10,  .conf= {.Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP,}}, // zeilen 4
		},
	},
	.dev_type=XSCAN,
    .state = {.label = {'1', '2', '3', 'a', '4', '5', '6', 'b', '7', '8', '9', 'c', '0', 'f', 'e' ,'d'},
              .state={OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF}},
	.key_cnt=16,
	.first = 0,

};

static xpad_dev_t default_eight_dev = {
	.spalte={{{0}}},
	.zeile ={
		.cnt = 8,
		.pin = {
			{ .port = GPIOB, .pin = GPIO_PIN_10, .conf = { .Mode = GPIO_MODE_INPUT, .Pull =	GPIO_PULLUP } },
			{ .port = GPIOB, .pin = GPIO_PIN_5,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOB, .pin = GPIO_PIN_3,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOA, .pin = GPIO_PIN_10, .conf = { .Mode = GPIO_MODE_INPUT, .Pull =	GPIO_PULLUP } },
			{ .port = GPIOA, .pin = GPIO_PIN_0,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOA, .pin = GPIO_PIN_4,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOB, .pin = GPIO_PIN_0,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
			{ .port = GPIOC, .pin = GPIO_PIN_1,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
		},
	},
	.dev_type=EIGHTKEY,
	.state = {.label = {'1', '2', '3', '4', '5', '6', '7', '8'},
	          .state={OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF}},
	.key_cnt=8,
	.first = 1,

};

static uint8_t lable2uint8(char label){
	uint8_t value = label -'0';
	if (value>9){
		label &= 0xEF;
		value = label-'a'+10;
	}
	return value;

}
static void xpad_set_spalten_pin(dev_handle_t dev, uint8_t spalten_nr) {
	if (mpy_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on xpad_set_spalte"NL, HAL_GetTick());
		return;
	}
	gpio_pin_t *pin = &my_xpad[dev].spalte->pin[spalten_nr];
	GpioPinWrite(pin, GPIO_PIN_SET);

	return;
}

static void xpad_reset_spalten_pin(dev_handle_t dev, uint8_t spalten_nr) {
	if (mpy_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on xpad_set_spalte"NL, HAL_GetTick());
		return;
	}
	gpio_pin_t *pin = &my_xpad[dev].spalte->pin[spalten_nr];
	GpioPinWrite(pin, GPIO_PIN_RESET);
	return;
}

static uint8_t xpad_update_key(uint8_t dev, uint8_t index, bool pinVal) {
	my_xpad[dev].key[index].current = pinVal;
//	if (pinVal) {
//		printf("%010ld: Pushed @ (index= %d)"NL, HAL_GetTick(), index);
//	}
	//uint8_t z,s = INDEX_2_ZEI_SPA(index);
	uint8_t z = INDEX_2_ZEI(index);
	uint8_t s = INDEX_2_SPA(index);
	uint8_t res=0;
//	if (my_xpad[dev].key[index].current ^ my_xpad[dev].key[index].last) {
//		printf("%010ld: Detected Key @ (index =%d)"NL, HAL_GetTick(), index);
//	}
	my_xpad[dev].key[index].unstable = my_xpad[dev].key[index].unstable
			|| (my_xpad[dev].key[index].current ^ my_xpad[dev].key[index].last);

	if (my_xpad[dev].key[index].unstable) {
		if (my_xpad[dev].key[index].current == my_xpad[dev].key[index].last) {
			my_xpad[dev].key[index].cnt++;
		}
	}
	if (my_xpad[dev].key[index].cnt > STABLE_CNT) {
		// We got a stable state
		my_xpad[dev].key[index].unstable = false;
		char label = my_xpad[dev].state.label[index];
		my_xpad[dev].key[index].last = pinVal;
		my_xpad[dev].key[index].stable = pinVal;
		if (pinVal) {
			my_xpad[dev].state.state[index] = ((my_xpad[dev].state.state[index] + 1)
					% KEY_STAT_CNT);
			my_xpad[dev].dirty = true;
			printf("%010ld: Pushed   Key @ (index =%d, z=%d, s=%d, value = %c)"NL, HAL_GetTick(), index, z, s, label);
		} else {
			printf("%010ld: Released Key @ (index =%d, z=%d, s=%d, value = %c)"NL, HAL_GetTick(), index, z , s, label);
		}
		res = res | (pinVal << z);
		my_xpad[dev].key[index].cnt = 0;
	}

	if (my_xpad[dev].key[index].current ^ my_xpad[dev].key[index].last) {
		my_xpad[dev].key[index].cnt = 0;
	}
	my_xpad[dev].key[index].last = my_xpad[dev].key[index].current;
	return res;
}

static uint16_t xpad_eight_scan(dev_handle_t dev) {
	if (mpy_xpad[dev]==NULL) {
		printf("%010ld: No valid handle on read_row"NL,HAL_GetTick());
		return false;
	}
	if (my_xpad[dev].dev_type!=EIGHTKEY) {
		printf("%010ld: Handle for this keyboard not valid"NL,HAL_GetTick());
		return false;
	}
	uint16_t res=0;
	for (uint8_t zeile=0;zeile<my_xpad[dev].zeile->cnt; zeile++) {
		bool pin=0;
		GpioPinRead(&my_xpad[dev].zeile->pin[zeile], &pin);
		pin =!pin;
		res = res|(xpad_update_key(dev, zeile, pin));
	}
	return res;
}
static uint16_t xpad_read_zeile(dev_handle_t dev, uint8_t spalten_nr) {
	if (mpy_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on read_zeile"NL, HAL_GetTick());
		return 0;
	}
	uint8_t res = 0;
	for (int8_t z =0 ; z<ZEILEN_CNT; z++) {
		bool pinVal = 0;
		GpioPinRead(&my_xpad[dev].zeile->pin[z], &pinVal);
		pinVal = !pinVal;
		uint8_t index = ZEI_SPA_2_INDEX(z, spalten_nr);
		res = res | xpad_update_key(dev, index, pinVal);
	}
	return res;
}

static uint16_t xpad_spalten_scan(dev_handle_t dev) {
	if (mpy_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on scan"NL, HAL_GetTick());
		return false;
	}
	if (my_xpad[dev].dev_type != XSCAN) {
		printf("%010ld: Handle (%d) for this keyboard not valid"NL,
				HAL_GetTick(), my_xpad[dev].dev_type);
		return false;
	}
	uint16_t res = 0;
	for (uint8_t s = 0; s < SPALTEN_CNT; s++) {
		uint8_t ir = 0;
		xpad_reset_spalten_pin(dev, s);
		HAL_Delay(SETTLE_TIME_MS);
		ir = xpad_read_zeile(dev, s);
		res = res | (ir << (4 * s));
		xpad_set_spalten_pin(dev, s);
	}
	if (res != 0) {
		printf("%010ld: Key Label is  0x%04lX"NL, HAL_GetTick(), (uint32_t) res);
	}
	return res;
}

static void xpad_init(dev_handle_t dev, dev_type_e dev_type, xpad_t *device) {
	if (dev_type == DEV_TYPE_NA)
		return;
	my_xpad[dev].dev_type= dev_type;
	mpy_xpad[dev] = &my_xpad[dev];
	if (device != NULL) {
		my_xpad[dev].spalte = device->spalte;
		my_xpad[dev].zeile =  device->zeile;
		memcpy(&my_xpad[dev].state, &device->state, sizeof(state_t));
	} else {
		if (dev_type ==XSCAN) {
			my_xpad[dev].spalte = &default_xscan_dev.spalte;
			my_xpad[dev].zeile  = &default_xscan_dev.zeile;
			my_xpad[dev].key_cnt  = default_xscan_dev.key_cnt;
			my_xpad[dev].first  = default_xscan_dev.first;
	        memcpy(&my_xpad[dev].state, &default_xscan_dev.state,  sizeof(state_t));
		} else if(dev_type==EIGHTKEY) {
			my_xpad[dev].spalte  = &default_eight_dev.spalte;
			my_xpad[dev].zeile   = &default_eight_dev.zeile;
			my_xpad[dev].key_cnt = default_eight_dev.key_cnt;
			my_xpad[dev].first   = default_eight_dev.first;
			memcpy(&my_xpad[dev].state , &default_eight_dev.state, sizeof(state_t));
		} else if (dev_type == TERMINAL) {
			printf("%010ld: Setup terminal %d"NL, HAL_GetTick(), dev_type);
		}
	}
	if (my_xpad[dev].key_cnt > 0) {
		GpioPortInit(my_xpad[dev].spalte);
	}
	if (my_xpad[dev].key_cnt > 0) {
		GpioPortInit(my_xpad[dev].zeile);
	}
	for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
		uint8_t value = lable2uint8(my_xpad[dev].state.label[i]);
		printf("%010ld: Label %c -> 0x%x"NL, HAL_GetTick(), my_xpad[dev].state.label[i], value);
	}
	memset(my_xpad[dev].val2idx, 0xff, MAX_BUTTON_CNT);
	for (uint8_t val=my_xpad[dev].first;val<my_xpad[dev].key_cnt;val++){
		for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
			uint8_t value=lable2uint8(my_xpad[dev].state.label[i]);
			if (val==value){
				my_xpad[dev].val2idx[val]= i;
				break;
			}
		}
	}
	for (uint8_t val=my_xpad[dev].first;val<my_xpad[dev].key_cnt;val++){
		uint8_t value = lable2uint8(my_xpad[dev].state.label[val]);
		printf("%010ld: val= %01x ->  %01x"NL, HAL_GetTick(), val,  value );
	}
	printf(NL);
}

static void xpad_state(dev_handle_t dev, kybd_r_t *ret) {
	if (mpy_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on state"NL, HAL_GetTick());
		return;
	}
	uint8_t i = 0;
	ret->first = my_xpad[dev].first;
	for (uint8_t val = ret->first; val < ret->first + ret->key_cnt; val++) {
		uint8_t idx = my_xpad[dev].val2idx[val];
		ret->state.state[i]   = my_xpad[dev].state.state[idx];
		ret->state.label[i++] = my_xpad[dev].state.label[idx];
	}
	ret->dirty = my_xpad[dev].dirty;
	return;
}

static void xpad_reset(dev_handle_t dev, bool hard) {
	if (mpy_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on reset"NL, HAL_GetTick());
		return;
	}
	my_xpad[dev].dirty = false;
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		if (hard) {
			my_xpad[dev].state.state[i] = OFF;
			my_xpad[dev].key[i] = reset_key;
		}
	}
	return;
}

kybd_t xscan_dev = {
	.init = &xpad_init,
	.scan = &xpad_spalten_scan,
	.reset= &xpad_reset,
	.state = &xpad_state,
	.dev_type = XSCAN,
	._state= {.label = {'1', '2', '3', 'a', '4', '5', '6', 'b', '7', '8', '9', 'c', '0', 'f', 'e' ,'d'},
	         .state = {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF}}	,
	.key_cnt = 16,
	.first = 0,
};

kybd_t eight_dev = {
	.init = &xpad_init,
	.scan = &xpad_eight_scan,
	.reset =&xpad_reset,
	.state = &xpad_state,
	.dev_type = EIGHTKEY,
	._state = {.label={'R','1', '2', '3','4', '5', '6','7', '8'},
			.state={OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF}},
	.key_cnt = 8,
	.first = 1,
};

void xpad_iprint(xpad_t *state, char *timestamp) {
	const uint8_t maxcnt = MINIMAL_LINESTART + 7;
	char text[maxcnt + 1];
	if (!state) {
		printf("%s Nothing returned"NL, timestamp);
		return;
	}
	snprintf(text, maxcnt, "%s%s", timestamp, "Value  ");
	printf(text);
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf(" %C ", state->state.label[i]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", timestamp, "State   ");
	printf(text);
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%s", key_state_3c[state->state.state[i]]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", timestamp, "Last    ");

	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%03d", state->key[i].last);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", timestamp, "Current  ");

	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%03d", state->key[i].current);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", timestamp, "Cnt      ");

	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%03d", state->key[i].cnt);
	}
	printf(NL);

}

