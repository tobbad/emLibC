/*
 * keypad.h
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */

#ifndef INC_KEYPAD_H_
#define INC_KEYPAD_H_
#include "keyboard.h"
#define COL_CNT 4
#define ROW_CNT 4
#define X_BUTTON_CNT (ROW_CNT*COL_CNT)

typedef struct xpad_s{
	GpioPin_t row[ROW_CNT];
	GpioPin_t col[COL_CNT];
	mkey_t key[X_BUTTON_CNT];
	key_state_e state[X_BUTTON_CNT];
	uint8_t label[X_BUTTON_CNT];
	int8_t lbl2idx[X_BUTTON_CNT];
	uint8_t key_cnt;
	uint8_t first;
	bool dirty;
}xpad_t;

extern kybd_t xscan_dev;

void xpad_init(kybd_h dev, void *x_pad);
bool xpad_scan(kybd_h handle);
void xpad_reset(kybd_h handle, kybd_r_t *state);
void  xpad_state(kybd_h handle, kybd_r_t *ret);
void  xpad_iprint(xpad_t *state, char* start);


#endif /* INC_KEYPAD_H_ */
