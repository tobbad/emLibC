/*
 * keypad.h
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */

#ifndef INC_XPAD_H_
#define INC_XPAD_H_
#include "keyboard.h"

#define ZEILEN_CNT 4
#define SPALTEN_CNT 4
#define X_BUTTON_CNT (ZEILEN_CNT*SPALTEN_CNT)

typedef struct xpad_s{
	GpioPin_t zeile[ZEILEN_CNT];
	GpioPin_t spalte[SPALTEN_CNT];
	mkey_t key[X_BUTTON_CNT];
	key_state_e state[X_BUTTON_CNT];
	uint8_t value[X_BUTTON_CNT];
	int8_t val2idx[X_BUTTON_CNT];
	uint8_t key_cnt;
	uint8_t first;
	bool dirty;
}xpad_t;

extern kybd_t xscan_dev;

void xpad_init(kybd_h dev, void *x_pad);
uint16_t xpad_scan(kybd_h handle);
void xpad_reset(kybd_h handle, bool hard);
void  xpad_state(kybd_h handle, kybd_r_t *ret);
void  xpad_iprint(xpad_t *state, char* start);


#endif /* INC_XPAD_H_ */
