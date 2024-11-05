/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "main.h"
#define COL_ROW_2_INDEX(col, row) (uint8_t)(col*COL_CNT+row)
#define MINIMAL_LINESTART 16

static mkey_t reset_key ={0,0,0, true};

xpad_t default_keymap ={
	.row ={ //Inputs
		{.port=PORTB, .pin=PIN_10, .conf= {.mode=INPUT, .pin =PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port=PORTB, .pin=PIN_4, .conf= {.mode=INPUT,  .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port=PORTB, .pin=PIN_5, .conf= {.mode=INPUT,  .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port=PORTB, .pin=PIN_3, .conf= {.mode=INPUT,  .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
	},
	.col = { // Outputs
		{.port = PORTA, .pin = PIN_0,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}} ,
		{.port = PORTA, .pin = PIN_1,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}} ,
		{.port = PORTA, .pin = PIN_4,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}} ,
		{.port = PORTB, .pin = PIN_0,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}}
	},
	.key = { {false,false ,0,false}, {false,false,0,false},  {false,false,0,false}, {false,false,0,false},
			 {false,false,0,false},  {false,false,0,false},  {false,false,0,false},  {false,false,0,false},
			 {false,false,0,false},  {false,false,0,false},  {false,false,0,false},  {false,false,0,false},
			 {false,false,0,false},  {false,false,0,false},  {false,false,0,false},  {false,false,0,false}
	},
	.state = {OFF, OFF, OFF, OFF,
			  OFF, OFF, OFF, OFF,
			  OFF, OFF, OFF, OFF,
			  OFF, OFF, OFF, OFF},
	.label = {0xd, 0xe, 0xf, 0,
			  0xc, 9, 8, 7,
			  0xb, 6, 5, 4,
			  0xA, 3, 2, 1 },
	.map = {-1, -1, -1, -1,
			-1, -1, -1, -1,
			-1, -1, -1, -1,
			-1, -1, -1 , -1},
	.key_cnt = X_BUTTON_CNT,
	.dirty = false,

};

xpad_t* my_xpad[KYBD_CNT];

static uint8_t xpad_read_row(kybd_h dev, uint8_t c);
static void xpad_set_col(kybd_h dev, uint8_t col_nr);
static void xpad_reset_col(kybd_h dev, uint8_t col_nr);


void xpad_init(kybd_h dev, void *xpad){
	if (xpad != NULL){
		my_xpad[dev] = (xpad_t *)xpad;
	} else {
		my_xpad[dev] = &default_keymap;
	}
	for (uint8_t col_idx= 0; col_idx<COL_CNT; col_idx++){
		GpioPinInit(&my_xpad[dev]->col[col_idx]);
	}
	for (uint8_t row_idx= 0; row_idx<ROW_CNT; row_idx++){
		GpioPinInit(&my_xpad[dev]->row[row_idx]);
	}
	for (uint8_t c=0;c<COL_CNT;c++){
		  xpad_set_col(dev, c);
	}
	for (uint8_t i=0;i<BUTTON_CNT;i++){
		my_xpad[dev]->map[my_xpad[dev]->label[i]] = i;
	}
	for (uint8_t i=0;i<BUTTON_CNT;i++){
		printf("%01X ->", i);
		printf(" -> %01X"NL, my_xpad[dev]->map[i]);
	}
}

kybd_r_t* xpad_state(kybd_h dev){
	static kybd_r_t ret;
	uint8_t i=0;
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on state"NL,HAL_GetTick());
		return false;
	}
	if (!(my_xpad[dev]->dirty)) {
		return NULL;
	}
	for (i=0; i<BUTTON_CNT; i++){
		ret.label[i] = my_xpad[dev]->label[i];
		ret.state[i] = my_xpad[dev]->state[i];
	}
	ret.key_cnt = my_xpad[dev]->key_cnt;
	return &ret;
}

