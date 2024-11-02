/*
 * 8key.h
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */

#ifndef COMMON_INC_EIGHTKEY_H_
#define COMMON_INC_EIGHTKEY_H_

typedef struct eight_s{
	GpioPin_t bttn_pin[BUTTON_CNT];
	mkey_t key[BUTTON_CNT];
	key_state_e state[BUTTON_CNT];
	uint8_t label[BUTTON_CNT];
	bool dirty;
}eight_t;

extern kybd_t eight_dev;

void eight_init(kybd_h dev, void *data);
bool xeight_scan(kybd_h handle);
void xeight_reset(kybd_h handle);
kybd_r_t* eight_state(kybd_h handle);
void  eight_iprint(xpad_t *state, char* start);




#endif /* COMMON_INC_EIGHTKEY_H_ */
