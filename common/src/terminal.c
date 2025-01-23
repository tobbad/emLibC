/*
 * terminal.c
 *
 *  Created on: Jun 10, 2024
 *      Author: TBA
 */
#include "main.h"
#include "device.h"
#include <keyboard.h>


static state_t my_kybd = {
           .state=  {.state=  {  OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},
                     .label = {'R', 1, 2, 3, 4, 5, 6, 7, 8  }},
    .cnt=9,
    .first = 1, //First valid value
    .dirty = false,
};

static bool check_key(char ch);
static void terminal_reset(dev_handle_t dev, bool hard);

static  void terminal_init(dev_handle_t handle, dev_type_e dev_type, xpad_t *device) {
    terminal_reset(handle, true);
}


static bool check_key(char ch){
    bool ret = false;
    if (((ch >= '0') && (ch < '9')) || (ch == 'R')) {
        ret = true;
    }
    return ret;
}

static uint16_t terminal_scan(dev_handle_t dev) {
    static bool asked = false;
    uint8_t ch = UINT8_MAX;
    uint16_t res = UINT16_MAX;
    //char allowed_keys={'R'};

    if (!asked) {
        asked = true;
        printf("%08ld: Please enter key"NL, HAL_GetTick());
    }
    HAL_StatusTypeDef status;
    status = HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY);
    if (status == HAL_OK) {
        if (check_key(ch)) {
            if (ch == 'R') {
                res = 1;
                my_kybd.state.state[res]= ON;
            } else {
                ch = ch - '0';
                if ((ch>0) &&(ch<9)){
                    my_kybd.state.state[ch]  =  (my_kybd.state.state[ch] + 1)%KEY_STAT_CNT;
                } else {
                    printf("%08ld: Ignore imnvalid key %d"NL, HAL_GetTick(), ch);
                }
            }
            asked = false;
            return res;
        }
    }
    return res;
}

static void terminal_state(dev_handle_t dev, state_t *ret){
    *ret = my_kybd;
}

static void terminal_reset(dev_handle_t dev, bool hard){
    for (uint8_t i= my_kybd.first; i<my_kybd.first+my_kybd.key_cnt;i++){
        my_kybd.state.state[i] =OFF;
    }
    return ;
}

kybd_t terminal_dev = {
        .init = &terminal_init,
        .scan = &terminal_scan,
        .reset = &terminal_reset,
        .state = &terminal_state,
        .dev_type = TERMINAL,
};

int8_t terminal_waitForKey(char **key) {
    static char buffer[16];
    uint8_t ch = 0xFF;
    int16_t idx = 0;
    while (ch == 0xff) {
        HAL_UART_Receive(&huart2, &ch, 1, 0);
        if (((ch >= '0') && (ch < '9')) || (ch == 'R')) {
            buffer[idx++] = ch;
            ch = 0xFF;
        } else {
            buffer[idx] = '\0';
            if (idx > 0) {
                ch = 0;
            }
        }
    }
    char *stopstring = NULL;
    long long int res = strtol((char*) &buffer, &stopstring, 10);
    if (res == 0) {
        if (buffer[0] == '0') {
            return 0;
        }
        *key = &buffer[0];
        return -1;
    }
    if ((res >= 0) && (res < 10)) {
        return res;
    }
    return EM_ERR;
}
