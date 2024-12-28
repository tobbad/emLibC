/*
 * keyboard.h
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#ifndef KEYBOARD_H_
#define KEYBOARD_H_
#include "common.h"

typedef uint8_t kybdh_t;
typedef struct xpad_s xpad_t;
typedef struct xpad_dev_s xpad_dev_t;

#define STABLE_CNT 10
#define SETTLE_TIME_MS	1
#define SCAN_MS	5
#define KYBD_CNT 4
#define MAX_BUTTON_CNT 16

typedef enum{
	XSCAN,
	EIGHTKEY,
	TERMINAL,
	DEV_TYPE_NA
} kybd_type_e;

typedef enum{
	OFF,
	BLINKING,
	ON,
	KEY_STAT_CNT
}key_state_e;

extern char* key_state_3c[];
extern char* key_state_2c[];

typedef struct key_s{
	bool last;
	bool current;
	bool unstable;
	uint8_t cnt;
	bool stable;
} mkey_t;


typedef struct kybd_r_s{
	 key_state_e state[MAX_BUTTON_CNT];
	 uint8_t  value[MAX_BUTTON_CNT];
     uint8_t key_cnt;
	 uint8_t first; //First valid value
	 bool dirty;
}kybd_r_t;


typedef struct kybd_s{
	void (*init)(kybdh_t dev, kybd_type_e dev_type, xpad_dev_t *device);
	uint16_t (*scan)(kybdh_t dev);
	void (*reset)(kybdh_t dev, bool hard);
	void (*state)(kybdh_t dev, kybd_r_t *ret);
	kybd_type_e dev_type;
	uint8_t value[MAX_BUTTON_CNT];
	uint8_t key_cnt;
	uint8_t first;
} kybd_t;

extern mkey_t reset_key;


kybdh_t keyboard_init(kybd_t *kybd, xpad_dev_t *device);
uint16_t keyboard_scan(kybdh_t dev);
void keyboard_reset(kybdh_t dev, bool hard);
void keyboard_state(kybdh_t dev, kybd_r_t *ret);
void  keyboard_print(kybd_r_t *state, char* start); // Show returned
kybd_type_e  keyboard_get_dev_type(kybdh_t dev);



#endif /* KEYBOARD_H_ */
