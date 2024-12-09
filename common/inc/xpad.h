/*
 * keypad.h
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */

#ifndef INC_XPAD_H_
#define INC_XPAD_H_
#include "keyboard.h"
#include "gpio.h"
#define ZEILEN_CNT 4
#define SPALTEN_CNT 4
#define EIGHT_BUTTON_CNT 8

typedef struct xpad_s{
	GpioPin_t zeile[EIGHT_BUTTON_CNT];
	GpioPin_t spalte[SPALTEN_CNT];
	mkey_t key[MAX_BUTTON_CNT];
	key_state_e state[MAX_BUTTON_CNT];
	uint8_t value[MAX_BUTTON_CNT];
	int8_t val2idx[MAX_BUTTON_CNT];
	uint8_t key_cnt;
	uint8_t first;
	bool dirty;
	kybd_type_e dev_type;
}xpad_t;

extern kybd_t xscan_dev;
extern kybd_t eight_dev;
void  xpad_iprint(xpad_t *state, char* start);


#endif /* INC_XPAD_H_ */
