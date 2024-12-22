/*
 * keypad.h
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */

#ifndef INC_XPAD_H_
#define INC_XPAD_H_
#include "gpio.h"
#include "keyboard.h"
#define ZEILEN_CNT 4
#define SPALTEN_CNT 4
#define EIGHT_BUTTON_CNT 8
#define MAX_BUTTON_CNT 16

typedef struct xpad_pins_s{
	uint8_t spalten_cnt;
	gpio_pin_t spalte[SPALTEN_CNT];
	uint8_t zeilen_cnt;
	gpio_pin_t zeile[EIGHT_BUTTON_CNT];
	kybd_type_e dev;
}xpad_pins_t;


typedef struct xpad_s{
	xpad_pins_t* pins;
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


#endif /* INC_XPAD_H_ */
