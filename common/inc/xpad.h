/*
 * keypad.h
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */

#ifndef INC_KEYPAD_H_
#define INC_KEYPAD_H_

#define COL_CNT 4
#define ROW_CNT 4
#define X_BUTTON_CNT (ROW_CNT*COL_CNT)
#define SCAN_MS	5
#define STABLE_CNT 10
typedef enum{
	OFF,
	BLINKING,
	ON,
	KEY_STAT_CNT
}key_state_e;

#define MINIMAL_LINESTART 8

typedef struct key_s{
	bool last;
	bool current;
	uint8_t cnt;
	bool valid;
} mkey_t;

typedef struct xpad_r_s{
	 key_state_e state[X_BUTTON_CNT];
	 char label[X_BUTTON_CNT];
}xpad_r_t;

typedef struct xpad_s{
	 GpioPin_t row[ROW_CNT];
	 GpioPin_t col[COL_CNT];
	 mkey_t key[X_BUTTON_CNT];
	 bool  second[X_BUTTON_CNT]; // True when whithin 10*STABLE_CNT a button was pressed;
	 key_state_e state[X_BUTTON_CNT];
	 uint8_t label[X_BUTTON_CNT];
	 bool dirty;
}xpad_t;

void xpad_init(xpad_t *x_pad);
bool xpad_scan();
void xpad_reset();
xpad_r_t* xpad_state();
void  xpad_print(xpad_r_t *state, char* start); // Show returnd
void  xpad_iprint(xpad_t *state, char* start); // Show internals


#endif /* INC_KEYPAD_H_ */
