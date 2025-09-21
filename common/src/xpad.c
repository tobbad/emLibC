/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "xpad.h"
#include "_gpio.h"
#include "common.h"
#include "device.h"
#include "gpio_port.h"
#include "keyboard.h"
#include "state.h"

#define MINIMAL_LINESTART 16
#define ZEILEN_CNT 4
#define SPALTEN_CNT 4

xpad_dev_t my_xpad[DEVICE_CNT];
xpad_dev_t *mpy_xpad[DEVICE_CNT];

static mkey_t reset_key = {.last = 1, .current = 1, .unstable = 0, .cnt = 0, .stable = true};

static xpad_dev_t default_xscan_dev = {
    .spalte =
        {
            // Output
            .cnt = SPALTEN_CNT,
            .pin =
                {
                    {.port = GPIOA,
                     .pin = GPIO_PIN_0,
                     .conf =
                         {
                             .Mode = GPIO_MODE_OUTPUT_PP,
                             .Pull = GPIO_PULLUP,
                         }}, // spalte 1
                    {.port = GPIOA,
                     .pin = GPIO_PIN_4,
                     .conf =
                         {
                             .Mode = GPIO_MODE_OUTPUT_PP,
                             .Pull = GPIO_PULLUP,
                         }}, // spalte 2
                    {.port = GPIOB,
                     .pin = GPIO_PIN_3,
                     .conf =
                         {
                             .Mode = GPIO_MODE_OUTPUT_PP,
                             .Pull = GPIO_PULLUP,
                         }}, // spalte 3
                    {.port = GPIOC,
                     .pin = GPIO_PIN_1,
                     .conf =
                         {
                             .Mode = GPIO_MODE_OUTPUT_PP,
                             .Pull = GPIO_PULLUP,
                         }}, // spalte 4
                },
        },
    .zeile =
        {
            // Input
            .cnt = ZEILEN_CNT,
            .pin =
                {
                    {.port = GPIOA,
                     .pin = GPIO_PIN_10,
                     .conf =
                         {
                             .Mode = GPIO_MODE_INPUT,
                             .Pull = GPIO_PULLUP,
                         }}, // zeilen 1
                    {.port = GPIOB,
                     .pin = GPIO_PIN_3,
                     .conf =
                         {
                             .Mode = GPIO_MODE_INPUT,
                             .Pull = GPIO_PULLUP,
                         }}, // zeilen 2
                    {.port = GPIOB,
                     .pin = GPIO_PIN_5,
                     .conf =
                         {
                             .Mode = GPIO_MODE_INPUT,
                             .Pull = GPIO_PULLUP,
                         }}, // zeilen 3
                    {.port = GPIOB,
                     .pin = GPIO_PIN_10,
                     .conf =
                         {
                             .Mode = GPIO_MODE_INPUT,
                             .Pull = GPIO_PULLUP,
                         }}, // zeilen 4
                },
        },
    .dev_type = XSCAN,
    .state =
        {
            .label = {'1', '2', '3', 'A', '4', '5', '6', 'B', '7', '8', '9', 'C', '0', 'F', 'E', 'D'},
            .state = {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},
            .cnt = ZEILEN_CNT * SPALTEN_CNT,
            .first = 0,

        },

};

static xpad_dev_t default_eight_dev = {
    .spalte = {{{0}}},
    .zeile =
        {
            .cnt = EIGHT_BUTTON_CNT,
            .pin =
                {
                    {.port = GPIOC, .pin = GPIO_PIN_1, .conf = {.Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP}},  // Taste 1
                    {.port = GPIOC, .pin = GPIO_PIN_2, .conf = {.Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP}},  // Taste 2
                    {.port = GPIOC, .pin = GPIO_PIN_3, .conf = {.Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP}},  // Taste 3
                    {.port = GPIOA, .pin = GPIO_PIN_0, .conf = {.Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP}},  // Taste 4
                    {.port = GPIOA, .pin = GPIO_PIN_4, .conf = {.Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP}},  // Taste 5
                    {.port = GPIOA, .pin = GPIO_PIN_5, .conf = {.Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP}},  // Taste 6
                    {.port = GPIOC, .pin = GPIO_PIN_15, .conf = {.Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP}}, // Taste 7
                    {.port = GPIOC, .pin = GPIO_PIN_9, .conf = {.Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP}},  // Taste 8
                },
        },

    .dev_type = EIGHTKEY,
    .state =
        {
            .label = {'1', '2', '3', '4', '5', '6', '7', '8'},
            .state = {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},
            .cnt = EIGHT_BUTTON_CNT,
            .first = 0,
        },
};
static void xpad_reset(dev_handle_t devh);
static void xpad_reset_key(mkey_t *key, uint8_t cnt);

