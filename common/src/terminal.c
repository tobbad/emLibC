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
    .state  = { OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF,  OFF, OFF, OFF, OFF, OFF, OFF, OFF},
    .label =  { ' ', '1', '2', '3', '4', '5', '6', '7', '8',  ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
    .dirty = false,

};
static sio_t _serial;
clabel_u clabel;
#define ZERO4 ((32<<24)+(32<<16)+(32<<8)+32)
bool data_in = false;
static void terminal_reset(dev_handle_t dev, bool hard);

static void terminal_init(dev_handle_t handle, dev_type_e dev_type,	void * serial) {
	terminal_reset(handle, true);
	_serial =*(sio_t*)serial;
	my_term.clabel.cmd=ZERO4;
	state_clear(&my_term);

}

static bool check_key(char ch) {
    return state_ch2idx(&my_term, ch)>=0;
}
//
//static uint16_t terminal_scan(dev_handle_t dev) {
//    char ch;
//	static bool asked = false;
//	int16_t res = -1;
//	//char allowed_keys={'R'};
//
//	if (!asked) {
//		asked = true;
//		printf("Please enter key"NL);
//	}
//	uint16_t rxLen=CMD_LEN;
//    HAL_StatusTypeDef status;
//    data_in=false;
//    //status = HAL_UART_Receive(sio.uart, (uint8_t*)&ch, 1, 0);
//	while (!data_in) {
//		status = HAL_UART_Receive_DMA(sio.uart, (uint8_t*)&clabel.cmd, rxLen);
//	}
//	ch = toupper(my_term.clabel.str[0]);
//	int8_t idx=state_ch2idx(&my_term, ch);
//	if (idx>=0){
//		res = ch - '0';
//		my_term.dirty= true;
//		if (res<9){
//			state_propagate(&my_term, ch);
//			return res;
//		}
//	} else {
//		my_term.clabel.str[CMD_LEN-1]=0;
//		printf("Command %s"NL, my_term.clabel.str);
//	}
//	return res;
//}
static uint16_t terminal_scan(dev_handle_t dev) {
	char ch=0xFF;
	static bool asked = false;
	HAL_StatusTypeDef status;
	static char buffer[LINE_LENGTH];
	bool stay=true;
	if (!asked) {
		asked = true;
		printf("Please enter key"NL);
	}
	memset(buffer, 0, LINE_LENGTH);
	while ((ch == 0xff)&&(stay)) {
		uint8_t idx=0;
		status = HAL_UART_Receive(_serial.uart,(uint8_t*) &ch, 1, 0);
		if (status == HAL_OK){
	        if (ch == '\r'){
	        	stay = false;
	        	ch=0xff;
	        }
            if (((ch >= '0') && (ch < '9')) || (ch == 'R')|| (ch=='r') || (ch == '+') || (ch == '-')) {
                buffer[idx++] = ch;
                if ((ch == 'R')|| (ch=='r')){
                	my_term.clabel.cmd=0;
                	my_term.clabel.str[0]='R';
                	printf("Set string to %s"NL, my_term.clabel.str);
                }
                if (idx>=1){
                	stay = false;
                }
                ch = 0xFF;
            }
		}
	}
	char *stopstring = NULL;
	long long int res = strtol((char*) &buffer, &stopstring, 10);
	if (*stopstring!=0) {
		return res;
	}
	int8_t idx=state_nr2idx(&my_term, res);
	if ((idx>=my_term.first)&&(res<=my_term.first+my_term.cnt)){
		my_term.dirty= true;
		state_propagate(&my_term, idx);
	} else {
		my_term.clabel.str[CMD_LEN-1]=0;
		printf("Command %s"NL, my_term.clabel.str);
	}

	return -1;

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
		status = HAL_UART_Receive(_serial.uart, &ch, 1, 0);
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

//int8_t terminal_waitForNumber(char **key) {
//	static char buffer[LINE_LENGTH];
//	memset(buffer, 0, LINE_LENGTH);
//	data_in = false;
//	while (!data_in) {
//		HAL_UART_Receive_DMA(sio.uart, (uint8_t*)&clabel.str[0], CMD_LEN);
//	}
//	char *stopstring = NULL;
//	if ((clabel.str[0]=='R')|| (clabel.str[0]=='r')){
//	    *key = &clabel.str[0];
//		return -1;
//	}
//	long long int res = strtol((char*) &clabel.str, &stopstring, 10);
//	if (strlen(stopstring)==0) {
//	    *key = &clabel.str[0];
//		return res;
//	}
//	return -1;
//}

void UART_IdleCallback(void)
{
	data_in=true;

    uint16_t receivedLength = CMD_LEN-__HAL_DMA_GET_COUNTER(_serial.uart->hdmarx);

    if (receivedLength > 0 && receivedLength <= 3)    {
        memcpy(&my_term.clabel.cmd, (uint8_t*)&clabel.cmd, receivedLength);
        clabel.cmd=ZERO4;
    }

    HAL_UART_Receive_DMA(_serial.uart, (uint8_t*)&clabel.cmd, CMD_LEN);
}

