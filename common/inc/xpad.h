/*
 * xpad.h
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#ifndef XPAD_H_
#define XPAD_H_

#include "keyboard.h"
#include "gpio.h"
#include "gpio_port.h"
#define ZEILEN_CNT 4
#define SPALTEN_CNT 4
#define EIGHT_BUTTON_CNT 8
#define MAX_BUTTON_CNT 16

typedef struct xpad_dev_s{
	gpio_port_t spalte;
	gpio_port_t zeile;
	kybd_type_e dev_type;
	uint8_t value[MAX_BUTTON_CNT];
} xpad_dev_t;


typedef struct xpad_s{
	gpio_port_t* zeile;
	gpio_port_t* spalte;
	kybd_type_e dev_type;
	mkey_t key[MAX_BUTTON_CNT];
	key_state_e state[MAX_BUTTON_CNT];
	uint8_t value[MAX_BUTTON_CNT];
	int8_t val2idx[MAX_BUTTON_CNT];
	uint8_t key_cnt;
	uint8_t first;
	bool dirty;
}xpad_t;

extern kybd_t xscan_dev;
extern kybd_t eight_dev;
void  xpad_iprint(xpad_t *state, char* timestamp);


#endif /* XPAD_H_  */
