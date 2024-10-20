/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "main.h"
keypad_t default_keymap ={
	.row ={ //Inputs
		{.port=PORTB, .pin=PIN_10, .conf= {.mode=INPUT, .pin =PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port=PORTB, .pin=PIN_4, .conf= {.mode=INPUT,  .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port=PORTB, .pin=PIN_5, .conf= {.mode=INPUT,  .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port=PORTB, .pin=PIN_3, .conf= {.mode=INPUT,  .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}}
	},
	.col = { // Outputs
		{.port = PORTA, .pin = PIN_0,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}} ,
		{.port = PORTA, .pin = PIN_1,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}} ,
		{.port = PORTA, .pin = PIN_4,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}} ,
		{.port = PORTB, .pin = PIN_0,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}}
	},
	.keys = { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0},  {0,0,0},  {0,0,0}},
	.state = {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},
	.stable_cnt = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.labels = "0123456789ABCDEF",
};
keypad_t my_keypad;

static uint8_t keypad_read_row();
static void keypad_set_col(uint8_t col_nr);
static void keypad_reset_col(uint8_t col_nr);

void keypad_init(keypad_t *key_pad){
	if (key_pad != NULL){
		my_keypad = *key_pad;
	} else {
		my_keypad = default_keymap;
	}
	for (uint8_t col_idx= 0; col_idx<COL_CNT; col_idx++){
		GpioPinInit(&my_keypad.col[col_idx]);
	}
	for (uint8_t row_idx= 0; row_idx<ROW_CNT; row_idx++){
		GpioPinInit(&my_keypad.row[row_idx]);
	}
	for (uint8_t c=0;c<COL_CNT;c++){
		  keypad_set_col(c);
	}
}

static uint8_t keypad_read_row(){
	uint8_t res=0;
	for (uint8_t r=0;r<ROW_CNT; r++)	{
		uint8_t pin=0;
		GpioPinRead(&my_keypad.row[r], &pin);
		pin =!pin;
		res = res | (pin<<r);
	}
	return res;
}
static void keypad_set_col(uint8_t col_nr){
	GpioPinWrite(&my_keypad.col[col_nr], GPIO_PIN_SET);
	return;
}
static void keypad_reset_col(uint8_t col_nr){
	GpioPinWrite(&my_keypad.col[col_nr], GPIO_PIN_RESET );
	return;
}

uint16_t keypad_scan(){
	uint16_t res=0;
	for (uint8_t c=0;c<COL_CNT;c++){
		uint8_t ir=0;
		keypad_reset_col(c);
		HAL_Delay(SCAN_MS);
		ir = keypad_read_row();
		res =  res |( ir<<(4*c));
		keypad_set_col(c);
	}
	return res;
}

