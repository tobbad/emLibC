/*
 * keyboard.c
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#include "common.h"
#include "device.h"
#include "xpad.h"
#include "keyboard.h"
#include "gpio_port.h"
static kybd_t *my_kybd[DEVICE_CNT];

char *key_state_3c[] = { "   ", "BLI", "ON ", "NA ", };
char *key_state_2c[] = { "  ", "BL", "ON ", "NA ", };

mkey_t reset_key = { .last=0, .current=0, .unstable=0, .cnt=0, .stable=true };

static uint8_t keyboard_find_dev(kybd_t *kybd) {
	for (uint8_t i = 0; i < DEVICE_CNT; i++) {
		if (my_kybd[i] == kybd) {
			return i + 1;
		}
	}
	for (uint8_t i = 0; i < DEVICE_CNT; i++) {
		if (my_kybd[i] == NULL) {
			return i + 1;
		}
	}
	return 0;
};

dev_handle_t keyboard_init(kybd_t *kybd, xpad_t *device) {
	int8_t dev_nr=0;
	if (kybd != NULL) {
		dev_nr = keyboard_find_dev(kybd);
		if (dev_nr > 0) {
			my_kybd[dev_nr] = kybd;
			kybd->init(dev_nr, kybd->dev_type, device);
		}
	} else {
		printf("%010ld: Cannot find device"NL, HAL_GetTick());
	}
	return dev_nr;
}

uint16_t keyboard_scan(dev_handle_t dev) {
	uint16_t res = 0;
	if ((dev > 0) && my_kybd[dev] != NULL) {
		res = my_kybd[dev]->scan(dev);
	}
	return res;
}
;
void keyboard_reset(dev_handle_t dev, bool hard) {
	if ((dev > 0) && my_kybd[dev] != NULL) {
		my_kybd[dev]->reset(dev, hard);
	}
	return;
}

void keyboard_state(dev_handle_t dev, kybd_r_t *ret) {
	if ((dev > 0) && my_kybd[dev] != NULL) {
		my_kybd[dev]->state(dev, ret);
	}
	return;
}
;
void keyboard_print(kybd_r_t *state, char *timestamp) {
	if (!state) {
		printf("%s Nothing returned"NL, timestamp);
		return;
	}
	printf("%s: Label  ", timestamp);
	for (uint8_t i = 0; i < state->key_cnt; i++) {
		if (state->state.label[i] < 10) {
			printf(" %c ", '0' + state->state.label[i]);
		} else {
			printf(" %c ", 'A' + (state->state.label[i] - 10));
		}
	}
	printf(NL);
	printf("%s: State  ", timestamp);
	for (uint8_t i = 0; i < state->key_cnt; i++) {
		char *text = key_state_3c[state->state.state[i]];
		printf("%s", text);
	}
	printf(NL);
}

dev_type_e keyboard_get_dev_type(dev_handle_t dev) {
	if ((dev > 0) && (dev < TERMINAL)) {
		return my_kybd[dev]->dev_type;
	}
	return DEV_TYPE_NA;
}

