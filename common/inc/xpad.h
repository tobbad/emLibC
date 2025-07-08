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
#include "device.h"
#include "gpio.h"
#include "gpio_port.h"

#define STABLE_CNT 5
#define EIGHT_BUTTON_CNT 8
typedef struct kybd_s kybd_t;

typedef struct key_s{
	bool last;
	bool current;
	bool unstable;
	uint8_t cnt;
	bool stable;
} mkey_t;

typedef struct xpad_dev_s{
	gpio_port_t spalte;
	gpio_port_t zeile;
	dev_type_e dev_type;
	mkey_t key[MAX_BUTTON_CNT];
	state_t state;
} xpad_dev_t;

extern kybd_t xscan_dev;
extern kybd_t eight_dev;


#endif /* XPAD_H_  */
