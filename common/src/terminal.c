/*
 * terminal.c
 *
 *  Created on: Jun 10, 2024
 *      Author: TBA
 */
#include "main.h"
#include "common.h"
#include "device.h"
#include "state.h"
#include <keyboard.h>

static state_t my_term = {
    .first = 1, //First valid value
    .cnt =8,
    .state  = { OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF },
    .label =  { 'R', '1', '2', '3', '4', '5', '6', '7', '8' },
};
static uint8_t ch=0xff;
rxData = false;
static bool check_key(char ch);
static void terminal_reset(dev_handle_t dev, bool hard);

static void terminal_init(dev_handle_t handle, dev_type_e dev_type,	xpad_t *device) {
	terminal_reset(handle, true);
}

static bool check_key(char ch) {
    bool ret = false;
    return state_ch2idx(&my_term, ch)>=0;
}

static uint16_t terminal_scan(dev_handle_t dev) {
	static bool asked = false;
	uint16_t res = 0;
	//char allowed_keys={'R'};

	if (!asked) {
		asked = true;
		printf("Please enter key"NL);
	}
    HAL_StatusTypeDef status;
    HAL_UARTEx_ReceiveToIdle_IT(&huart2, ch, LINE_LENGTH);
	while (rxData == false){}
	rxData = false;
	ch = toupper(ch);
	int8_t idx=state_ch2idx(&my_term, ch);
	if (idx>=0){
		res = ch - '0';
		if (res<9){
			state_propagate(&my_term, ch);
			my_term.clabel = 0;
		} else {
			my_term.clabel = ch;
			my_term.state[0] = ON;
		}
	}
	asked = false;
	return res;
}

static void terminal_state(dev_handle_t dev, state_t *ret) {
    state_merge(&my_term, ret);// problem here
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
		.dev_type = TERMINAL,
		.cnt = 8,
		.first= 1

};

int8_t terminal_waitForNumber(char **key) {
	static uint8_t buffer[LINE_LENGTH];
	memset(buffer, 0, LINE_LENGTH);
	HAL_StatusTypeDef status;
	bool stay=true;
	int16_t idx = 0;
	while ((ch == 0xff)&&(stay)) {
		status = HAL_UARTEx_ReceiveToIdle_IT(&huart2, &ch, LINE_LENGTH);
		while (rxData == false){}
		rxData = false;
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

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  rxData= true;
  HAL_UARTEx_ReceiveToIdle_IT(&huart2, (uint8_t*)&ch, 1);
}
