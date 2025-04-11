/*
 * keyboard.h
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#define MAX_BUTTON_CNT 16
#include "common.h"
#include "state.h"
#include "device.h"

typedef struct xpad_s xpad_t;

#define SETTLE_TIME_MS	1
#define SCAN_MS	5


extern char* key_state_3c[];
extern char* key_state_2c[];

typedef struct key_s{
	bool last;
	bool current;
	bool unstable;
	uint8_t cnt;
	bool stable;
} mkey_t;

typedef struct kybd_s{
	void (*init)(dev_handle_t dev, dev_type_e dev_type, xpad_t *device);
	uint16_t (*scan)(dev_handle_t dev);
	void (*reset)(dev_handle_t dev, bool hard);
	void (*state)(dev_handle_t dev, state_t *ret);
	dev_type_e dev_type;
	uint8_t cnt;
	uint8_t first;
} kybd_t;

extern mkey_t reset_key;


dev_handle_t keyboard_init(kybd_t *kybd, xpad_t *device);
int16_t keyboard_scan(dev_handle_t dev);
void keyboard_reset(dev_handle_t dev, bool hard);
void keyboard_state(dev_handle_t dev, state_t *ret);
void  keyboard_print(state_t *state, char* start); // Show returned
dev_type_e  keyboard_get_dev_type(dev_handle_t dev);



#endif /* KEYBOARD_H_ */