static uint16_t xpad_read_zeile(dev_handle_t devh, uint8_t spalten_nr);

static uint8_t index_2_zei(xpad_dev_t *kbd, uint8_t index) {
    uint8_t res = index % (kbd->zeile.cnt);
    return res;
}
static uint8_t index_2_spa(xpad_dev_t *kbd, uint8_t index) {
    uint8_t zei = index_2_zei(kbd, index);
    uint8_t res = index - zei * kbd->spalte.cnt;
    return res;
}

static uint8_t zei_spa_2_index(xpad_dev_t *kbd, uint8_t zeile, uint8_t spalte) {
    return (uint8_t)(spalte + zeile * kbd->spalte.cnt);
}

static int8_t label2int8(char label) {
    int8_t value = label - '0';
    if (label == ' ')
        return -1;
    if (value < 10)
        return value;
    if (value > 9) {
        label &= 0xEF;
        value = label - 'a' + 10;
        if (value > MAX_STATE_CNT)
            return -1;
    } else {
        return -1;
    }
    return value;
}

em_msg xpad_copy_state(dev_handle_t devh) {
    state_t *from = &default_eight_dev.state;
    state_t *to = &my_xpad[devh].state;
    return state_copy(from, to);
};

static em_msg xpad_init(dev_handle_t devh, dev_type_e dev_type, void *dev) {
    if (dev_type == DEV_TYPE_NA)
        return EM_ERR;
    xpad_dev_t *device = (xpad_dev_t *)dev;
    my_xpad[devh].dev_type = dev_type;
    mpy_xpad[devh] = &my_xpad[devh];
    if (device != NULL) {
        my_xpad[devh].spalte = device->spalte;
        my_xpad[devh].zeile = device->zeile;
        memcpy(&my_xpad[devh].state, &device->state, sizeof(state_t));
    } else {
        if (dev_type == XSCAN) {
            my_xpad[devh].spalte = default_xscan_dev.spalte;
            my_xpad[devh].zeile = default_xscan_dev.zeile;
            my_xpad[devh].state = default_xscan_dev.state;
            memcpy(&my_xpad[devh].state, &default_xscan_dev.state, sizeof(state_t));
        } else if (dev_type == EIGHTKEY) {
            my_xpad[devh].spalte = default_eight_dev.spalte;
            my_xpad[devh].zeile = default_eight_dev.zeile;
            memset(&my_xpad[devh].state, 0, sizeof(state_t));
            memcpy(&my_xpad[devh].state, &default_eight_dev.state, sizeof(state_t));
        } else if (dev_type == TERMINAL) {
            printf("Setup terminal %d" NL, dev_type);
        }
    }
    if (my_xpad[devh].spalte.cnt > 0) {
        GpioPortInit(&my_xpad[devh].spalte);
    } else {
        my_xpad[devh].spalte.cnt = 1;
    }

    if (my_xpad[devh].zeile.cnt > 0) {
        GpioPortInit(&my_xpad[devh].zeile);
    } else {
        my_xpad[devh].zeile.cnt = 1;
    }
    xpad_reset_key(my_xpad[devh].key, MAX_BUTTON_CNT);
    xpad_copy_state(devh);
    xpad_reset(devh);
    return EM_OK;
}

static void xpad_set_spalten_pin(dev_handle_t devh, uint8_t spalten_nr) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on xpad_set_spalte" NL);
        return;
    }
    gpio_pin_t *pin = &my_xpad[devh].spalte.pin[spalten_nr];
    GpioPinWrite(pin, GPIO_PIN_SET);

    return;
}

static void xpad_reset_spalten_pin(dev_handle_t devh, uint8_t spalten_nr) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on xpad_set_spalte" NL);
        return;
    }
    gpio_pin_t *pin = &my_xpad[devh].spalte.pin[spalten_nr];
    GpioPinWrite(pin, GPIO_PIN_RESET);
    return;
}

