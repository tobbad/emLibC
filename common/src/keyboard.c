/*
 * keyboard.c
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#include "keyboard.h"
#include "common.h"
#include "device.h"
#include "gpio_port.h"
#include "state.h"
#include "xpad.h"
#define key_reset_cnt 100

static kybd_t *my_kybd[DEVICE_CNT];

char *key_state_3c[] = {
    "   ",
    "BLI",
    "ON ",
    "NA ",
};
char *key_state_2c[] = {
    "  ",
    "BL",
    "ON ",
    "NA ",
};

static uint8_t keyboard_find_dev(kybd_t *kybd) {
    uint8_t i;
    for (i = 1; i < DEVICE_CNT; i++) {
        if (my_kybd[i] == kybd) {
            return i;
        }
    }
    for (i = 1; i < DEVICE_CNT; i++) {
        if (my_kybd[i] == NULL) {
            my_kybd[i] = kybd;
            return i;
        }
    }
    return 0;
};

dev_handle_t keyboard_init(kybd_t *kybd, void *device) {
    int8_t dev_nr = 0;
    if (kybd != NULL) {
        dev_nr = keyboard_find_dev(kybd);
        if (dev_nr > 0) {
            kybd->init(dev_nr, kybd->dev_type, device);
            kybd->pcnt = -1;
        }
    } else {
        printf("Cannot find device" NL);
    }
    return dev_nr;
}

int16_t keyboard_scan(dev_handle_t dev) {
    int16_t res = 0;
    if ((dev > 0) && my_kybd[dev] != NULL) {
        res = my_kybd[dev]->scan(dev);
    }
    return res;
};
void keyboard_reset(dev_handle_t dev) {
    if ((dev > 0) && my_kybd[dev] != NULL) {
        my_kybd[dev]->reset(dev);
    }
    return;
}

void keyboard_set_state(dev_handle_t dev, state_t *state) {
    if ((dev > 0) && my_kybd[dev] != NULL) {
        my_kybd[dev]->set_state(dev, state);
    }
}

void keyboard_state(dev_handle_t dev, state_t *ret) {
    if ((dev > 0) && my_kybd[dev] != NULL) {
        my_kybd[dev]->state(dev, ret);
    }
    return;
}

em_msg keyboard_diff(dev_handle_t dev, state_t *ref, state_t *diff) {
    if ((dev > 0) && my_kybd[dev] != NULL) {
        return my_kybd[dev]->diff(dev, ref, diff);
    }
    return EM_ERR;
}
void keyboard_add(dev_handle_t dev, state_t *add) {
    if ((dev > 0) && my_kybd[dev] != NULL) {
        my_kybd[dev]->add(dev, add);
    }
    return;
}
void keyboard_undirty(dev_handle_t dev) {
    if ((dev > 0) && my_kybd[dev] != NULL) {
        my_kybd[dev]->undirty(dev);
    }
    return;
}

bool keyboard_isdirty(dev_handle_t dev) {
    if ((dev > 0) && my_kybd[dev] != NULL) {
        return my_kybd[dev]->isdirty(dev);
    }
    return false;
}

void keyboard_print(state_t *state, char *title) {
    if (!state) {
        printf("No input" NL);
        return;
    }
    if (title != NULL) {
        state_print(state, title);
    } else {
        state_print(state, "Keyboard");
    }
}
