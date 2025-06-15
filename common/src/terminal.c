/*
 * terminal.c
 *
 *  Created on: Jun 10, 2024
 *      Author: TBA
 */
#include "main.h"
#include "common.h"
#include "device.h"
#include "serial.h"
#include "state.h"
#include <keyboard.h>

static state_t my_term = {
    .first = 1, //First valid value
    .cnt =8,
    .state  = { OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF },
    .label =  { 'R', '1', '2', '3', '4', '5', '6', '7', '8' },
    .dirty = false,

};
static sio_t sio;
static void terminal_reset(dev_handle_t dev, bool hard);

static void terminal_init(dev_handle_t handle, dev_type_e dev_type,	sio_t * serial) {
	terminal_reset(handle, true);
	sio =(sio_t)*serial;

}

static bool check_key(char ch) {
    return state_ch2idx(&my_term, ch)>=0;
}

static uint16_t terminal_scan(dev_handle_t dev) {
    char ch;
	static bool asked = false;
	int16_t res = 0;
	//char allowed_keys={'R'};

	if (!asked) {
		asked = true;
		printf("Please enter key"NL);
	}
    HAL_StatusTypeDef status;
    status = HAL_UART_Receive(sio.uart, (uint8_t*)&ch, 1, 0);
    if (status == HAL_OK) {
        ch = toupper(ch);
        int8_t idx=state_ch2idx(&my_term, ch);
        if (idx>=0){
            res = ch - '0';
            my_term.dirty= true;
            if (res<9){
                state_propagate(&my_term, ch);
                my_term.clabel = 0;
                my_term.dirty= true;
            } else {
                my_term.clabel = ch;
            }
        }
        asked = false;
        return res;
	}
	return res;
}

static void terminal_state(dev_handle_t dev, state_t *ret) {
    state_merge(&my_term, ret);// problem here
}

static bool terminal_isdirty(dev_handle_t dev){
    return my_term.dirty;
}
static void terminal_undirty(dev_handle_t dev){
    my_term.dirty = false;
    return;
}

static void terminal_reset(dev_handle_t dev, bool hard) {
    my_term.dirty=false;
    if (hard){
        for (uint8_t i = my_term.first; i < my_term.first + my_term.cnt; i++) {
            my_term.state[i] = OFF;
        }
    }
	return;
}

kybd_t terminal_dev = {
		.init = &terminal_init,
		.scan = &terminal_scan,
		.reset =&terminal_reset,
		.state = &terminal_state,
        .isdirty = &terminal_isdirty,
        .undirty = &terminal_undirty,
		.dev_type = TERMINAL,
		.cnt = 8,
		.first= 1

};

int8_t terminal_waitForNumber(char **key) {
	static char buffer[LINE_LENGTH];
	memset(buffer, 0, LINE_LENGTH);
	HAL_StatusTypeDef status;
	uint8_t ch = 0xFF;
	bool stay=true;
	int16_t idx = 0;
	while ((ch == 0xff)&&(stay)) {
		status = HAL_UART_Receive(sio.uart, &ch, 1, 0);
		if (status == HAL_OK){
	        if (ch == '\r'){
	        	stay = false;
	        	ch=0xff;
	        }
            if (((ch >= '0') && (ch < '9')) || (ch == 'R')|| (ch=='r') || (ch == '+') || (ch == '-')) {
                buffer[idx++] = ch;
                if ((ch == 'R')|| (ch=='r')){
                	stay = false;
                }
                if (idx>=1){
                	stay = false;
                }
                ch = 0xFF;
            }
		}
	}
	char *stopstring = NULL;
	if ((buffer[0]=='R')|| (buffer[0]=='r')){
	    *key = &buffer[0];
		return -1;
	}
	long long int res = strtol((char*) &buffer, &stopstring, 10);
	if (strlen(stopstring)==0) {
	    *key = &buffer[0];
		return res;
	}
	return -1;
}
