/*
 * terminal.c
 *
 *  Created on: Jun 10, 2024
 *      Author: TBA
 */
#include "common.h"
#include "device.h"
#include "keyboard.h"
#include "serial.h"
#include "state.h"
// clang-format off
static state_t my_term = {
    .first = 1, // First valid value
    .cnt = 8,
    .state = {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},
    .label = {' ', '1', '2', '3', '4', '5', '6', '7', '8', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    .dirty = false,

};
// clang-format on

static sio_t _serial;
bool data_in = false;
static void terminal_reset(dev_handle_t dev);

static em_msg terminal_init(dev_handle_t handle, dev_type_e dev_type, void *serial) {
    if (serial == NULL) {
        return EM_ERR;
    }
    terminal_reset(handle);
    _serial = *(sio_t *)serial;
    state_reset(&my_term);
    return EM_OK;
}

static void terminal_state(dev_handle_t dev, state_t *ret) {
    state_merge(&my_term, ret); // problem here
}

static em_msg terminal_diff(dev_handle_t dev, state_t *ref, state_t *diff) { return state_diff(&my_term, ref, diff); }

static em_msg terminal_add(dev_handle_t dev, state_t *add) { return state_add(&my_term, add); }

static bool terminal_isdirty(dev_handle_t dev) { return my_term.dirty; }
static void terminal_undirty(dev_handle_t dev) {
    my_term.dirty = false;
    return;
}

static void terminal_reset(dev_handle_t dev) {
    my_term.dirty = false;
    for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
        my_term.state[i] = OFF;
    }
    return;
}

kybd_t terminal_dev = {.init = &terminal_init,
                       .reset = &terminal_reset,
                       .state = &terminal_state,
                       .add = &terminal_add,
                       .diff = &terminal_diff,
                       .isdirty = &terminal_isdirty,
                       .undirty = &terminal_undirty,
                       .dev_type = TERMINAL,
                       .cnt = 8,
                       .first = 1

};

int8_t terminal_waitForNumber(char **key) {
    static char buffer[TX_BUFFER_SIZE];
    memset(buffer, 0, TX_BUFFER_SIZE);
    HAL_StatusTypeDef status;
    char ch[3] = {0xFF, 0, 0}; // only one is needed
    bool stay = true;
    int16_t idx = 0;
    while ((ch[0] == 0xff) && (stay)) {
        status = HAL_UART_Receive(_serial.uart, (uint8_t *)&ch, 1, 0);
        if (status == HAL_OK) {
            if ((ch[0] == '\r') || (ch[0] == '\n')) {
                stay = false;
                ch[0] = 0xff;
            }
            if (((ch[0] >= '0') && (ch[0] < '9')) || (ch[0] == 'R') || (ch[0] == 'r')) {
                buffer[idx++] = ch[0];
                if ((ch[0] == 'R') || (ch[0] == 'r')) {
                    stay = false;
                }
                if (idx >= 1) {
                    stay = false;
                }
                ch[0] = 0xFF;
            }
        }
    }
    char *stopstring = NULL;
    if ((buffer[0] == 'R') || (buffer[0] == 'r')) {
        *key = &buffer[0];
        return -1;
    }
    long long int res = strtol((char *)&buffer, &stopstring, 10);
    if (strlen(stopstring) == 0) {
        *key = &buffer[0];
        return res;
    }
    return -1;
}
