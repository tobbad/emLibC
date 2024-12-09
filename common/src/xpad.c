/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "common.h"
#include "gpio.h"
#include "keyboard.h"
#include "xpad.h"
#define ZEI_SPA_2_INDEX(zeile, spalte ) (uint8_t)(spalte*ZEILEN_CNT+zeile)
#define MINIMAL_LINESTART 16

static mkey_t reset_key = { 0, 0, 0, true };

static xpad_t default_xpad = {
	.zeile = { // Input rows
			{ .port = GPIOB, .pin = GPIO_PIN_10 }, // Zeile 1
			{ .port = GPIOB, .pin = GPIO_PIN_5 },  // Zeile 1
			{ .port = GPIOB, .pin = GPIO_PIN_3 },  // Zeile 1
			{ .port = GPIOA, .pin = GPIO_PIN_10 }, // Zeile 4
	},
	.spalte = { // Output columns each of type GpioPin_t
		{ .port = GPIOA, .pin = GPIO_PIN_0 }, // Spalte 1
		{ .port = GPIOA, .pin = GPIO_PIN_4 }, // Spalte 1
		{ .port = GPIOB, .pin = GPIO_PIN_0 }, // Spalte 3
		{ .port = GPIOC, .pin = GPIO_PIN_1 }, // Spalte 4
	},
	.key = {
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true },
		{ false, false, false, 0, true }
	},
	.state = { 	OFF, OFF, OFF, OFF,
				OFF, OFF, OFF, OFF,
				OFF, OFF, OFF, OFF,
				OFF, OFF, OFF, OFF },
	.value = {
		1, 2, 3, 0xa, // Zeile 1
		4, 5, 6, 0xb,  // Zeile 2
		7, 8, 9, 0xc,  // Zeile 3
		0, 0xf, 0xe, 0xd }, // Zeile 4
	.val2idx = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	.first = 0,
	.key_cnt = MAX_BUTTON_CNT,
	.dirty =false,
	.dev_type=XSCAN,
};

static xpad_t default_eight ={
		.zeile = { // Input rows
				{.port = GPIOA,  .pin=GPIO_PIN_0 },
				{.port = GPIOA,  .pin=GPIO_PIN_4 },
				{.port = GPIOB,  .pin=GPIO_PIN_0 },
				{.port = GPIOC,  .pin=GPIO_PIN_1 },
				{.port = GPIOB,  .pin=GPIO_PIN_10},
				{.port = GPIOB,  .pin=GPIO_PIN_5 },
				{.port = GPIOB,  .pin=GPIO_PIN_3 },
				{.port = GPIOA,  .pin=GPIO_PIN_10},
		},
		.spalte = 0,

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
	.val2idx = { 1, 2, 3, 4, 5, 6, 7, 8 },
	.first =0,
	.key_cnt =EIGHT_BUTTON_CNT,
	.dirty = false,
	.dev_type=EIGHTKEY
};

void xpad_t *my_xpad[KYBD_CNT];

static void update_key(uint8_t index, bool pin);

static void xpad_set_spalten_pin(kybdh_t dev, uint8_t spalten_nr) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on xpad_set_spalte"NL, HAL_GetTick());
		return;
	}
	GpioPin_t *pin = &my_xpad[dev]->spalte[spalten_nr];
	GpioPinWrite(pin, GPIO_PIN_SET);

	return;
}

static void xpad_reset_spalten_pin(kybdh_t dev, uint8_t spalten_nr) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on xpad_set_spalte"NL, HAL_GetTick());
		return;
	}
	GpioPin_t *pin = &my_xpad[dev]->spalte[spalten_nr];
	GpioPinWrite(pin, GPIO_PIN_RESET);
	return;
}
static void update_key(uint8_t index, bool pin){
	my_xpad[dev]->key[index].current = pinVal;
	if (pinVal){
		printf("%010ld: Pushed @ (z= %d, s=%d)"NL,HAL_GetTick(), z, spalten_nr);
	}
	if (my_xpad[dev]->key[index].current ^ my_xpad[dev]->key[index].last){
		printf("%010ld: Detected Key @ (z= %d, s=%d)"NL,HAL_GetTick(), z, spalten_nr);
	}
	my_xpad[dev]->key[index].unstable = my_xpad[dev]->key[index].unstable||(my_xpad[dev]->key[index].current ^ my_xpad[dev]->key[index].last);

	if (my_xpad[dev]->key[index].unstable) {
		if (my_xpad[dev]->key[index].current == my_xpad[dev]->key[index].last) {
			my_xpad[dev]->key[index].cnt++;
		}
	}
	if (my_xpad[dev]->key[index].cnt > STABLE_CNT) {
		// We got a stable state
		my_xpad[dev]->key[index].unstable = false;
		uint8_t value = my_xpad[dev]->value[index];
		my_xpad[dev]->key[index].last = pinVal;
		my_xpad[dev]->key[index].stable = pinVal;
		if (pinVal) {
			my_xpad[dev]->state[index] = ((my_xpad[dev]->state[index] + 1) % KEY_STAT_CNT);
			my_xpad[dev]->dirty = true;
			printf("%010ld: Pushed   value %X @(z= %d, s=%d)",
					HAL_GetTick(), value, z, spalten_nr);
		} else {
			printf("%010ld: Released value %X @(z= %d, s=%d)",
					HAL_GetTick(), value, z, spalten_nr);
		}
		res = res | (pinVal << z);
		my_xpad[dev]->key[index].cnt=0;
	}
	res = res | (pinVal << z);

	if (my_xpad[dev]->key[index].current ^ my_xpad[dev]->key[index].last){
		my_xpad[dev]->key[index].cnt=0;
	}
	my_xpad[dev]->key[index].last = my_xpad[dev]->key[index].current;

}

