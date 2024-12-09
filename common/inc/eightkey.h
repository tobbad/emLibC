/*
 * 8key.h
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#ifndef COMMON_INC_EIGHTKEY_H_
#define COMMON_INC_EIGHTKEY_H_
#include "common.h"
#include "keyboard.h"
#include "xpad.h"

#define EIGHT_BUTTON_CNT 8

typedef struct eight_s{
	GpioPin_t bttn_pin[EIGHT_BUTTON_CNT];
	mkey_t key[EIGHT_BUTTON_CNT];
	key_state_e state[EIGHT_BUTTON_CNT];
	int8_t value[EIGHT_BUTTON_CNT];
    uint8_t key_cnt;
	uint8_t first;
	bool dirty;
}eight_t;

extern kybd_t eight_dev;
void  eight_iprint(xpad_t *state, char* start);




#endif /* COMMON_INC_EIGHTKEY_H_ */
