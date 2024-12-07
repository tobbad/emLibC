/*
 * keyboard.h
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#ifndef COMMON_INC_KEYBOARD_H_
#define COMMON_INC_KEYBOARD_H_
#include "common.h"
#include "gpio.h"

#define STABLE_CNT 10
#define SETTLE_TIME_MS	1
#define SCAN_MS	5
#define KYBD_CNT 4
#define BUTTON_CNT 16

typedef enum{
	XSCAN,
	TERMINAL,
	EIGHTKEY,
	DEV_TYPE_CNT
} kybd_type_e;

typedef enum{
	OFF,
	BLINKING,
	ON,
	KEY_STAT_CNT
}key_state_e;

extern char* key_state_c[];

typedef struct key_s{
	bool last;
	bool current;
	bool unstable;
	uint8_t cnt;
	bool stable;
} mkey_t;


typedef struct kybd_r_s{
	 key_state_e state[BUTTON_CNT];
	 uint8_t  value[BUTTON_CNT];
     uint8_t key_cnt;
	 uint8_t first; //First valid value
	 bool dirty;
}kybd_r_t;

typedef int8_t kybd_h;

typedef struct kybd_s{
	void (*init)(kybd_h handle, void* kybd);
	uint16_t (*scan)(kybd_h handle);
	void (*reset)(kybd_h handle, bool hard);
	void  (*state)(kybd_h handle, kybd_r_t *ret);
	void *user_data;
	uint8_t first;
	uint8_t button_cnt;
	kybd_type_e dev_type;
}kybd_t;

kybd_h keyboard_init(kybd_t *kybd, void *user_data);
uint16_t keyboard_scan(kybd_h handle);
void keyboard_reset(kybd_h handle, bool hard);
void keyboard_state(kybd_h handle, kybd_r_t *ret);
void  keyboard_print(kybd_r_t *state, char* start); // Show returnd
void  keyboard_iprint(kybd_r_t *state, char* start); // Show internals



#endif /* COMMON_INC_KEYBOARD_H_ */