static uint8_t xpad_read_zeile(kybdh_t dev, uint8_t spalten_nr) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on read_zeile"NL, HAL_GetTick());
		return 0;
	}
	uint8_t res = 0;
	for (uint8_t z = 0; z < ZEILEN_CNT; z++) {
		bool pinVal = 0;
		GpioPinRead(&my_xpad[dev]->zeile[z], &pinVal);
		pinVal = !pinVal;
		uint8_t index = ZEI_SPA_2_INDEX(z, spalten_nr);
		update_key(index, pin);
	}
	return res;
}

static uint16_t xpad_spalten_scan(kybdh_t dev) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on scan"NL, HAL_GetTick());
		return false;
	}
	if (my_xpad[dev]->dev_type==XSCAN){
		printf("%010ld: Handle for this keyboard not valid"NL,HAL_GetTick());
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

void uint16_t xpad_eight_scan(kybdh_t dev){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on read_row"NL,HAL_GetTick());
		return false;
	}
	if (my_xpad[dev]->dev_type==EIGHTKEY){
		printf("%010ld: Handle for this keyboard not valid"NL,HAL_GetTick());
		return false;
	}
	uint16_t res=0;
	uint8_t cnt = ELCNT(my_xpad[dev]->zeile);
	for (uint8_t index=0;index<cnt; index++)	{
		bool pin=0;
		GpioPinRead(&my_xpad[dev]->zeile[index], &pin);
		pin =!pin;
		res = res|(update_pin(index, pin))<<index;
	}
	return res;
};

static void xpad_init(kybdh_t dev, xpad_t *scan_dev) {
	kybd_type_e dev_type=keyboard_get_dev_type(dev);
	if (dev_type==DEV_TYPE_NA) return;
	if (scan_dev != NULL) {
		my_xpad[dev] = scan_dev;
	} else {
		if (dev_type==XSCAN)
			my_xpad[dev] = &default_xpad;
		else if(dev_type==EIGHTKEY){
			my_xpad[dev] = &default_eight;
		} else{
			printf("%010ld: Unknown device type %d"NL, HAL_GetTick(), dev_type);
		}
	}
	uint8_t sp_cnt = ELCNT(my_xpad[dev]->spalte);
	if (sp_cnt!=0) {
		for (uint8_t spa_idx = 0; spa_idx < SPALTEN_CNT; spa_idx++) {
			GpioPin_t *pin = &my_xpad[dev]->spalte[spa_idx];
			GpioPinWrite(pin, true);
		}
	}
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		for (uint8_t j = 0; j < MAX_BUTTON_CNT; j++) {
			if (my_xpad[dev]->value[j] == i) {
				my_xpad[dev]->val2idx[i] = j;
			}
		}
	}
	printf(NL);
	printf("Value  Index"NL);
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%5X ", i);
		printf(" -> %5X"NL, my_xpad[dev]->val2idx[i]);
	}
}

static void xpad_state(kybdh_t dev, kybd_r_t *ret) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on state"NL, HAL_GetTick());
		return;
	}
	uint8_t i = 0;
	for (uint8_t val = ret->first; val < ret->first + ret->key_cnt; val++) {
		uint8_t idx = my_xpad[dev]->val2idx[val];
		ret->state[i] = my_xpad[dev]->state[idx];
		ret->value[i++] = my_xpad[dev]->value[idx];
	}
	ret->dirty = my_xpad[dev]->dirty;
	return;
}

static void xpad_reset(kybdh_t dev, bool hard) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on reset"NL, HAL_GetTick());
		return;
	}
	my_xpad[dev]->dirty = false;
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		if (hard){
			my_xpad[dev]->state[i] = OFF;
		}
		my_xpad[dev]->key[i] = reset_key;
	}
	return;
}

kybd_t xscan_dev = {
		.init = &xpad_init,
		.scan = &xpad_spalten_scan,
		.reset =&xpad_reset,
		.state = &xpad_state,
		.dev_type = XSCAN,
};

kybd_t eight_dev = {
		.init = &xpad_init,
		.scan = &xpad_eight_scan,
		.reset =&xpad_reset,
		.state = &xpad_state,
		.dev_type = EIGHTKEY,
};

void xpad_iprint(xpad_t *state, char *start) {
	const uint8_t maxcnt = MINIMAL_LINESTART + 7;
	char text[maxcnt + 1];
	if (!state) {
		printf("%s Nothing returned"NL, start);
		return;
	}
	snprintf(text, maxcnt, "%s%s", start, "Value  ");
	printf(text);
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf(" %C ", state->value[i]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "State   ");
	printf(text);
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%s", key_state_3c[state->state[i]]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "Last    ");

	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%03d", state->key[i].last);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "Current  ");

	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%03d", state->key[i].current);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "Cnt      ");

	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		printf("%03d", state->key[i].cnt);
	}
	printf(NL);

}

