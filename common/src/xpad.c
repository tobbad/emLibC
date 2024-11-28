/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "gpio.h"
#include "keyboard.h"
#define ZEI_SPA_2_INDEX(zeile, spalte ) (uint8_t)(spalte*ZEILEN_CNT+zeile)
#define MINIMAL_LINESTART 16

static mkey_t reset_key = { 0, 0, 0, true };

xpad_t default_keyboard = { .spalte = { // Output columns each of type GpioPin_t
		{ .port = GPIOA, .pin = GPIO_PIN_0 }, // Spalte 1
				{ .port = GPIOA, .pin = GPIO_PIN_4 }, // Spalte 1
				{ .port = GPIOB, .pin = GPIO_PIN_0 }, // Spalte 3
				{ .port = GPIOC, .pin = GPIO_PIN_1 }, // Spalte 4
		}, .zeile = { // Input rows
		{ .port = GPIOB, .pin = GPIO_PIN_10 }, // Zeile 1
				{ .port = GPIOB, .pin = GPIO_PIN_5 },  // Zeile 1
				{ .port = GPIOB, .pin = GPIO_PIN_3 },  // Zeile 1
				{ .port = GPIOA, .pin = GPIO_PIN_10 }, // Zeile 4
		}, .key = { { false, false, 0, false }, { false, false, 0, false }, {
		false, false, 0, false }, { false, false, 0, false }, { false, false, 0,
		false }, { false, false, 0, false }, { false, false, 0, false }, {
		false, false, 0, false }, { false, false, 0, false }, { false, false, 0,
		false }, { false, false, 0, false }, { false, false, 0, false }, {
		false, false, 0, false }, { false, false, 0, false }, { false, false, 0,
		false }, { false, false, 0, false } }, .state = { OFF, OFF, OFF, OFF,
		OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF }, .value = {
		1, 2, 3, 0xa, // Zeile 1
		4, 5, 6, 0xb,  // Zeile 2
		7, 8, 9, 0xc,  // Zeile 3
		0, 0xf, 0xe, 0xd }, // Zeile 4

		.val2idx = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
				-1 }, .first = 0, .key_cnt = X_BUTTON_CNT, .first = 0, .dirty =
				false,

};

xpad_t *my_xpad[KYBD_CNT];

static void xpad_set_spalten_pin(kybd_h dev, uint8_t spalten_nr) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on xpad_set_spalte"NL, HAL_GetTick());
		return;
	}
	GpioPin_t *pin = &my_xpad[dev]->spalte[spalten_nr];
	GpioPinWrite(pin, GPIO_PIN_SET);

	return;
}

static void xpad_reset_spalten_pin(kybd_h dev, uint8_t spalten_nr) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on xpad_set_spalte"NL, HAL_GetTick());
		return;
	}
	GpioPin_t *pin = &my_xpad[dev]->spalte[spalten_nr];
	GpioPinWrite(pin, GPIO_PIN_RESET);
	return;
}
static uint8_t xpad_read_zeile(kybd_h dev, uint8_t spalten_nr) {
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
		my_xpad[dev]->key[index].current = pinVal;
		//bool newUnstable = my_xpad[dev]->key[index].current^my_xpad[dev]->key[index].last;
		my_xpad[dev]->key[index].unstable = my_xpad[dev]->key[index].current
				^ my_xpad[dev]->key[index].last;
		if (my_xpad[dev]->key[index].unstable) {
			my_xpad[dev]->key[index].cnt = 0;
		}
		if (my_xpad[dev]->key[index].unstable) {
			if (my_xpad[dev]->key[index].current
					== my_xpad[dev]->key[index].last) {
				my_xpad[dev]->key[index].cnt++;
			}
		}
		if (my_xpad[dev]->key[index].cnt > STABLE_CNT) {
			// We got a stable state
			uint8_t value = my_xpad[dev]->value[index];
			my_xpad[dev]->key[index].last = pinVal;
			my_xpad[dev]->key[index].current = pinVal;
			if (pinVal) {
				my_xpad[dev]->state[index] = ((my_xpad[dev]->state[index] + 1)
						% KEY_STAT_CNT);
				my_xpad[dev]->dirty = true;
				printf("%010ld: Pushed   value %X @(z= %d, s=%d)",
						HAL_GetTick(), value, z, spalten_nr);
			} else {
				printf("%010ld: Released value %X @(z= %d, s=%d)",
						HAL_GetTick(), value, z, spalten_nr);
			}
			my_xpad[dev]->key[index].stable = pinVal;
		}
		res = res | (pinVal << z);
		my_xpad[dev]->key[index].last = my_xpad[dev]->key[index].current;
	}
	return res;
}

static uint16_t xpad_spalten_scan(kybd_h dev) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on scan"NL, HAL_GetTick());
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

void xpad_init(kybd_h dev, void *xpad) {
	if (xpad != NULL) {
		my_xpad[dev] = (xpad_t*) xpad;
	} else {
		my_xpad[dev] = &default_keyboard;
	}
	for (uint8_t spa_idx = 0; spa_idx < SPALTEN_CNT; spa_idx++) {
		GpioPin_t *pin = &my_xpad[dev]->spalte[spa_idx];
		GpioPinWrite(pin, true);

	}
	for (uint8_t i = 0; i < X_BUTTON_CNT; i++) {
		for (uint8_t j = 0; j < X_BUTTON_CNT; j++) {
			if (my_xpad[dev]->value[j] == i) {
				my_xpad[dev]->val2idx[i] = j;
			}
		}
	}
	printf(NL);
	printf("Value  Index"NL);
	for (uint8_t i = 0; i < X_BUTTON_CNT; i++) {
		printf("%5X ", i);
		printf(" -> %5X"NL, my_xpad[dev]->val2idx[i]);
	}
}

void xpad_state(kybd_h dev, kybd_r_t *ret) {
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

void xpad_reset(kybd_h dev) {
	if (my_xpad[dev] == NULL) {
		printf("%010ld: No valid handle on reset"NL, HAL_GetTick());
		return;
	}
	my_xpad[dev]->dirty = false;
	for (uint8_t i = 0; i < X_BUTTON_CNT; i++) {
		my_xpad[dev]->state[i] = OFF;
		my_xpad[dev]->key[i] = reset_key;
	}
	return;
}

kybd_t xscan_dev = { .init = &xpad_init, .scan = &xpad_spalten_scan, .reset =
		&xpad_reset, .state = &xpad_state, .dev_type = XSCAN, };

void xpad_iprint(xpad_t *state, char *start) {
	const uint8_t maxcnt = MINIMAL_LINESTART + 7;
	char text[maxcnt + 1];
	if (!state) {
		printf("%s Nothing returned"NL, start);
		return;
	}
	snprintf(text, maxcnt, "%s%s", start, "Value  ");
	printf(text);
	for (uint8_t i = 0; i < X_BUTTON_CNT; i++) {
		printf(" %C ", state->value[i]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "State   ");
	printf(text);
	for (uint8_t i = 0; i < X_BUTTON_CNT; i++) {
		printf("%s", state2str[state->state[i]]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "Last    ");

	for (uint8_t i = 0; i < X_BUTTON_CNT; i++) {
		printf("%03d", state->key[i].last);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "Current  ");

	for (uint8_t i = 0; i < X_BUTTON_CNT; i++) {
		printf("%03d", state->key[i].current);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "Cnt      ");

	for (uint8_t i = 0; i < X_BUTTON_CNT; i++) {
		printf("%03d", state->key[i].cnt);
	}
	printf(NL);

}

