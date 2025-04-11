/*
 * xpad.h
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#ifndef XPAD_H_
#define XPAD_H_

#include "common.h"
#include "keyboard.h"
#include "gpio.h"
#include "gpio_port.h"

#define STABLE_CNT 5

typedef struct xpad_dev_s{
	gpio_port_t spalte;
	gpio_port_t zeile;
	dev_type_e dev_type;
	state_t state;
} xpad_dev_t;


typedef struct xpad_s{
	gpio_port_t* zeile;
	gpio_port_t* spalte;
	dev_type_e dev_type;
	mkey_t key[MAX_BUTTON_CNT];
	state_t state;
	int8_t val2idx[MAX_BUTTON_CNT];
	bool dirty;
}xpad_t;

extern kybd_t xscan_dev;
extern kybd_t eight_dev;
void  xpad_iprint(xpad_t *state, char* timestamp);


#endif /* XPAD_H_  */