static void xpad_reset_key(mkey_t *key, uint8_t cnt) {
    for (uint8_t i = 0; i < cnt; i++) {
        key[i] = reset_key;
    }
    return;
}

static uint16_t xpad_update_key(uint8_t devh, uint8_t zeile, bool pinVal) {
    my_xpad[devh].key[zeile].current = pinVal;
    //	if (pinVal) {
    //		printf("Pushed @ (index= %d) to %d"NL, index, pinVal);
    //	}
    // uint8_t z,s = INDEX_2_ZEI_SPA(index);
    uint8_t z = index_2_zei(&my_xpad[devh], zeile);
    uint8_t s = index_2_spa(&my_xpad[devh], zeile);
    uint8_t res = 0;
    //	if (my_xpad[devh].key[index].current ^ my_xpad[devh].key[index].last) {
    //		printf("Detected Key @ (index =%d) with label %c"NL, index, my_xpad[devh].state.label[index]);
    //	}
    // bool unstable = my_xpad[devh].key[index].unstable;
    my_xpad[devh].key[zeile].unstable =
        my_xpad[devh].key[zeile].unstable || (my_xpad[devh].key[zeile].current ^ my_xpad[devh].key[zeile].last);
    if (my_xpad[devh].key[zeile].current ^ my_xpad[devh].key[zeile].last) {
        // printf("Set unstable to %d"NL,my_xpad[devh].key[zeile].unstable);
    }
    if (my_xpad[devh].key[zeile].unstable) {
        if (my_xpad[devh].key[zeile].current == my_xpad[devh].key[zeile].last) {
            my_xpad[devh].key[zeile].cnt++;
            if (pinVal != false) {
                // printf("Increased zeile %d to %d (pinVal=%d)"NL, zeile, my_xpad[devh].key[zeile].cnt, pinVal);   unknwn
                // port
            }
        } else {
            // printf("Reset zeile %d from %d (pinVal=%d)"NL, zeile,
            // my_xpad[devh].key[zeile].cnt, pinVal);
            my_xpad[devh].key[zeile].cnt = 0;
        }
    }
    if (my_xpad[devh].key[zeile].cnt > STABLE_CNT) {
        // We got a stable state
        my_xpad[devh].key[zeile].unstable = false;
        char label = my_xpad[devh].state.label[zeile];
        my_xpad[devh].key[zeile].last = pinVal;
        my_xpad[devh].key[zeile].stable = pinVal;
        // printf("Reached zeile %d logi level %d (pinVal=%d)"NL, zeile,STABLE_CNT, pinVal);
        if (my_xpad[devh].key[zeile].stable) {
            my_xpad[devh].state.state[zeile] = ((my_xpad[devh].state.state[zeile] + 1) % STATE_CNT);
            // my_xpad[devh].state.dirty = true;
            my_xpad[devh].state.dirty = true;
            // printf("Pushed   Key @ (zeile =%d, z=%d, s=%d, value = %c)" NL, zeile, z, s, label);
        } else {
            // printf("Released Key @ (zeile =%d, z=%d, s=%d, value = %c)"NL, zeile, z s, label);
            // ,
            my_xpad[devh].key[zeile].cnt = 0;
        }
        res = res | (pinVal << z);
        my_xpad[devh].key[zeile].cnt = 0;
    }

    //	if (my_xpad[devh].key[zeile].current ^ my_xpad[devh].key[zeile].last) {
    //		my_xpad[devh].key[zeile].cnt = 0;
    //	}
    my_xpad[devh].key[zeile].last = my_xpad[devh].key[zeile].current;
    return res;
}

uint16_t key2value(uint16_t value) {
    for (uint8_t i = 0; i < 16; i++) {
        if (1 << i == value)
            return i;
    }
    return 0xffff;
}

