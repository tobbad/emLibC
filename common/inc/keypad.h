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
	bool valid;
	bool current;
} mkey_t;
typedef struct keypad_s{
	 GpioPin_t row[ROW_CNT];
	 GpioPin_t col[COL_CNT];
	 mkey_t keys[BUTTON_CNT];
	 uint8_t state[BUTTON_CNT];
	 uint8_t stable_cnt[BUTTON_CNT];
	 char labels[BUTTON_CNT+1];
}keypad_t;
void keypad_init(keypad_t *key_pad);
uint16_t keypad_scan();



#endif /* INC_KEYPAD_H_ */
