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

static kybd_t *my_kkybd[DEVICE_CNT];

char *key_state_3c[] = { "   ", "BLI", "ON ", "NA ", };
char *key_state_2c[] = { "  ", "BL", "ON ", "NA ", };


static uint8_t keyboard_find_dev(kybd_t *kybd) {
	for (uint8_t i = 0; i < DEVICE_CNT; i++) {
		if (my_kkybd[i] == kybd) {
			return i;
		}
	}
	for (uint8_t i = 1; i < DEVICE_CNT; i++) {
		if (my_kkybd[i] == NULL) {
			return i ;
		}
	}
	return 0;
};

dev_handle_t keyboard_init(kybd_t *kybd, void *device) {
	int8_t dev_nr=0;
	if (kybd != NULL) {
		dev_nr = keyboard_find_dev(kybd);
		if (dev_nr > 0) {
			my_kkybd[dev_nr] = kybd;
			kybd->init(dev_nr, kybd->dev_type, device);
            kybd->pcnt = -1;
		}
	} else {
		printf("Cannot find device"NL);
	}
	return dev_nr;
}

int16_t keyboard_scan(dev_handle_t dev) {
	int16_t res = 0;
	if ((dev > 0) && my_kkybd[dev] != NULL) {
		res = my_kkybd[dev]->scan(dev);
	}
	if (res>0){ // A number was enter
	    my_kkybd[dev]->plcnt=my_kkybd[dev]->pcnt-1;
	}
	if ((my_kkybd[dev]->plcnt>0) &&(my_kkybd[dev]->cnt-my_kkybd[dev]->plcnt)%key_reset_cnt==0){
		keyboard_reset(dev);
		my_kkybd[dev]->pcnt=-1;
		printf("Reset dirty"NL);
	}
	my_kkybd[dev]->pcnt++;
	return res;
}
;
void keyboard_reset(dev_handle_t dev) {
	if ((dev > 0) && my_kkybd[dev] != NULL) {
		my_kkybd[dev]->reset(dev);
	}
	return;
}

void keyboard_state(dev_handle_t dev, state_t *ret) {
    if ((dev > 0) && my_kkybd[dev] != NULL) {
        my_kkybd[dev]->state(dev, ret);
    }
    return;
}
void keyboard_undirty(dev_handle_t dev) {
    if ((dev > 0) && my_kkybd[dev] != NULL) {
        my_kkybd[dev]->undirty(dev);
    }
    return;
}

bool keyboard_isdirty(dev_handle_t dev) {
    if ((dev > 0) && my_kkybd[dev] != NULL) {
        return my_kkybd[dev]->isdirty(dev);
    }
    return false;
}

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

