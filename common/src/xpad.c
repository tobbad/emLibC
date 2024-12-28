/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "common.h"
#include "gpio.h"
#include "gpio_port.h"
#include "keyboard.h"
#include "xpad.h"

#define ZEI_SPA_2_INDEX(zeile, spalte ) (uint8_t)(spalte*ZEILEN_CNT+zeile)
#define INDEX_2_ZEI_SPA(index) (  ((uint8_t)(index%ZEILEN_CNT)), ((uint8_t)(index-(index%ZEILEN_CNT)*ZEILEN_CNT) ))
#define INDEX_2_ZEI(index)  ((uint8_t)(index%ZEILEN_CNT))
#define INDEX_2_SPA(index) ((uint8_t)(((index - INDEX_2_ZEI(index)))%ZEILEN_CNT))
#define MINIMAL_LINESTART 16

xpad_t my_xpad[KYBD_CNT];
xpad_t *mpy_xpad[KYBD_CNT];



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
	.value = {1, 2, 3, 0xa, 4, 5, 6, 0xb, 7, 8, 9, 0xc, 0, 0xf, 0xe ,0xd },
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
	.value = {1, 2, 3, 4, 5, 6, 7, 8},
	.key_cnt=8,
	.first = 1,

};

static void xpad_set_spalten_pin(kybdh_t dev, uint8_t spalten_nr) {
	if (mpy_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on xpad_set_spalte"NL, HAL_GetTick());
		return;
	}
	gpio_pin_t *pin = &my_xpad[dev].spalte->pin[spalten_nr];
	GpioPinWrite(pin, GPIO_PIN_SET);

	return;
}

static void xpad_reset_spalten_pin(kybdh_t dev, uint8_t spalten_nr) {
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
		uint8_t value = my_xpad[dev].value[index];
		my_xpad[dev].key[index].last = pinVal;
		my_xpad[dev].key[index].stable = pinVal;
		if (pinVal) {
			my_xpad[dev].state[index] = ((my_xpad[dev].state[index] + 1)
					% KEY_STAT_CNT);
			my_xpad[dev].dirty = true;
			printf("%010ld: Pushed   Key @ (index =%d, z=%d, s=%d, value = %d)"NL, HAL_GetTick(), index, z, s, value);
		} else {
			printf("%010ld: Released Key @ (index =%d, z=%d, s=%d, value = %d)"NL, HAL_GetTick(), index, z , s, value);
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

static uint16_t xpad_eight_scan(kybdh_t dev) {
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
static uint16_t xpad_read_zeile(kybdh_t dev, uint8_t spalten_nr) {
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

static uint16_t xpad_spalten_scan(kybdh_t dev) {
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
		printf("%010ld: Key Label is  0x%lX"NL, HAL_GetTick(), (uint32_t) res);
	}
	return res;
}

static void xpad_init(kybdh_t dev, kybd_type_e dev_type, xpad_dev_t *device) {
	if (dev_type == DEV_TYPE_NA)
		return;
	my_xpad[dev].dev_type= dev_type;
	mpy_xpad[dev] = &my_xpad[dev];
	if (device != NULL) {
		my_xpad[dev].spalte = &device->spalte;
		my_xpad[dev].zeile =  &device->zeile;
		memcpy(my_xpad[dev].value, device->value, MAX_BUTTON_CNT);
	} else {
		if (dev_type ==XSCAN) {
			my_xpad[dev].spalte = &default_xscan_dev.spalte;
			my_xpad[dev].zeile  = &default_xscan_dev.zeile;
			my_xpad[dev].key_cnt  = default_xscan_dev.key_cnt;
			my_xpad[dev].first  = default_xscan_dev.first;
			memcpy(my_xpad[dev].value, default_xscan_dev.value, my_xpad[dev].key_cnt+1);
		} else if(dev_type==EIGHTKEY) {
			my_xpad[dev].spalte = &default_eight_dev.spalte;
			my_xpad[dev].zeile  = &default_eight_dev.zeile;
			my_xpad[dev].key_cnt    = default_eight_dev.key_cnt;
			my_xpad[dev].first    = default_eight_dev.first;
			memset(my_xpad[dev].value, 0xff, MAX_BUTTON_CNT);
			memcpy(my_xpad[dev].value , default_eight_dev.value, my_xpad[dev].key_cnt);
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
	memset(my_xpad[dev].val2idx, 0xff, MAX_BUTTON_CNT);
	for (uint8_t val=my_xpad[dev].first;val<my_xpad[dev].key_cnt+1;val++){
		for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
			if (val==my_xpad[dev].value[i]){
				my_xpad[dev].val2idx[val]= i;
				break;
			}
		}
	}
	for (uint8_t val=my_xpad[dev].first;val<my_xpad[dev].key_cnt;val++){
		printf("%010ld: val= %01x ->  %01x"NL, HAL_GetTick(), val,  my_xpad[dev].val2idx[val] );
	}
	printf(NL);
}

static void xpad_state(kybdh_t dev, kybd_r_t *ret) {
	if (mpy_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on state"NL, HAL_GetTick());
		return;
	}
	uint8_t i = 0;
	ret->first = my_xpad[dev].first;
	for (uint8_t val = ret->first; val < ret->first + ret->key_cnt; val++) {
		uint8_t idx = my_xpad[dev].val2idx[val];
		ret->state[i]   = my_xpad[dev].state[idx];
		ret->value[i++] = my_xpad[dev].value[idx];
	}
	ret->dirty = my_xpad[dev].dirty;
	return;
}

static void xpad_reset(kybdh_t dev, bool hard) {
	if (mpy_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on reset"NL, HAL_GetTick());
		return;
	}
	my_xpad[dev].dirty = false;
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		if (hard) {
			my_xpad[dev].state[i] = OFF;
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
	.value ={1, 2, 3, 0xa, 4, 5, 6, 0xb, 7, 8, 9, 0xc, 0, 0xf, 0xe ,0xd },
	.key_cnt = 16,
	.first = 0,
};

kybd_t eight_dev = {
	.init = &xpad_init,
	.scan = &xpad_eight_scan,
	.reset =&xpad_reset,
	.state = &xpad_state,
	.dev_type = EIGHTKEY,
	.value = {1, 2, 3, 4, 5, 6, 7, 8},
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
		printf(" %C ", state->value[i]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", timestamp, "State   ");
	printf(text);
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%s", key_state_3c[state->state[i]]);
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

