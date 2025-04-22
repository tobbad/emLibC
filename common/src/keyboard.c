/*
 * keyboard.c
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#include "common.h"
#include "device.h"
#include "state.h"
#include "xpad.h"
#include "keyboard.h"
#include "gpio_port.h"
#define key_reset_cnt 100

static kybd_t *my_kybd[DEVICE_CNT];

char *key_state_3c[] = { "   ", "BLI", "ON ", "NA ", };
char *key_state_2c[] = { "  ", "BL", "ON ", "NA ", };

mkey_t reset_key = { .last=0, .current=0, .unstable=0, .cnt=0, .stable=true };

static uint8_t keyboard_find_dev(kybd_t *kybd) {
	for (uint8_t i = 0; i < DEVICE_CNT; i++) {
		if (my_kybd[i] == kybd) {
			return i;
		}
	}
	for (uint8_t i = 1; i < DEVICE_CNT; i++) {
		if (my_kybd[i] == NULL) {
			return i ;
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
		printf("Cannot find device"NL);
	}
	return dev_nr;
}

uint16_t keyboard_scan(dev_handle_t dev) {
	static int32_t cnt=0;
	static int32_t lcnt=-1;
	int16_t res = 0;
	if ((dev > 0) && my_kybd[dev] != NULL) {
		res = my_kybd[dev]->scan(dev);
	}
	if (res>0){
		lcnt=cnt-1;
	}
	if ((lcnt>0) &&(cnt-lcnt)%key_reset_cnt==0){
		keyboard_reset(dev, false);
		lcnt=-1;
		printf("Reset dirty"NL);
	}
	cnt++;
	return res;
}
;
void keyboard_reset(dev_handle_t dev, bool hard) {
	if ((dev > 0) && my_kybd[dev] != NULL) {
		my_kybd[dev]->reset(dev, hard);
	}
	return;
}

void keyboard_state(dev_handle_t dev, state_t *ret) {
	if ((dev > 0) && my_kybd[dev] != NULL) {
		my_kybd[dev]->state(dev, ret);
	}
	return;
}
;
void keyboard_print(state_t *state, char *title) {
	if (!state) {
		printf("No input"NL);
		return;
	}
	if (title!=NULL){
	    state_print(state, title);
	} else {
        state_print(state, "Keyboard");
	}
}

dev_type_e keyboard_get_dev_type(dev_handle_t dev) {
	if ((dev > 0) && (dev < TERMINAL)) {
		return my_kybd[dev]->dev_type;
	}
	return DEV_TYPE_NA;
}