bool xpad_scan(kybd_h dev){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on scan"NL,HAL_GetTick());
		return false;
	}
	uint16_t res=0;
	for (uint8_t c=0;c<COL_CNT;c++){
		uint8_t ir=0;
		xpad_reset_col(dev, c);
		HAL_Delay(SCAN_MS);
		ir = xpad_read_row(dev, c);
		res =  res |( ir<<(4*c));
		xpad_set_col(dev, c);
	}
	return my_xpad[dev]->dirty;
}

void xpad_reset(kybd_h dev){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on reset"NL,HAL_GetTick());
		return;
	}
	my_xpad[dev]->dirty=  false;
	for (uint8_t idx=0; idx<BUTTON_CNT; idx++){
		my_xpad[dev]->key[idx] = reset_key;
	}
	return;
}

kybd_t xscan_dev ={
	.init = &xpad_init,
	.scan = &xpad_scan,
	.reset = &xpad_reset,
	.state = &xpad_state,
	.dev_type = XSCAN,
};


void  xpad_iprint(xpad_t *state, char* start){
	const uint8_t maxcnt = MINIMAL_LINESTART+7;
	char text[maxcnt+1];
	if (!state){
		printf("%s Nothing returned"NL, start);
		return;
	}
	snprintf(text,maxcnt, "%s%s", start, "Label  " );
	printf(text);
	for (uint8_t i=0;i<BUTTON_CNT;i++){
		printf(" %C ", state->label[i]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "State   " );
	printf(text);
	for (uint8_t i=0;i<BUTTON_CNT;i++){
		printf("%s", key_state_c[state->state[i]]);
	}
	printf(NL);
	snprintf(text,maxcnt, "%s%s", start, "last    " );

	for (uint8_t i=0;i<BUTTON_CNT;i++){
		printf("%03d", state->key[i].last);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "current  " );

	for (uint8_t i=0;i<BUTTON_CNT;i++){
		printf("%03d", state->key[i].current);
	}
	printf(NL);
	snprintf(text,maxcnt, "%s%s", start, "cnt      " );

	for (uint8_t i=0;i<BUTTON_CNT;i++){
		printf("%03d", state->key[i].cnt);
	}
	printf(NL);

}

static uint8_t  xpad_read_row(kybd_h dev, uint8_t c){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on read_row"NL,HAL_GetTick());
		return false;
	}
	uint8_t res=0;
	for (uint8_t r=0;r<ROW_CNT; r++)	{
		bool pin=0;
		GpioPinRead(&my_xpad[dev]->row[r], &pin);
		pin =!pin;
		res = res | (pin<<r);
		uint8_t index = COL_ROW_2_INDEX(r,c);
		my_xpad[dev]->key[index].current=pin;
		bool this = my_xpad[dev]->key[index].current^my_xpad[dev]->key[index].last;
		if (this){
			my_xpad[dev]->key[index].cnt=0;
		}
		my_xpad[dev]->key[index].unstable =pin || my_xpad[dev]->key[index].unstable;
		if (my_xpad[dev]->key[index].unstable){
			my_xpad[dev]->key[index].cnt++;
		}
		if (my_xpad[dev]->key[index].cnt>STABLE_CNT){
			uint8_t value = my_xpad[dev]->map[index];
	    	if (value<my_xpad[dev]->key_cnt){
				my_xpad[dev]->dirty=true;
				if (pin==false){
					my_xpad[dev]->state[index] = ((my_xpad[dev]->state[index]+1)%KEY_STAT_CNT);
				}
	    	}
		}
	}
	return res;
}

static void xpad_set_col(kybd_h dev, uint8_t col_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on set_col"NL,HAL_GetTick());
		return;
	}
	GpioPinWrite(&my_xpad[dev]->col[col_nr], GPIO_PIN_SET);
	return;
}
static void xpad_reset_col(kybd_h dev, uint8_t col_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on reset_col"NL,HAL_GetTick());
		return;
	}
	GpioPinWrite(&my_xpad[dev]->col[col_nr], GPIO_PIN_RESET );
	return;
}


