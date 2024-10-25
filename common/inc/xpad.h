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
#define BUTTON_CNT (ROW_CNT*COL_CNT)
#define SCAN_MS	5
#define STABLE_CNT 4
typedef enum{
	OFF,
	BLINKING,
	ON,
	KEY_STAT_CNT
}key_state_e;

typedef struct key_s{
	bool last;
	bool current;
	uint8_t cnt;
} mkey_t;

typedef struct keypad_s{
	 GpioPin_t row[ROW_CNT];
	 GpioPin_t col[COL_CNT];
	 mkey_t key[BUTTON_CNT];
	 key_state_e state[BUTTON_CNT];
	 uint8_t labels_n[BUTTON_CNT];
	 char labels[BUTTON_CNT+1];
	 uint8_t last[2];
}keypad_t;

typedef struct keypad_r_s{
	 key_state_e state[BUTTON_CNT];
	 char labels[BUTTON_CNT+1];
}keypad_r_t;

void keypad_init(keypad_t *key_pad);
bool keypad_scan();
keypad_r_t* keypad_state();
void  keypad_print(keypad_r_t *state, char* start);



#endif /* INC_KEYPAD_H_ */