static int16_t xpad_eight_scan(dev_handle_t devh) {
    uint16_t res = 0;
    int16_t index = 0;
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on read_row" NL);
        return false;
    }
    if (my_xpad[devh].dev_type != EIGHTKEY) {
        printf("Handle for this keyboard not valid" NL);
        return false;
    }
    for (uint8_t zeile = 0; zeile < my_xpad[devh].zeile.cnt; zeile++) {
        bool pin = 0;
        GpioPinRead(&my_xpad[devh].zeile.pin[zeile], &pin);
        res = res | (xpad_update_key(devh, zeile, pin));
    }
    if (res == 0)
        return -1;
    index = key2value(res);
    char ch = my_xpad[devh].state.label[index];
    // printf("Got Keyscan 0x%04x, index %d, label %c"NL, res, index, ch);
    return index;
}

static uint16_t xpad_spalten_scan(dev_handle_t devh) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on scan" NL);
        return false;
    }
    if (my_xpad[devh].dev_type != XSCAN) {
        printf("Handle (%d) for this keyboard not valid" NL, my_xpad[devh].dev_type);
        return false;
    }
    int16_t res = 0;
    for (uint8_t s = 0; s < my_xpad[devh].spalte.cnt; s++) {
        uint8_t ir = 0;
        xpad_reset_spalten_pin(devh, s);
        HAL_Delay(SETTLE_TIME_MS);
        ir = xpad_read_zeile(devh, s);
        res = res | (ir << (4 * s));
        xpad_set_spalten_pin(devh, s);
    }
    uint8_t ch = my_xpad[devh].state.label[res];
    return ch;
}

static uint16_t xpad_read_zeile(dev_handle_t devh, uint8_t spalten_nr) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on read_zeile" NL);
        return 0;
    }
    uint16_t res = 0;
    for (int8_t z = 0; z < my_xpad[devh].zeile.cnt; z++) {
        bool pinVal = 0;
        GpioPinRead(&my_xpad[devh].zeile.pin[z], &pinVal);
        uint8_t index = zei_spa_2_index(&my_xpad[devh], z, spalten_nr);
        res = res | xpad_update_key(devh, index, pinVal);
    }
    return res;
}

static void xpad_state(dev_handle_t devh, state_t *oState) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on state" NL);
        return;
    }
    uint8_t oIdx = oState->first;
    oState->clabel.cmd = my_xpad[devh].state.clabel.cmd;
    uint8_t iIdx = my_xpad[devh].state.first;
    for (uint8_t i = 0; i < oState->cnt; i++, iIdx++, oIdx++) {
        if (oState->state[oIdx] != my_xpad[devh].state.state[iIdx]) {
            oState->state[oIdx] = my_xpad[devh].state.state[iIdx];
            oState->dirty = true;
        }
        oState->label[oIdx] = my_xpad[devh].state.label[iIdx];
    }
    return;
}

static em_msg xpad_diff(dev_handle_t devh, state_t *ref, state_t *diff) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on state" NL);
        return EM_ERR;
    }
    return state_diff(ref, &my_xpad[devh].state, diff);
}

static em_msg xpad_add(dev_handle_t devh, state_t *add) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on state" NL);
        return EM_ERR;
    }
    return state_add(&my_xpad[devh].state, add);
}

static bool xpad_isdirty(dev_handle_t devh) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on state" NL);
        return false;
    }
    return my_xpad[devh].state.dirty;
}
static void xpad_undirty(dev_handle_t devh) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on state" NL);
        return;
    }
    my_xpad[devh].state.dirty = false;
    return;
}

static void xpad_reset(dev_handle_t devh) {
    if (mpy_xpad[devh] == NULL) {
        printf("No valid handle on reset" NL);
        return;
    }
    state_reset(&my_xpad[devh].state);
    return;
}

kybd_t xscan_dev = {
    .init = &xpad_init,
    .scan = &xpad_spalten_scan,
    .reset = &xpad_reset,
    .state = &xpad_state,
    .diff = &xpad_diff,
    .add = &xpad_add,
    .isdirty = &xpad_isdirty,
    .undirty = &xpad_undirty,
    .dev_type = XSCAN,
    .cnt = 16,
    .first = 0,
};

kybd_t eight_dev = {
    .init = &xpad_init,
    .scan = &xpad_eight_scan,
    .reset = &xpad_reset,
    .state = &xpad_state,
    .diff = &xpad_diff,
    .add = &xpad_add,
    .isdirty = &xpad_isdirty,
    .undirty = &xpad_undirty,
    .dev_type = EIGHTKEY,
    .cnt = 8,
    .first = 0,
};
