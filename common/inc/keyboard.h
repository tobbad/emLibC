/*
 * keyboard.h
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "common.h"
#include "state.h"
#include "device.h"
#include "xpad.h"

#define SETTLE_TIME_MS	1
#define SCAN_MS	5

typedef struct xpad_dev_s xpad_dev_t;

extern char* key_state_3c[];
extern char* key_state_2c[];

typedef struct kybd_s{
	void (*init)(dev_handle_t dev, dev_type_e dev_type, void *device);
	uint16_t (*scan)(dev_handle_t dev);
	void (*reset)(dev_handle_t dev, bool hard);
    void (*state)(dev_handle_t dev, state_t *ret);
    bool (*isdirty)(dev_handle_t dev);
    void (*undirty)(dev_handle_t dev);
	dev_type_e dev_type;
	uint8_t cnt;
	uint8_t first;
    int32_t pcnt;
    int32_t plcnt;
} kybd_t;



dev_handle_t keyboard_init(kybd_t *kybd, xpad_dev_t *device);
uint16_t keyboard_scan(dev_handle_t dev);
void keyboard_reset(dev_handle_t dev, bool hard);
void keyboard_state(dev_handle_t dev, state_t *ret);
bool keyboard_isdirty(dev_handle_t dev);
void keyboard_undirty(dev_handle_t dev);
void  keyboard_print(state_t *state, char* start); // Show returned
dev_type_e  keyboard_get_dev_type(dev_handle_t dev);



#endif /* KEYBOARD_H_ */
