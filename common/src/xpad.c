/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "main.h"
static char* x_state_c[]={
	"   ",
	"BLI",
	"ON ",
	"NA ",
};
#define COL_ROW_2_INDEX(col, row) (uint8_t)(col*COL_CNT+row)

static mkey_t reset_key ={0,0,0,false};

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
	.key = { {0,0,0,false}, {0,0,0,false}, {0,0,0,false}, {0,0,0,false}, {0,0,0,false},  {0,0,0,false},  {0,0,0,false},  {0,0,0,false}},
	.state = {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},
	.label = {0xd, 0xe, 0xf, 0,  0xc, 9, 8, 7, 0xb, 6, 5, 4 ,0xA, 3, 2, 1 },
	.dirty = false,
};
xpad_t my_xpad;


static uint8_t xpad_read_row(uint8_t c);
static void xpad_set_col(uint8_t col_nr);
static void xpad_reset_col(uint8_t col_nr);

void xpad_init(xpad_t *x_pad){
	if (x_pad != NULL){
		my_xpad = *x_pad;
	} else {
		my_xpad = default_keymap;
	}
	for (uint8_t col_idx= 0; col_idx<COL_CNT; col_idx++){
		GpioPinInit(&my_xpad.col[col_idx]);
	}
	for (uint8_t row_idx= 0; row_idx<ROW_CNT; row_idx++){
		GpioPinInit(&my_xpad.row[row_idx]);
	}
	for (uint8_t c=0;c<COL_CNT;c++){
		  xpad_set_col(c);
	}
}

static uint8_t xpad_read_row(uint8_t c){
	uint8_t res=0;
	for (uint8_t r=0;r<ROW_CNT; r++)	{
		uint8_t pin=0;
		GpioPinRead(&my_xpad.row[r], &pin);
		pin =!pin;
		res = res | (pin<<r);
		uint8_t index = COL_ROW_2_INDEX(r,c);
		my_xpad.key[index].current=pin;
		if (pin) {
			if (my_xpad.key[index].current!=my_xpad.key[index].last){
				my_xpad.key[index].last = my_xpad.key[index].current;
			}
			if (my_xpad.key[index].last == my_xpad.key[index].current){
				my_xpad.key[index].cnt++;
				if (my_xpad.key[index].cnt>STABLE_CNT){
					my_xpad.key[index].valid = true;
					my_xpad.state[index] = ((my_xpad.state[index]+1)%KEY_STAT_CNT);
					my_xpad.dirty=true;
				}
			} else{
				my_xpad.key[index].cnt = 0;
			}
		}
	}
	return res;
}
static void xpad_set_col(uint8_t col_nr){
	GpioPinWrite(&my_xpad.col[col_nr], GPIO_PIN_SET);
	return;
}
static void xpad_reset_col(uint8_t col_nr){
	GpioPinWrite(&my_xpad.col[col_nr], GPIO_PIN_RESET );
	return;
}

bool xpad_scan(){
	uint16_t res=0;
	for (uint8_t c=0;c<COL_CNT;c++){
		uint8_t ir=0;
		xpad_reset_col(c);
		HAL_Delay(SCAN_MS);
		ir = xpad_read_row(c);
		res =  res |( ir<<(4*c));
		xpad_set_col(c);
	}
	return my_xpad.dirty;
}

xpad_r_t *xpad_state(){
	static xpad_r_t ret;
	uint8_t i=0;
	if (!(my_xpad.dirty)) {
		return NULL;
	}
	for (i=0; i<X_BUTTON_CNT; i++){
		if (my_xpad.label[i]<10){
			ret.label[i] = ('0'+my_xpad.label[i]);
		} else {
			ret.label[i] = ('A'-10+my_xpad.label[i]);
		}
		ret.state[i] = my_xpad.state[i];
	}
	return &ret;
}
void xpad_reset(){
	my_xpad.dirty=  false;
	for (uint8_t idx=0; idx<X_BUTTON_CNT; idx++){
		my_xpad.key[idx] = reset_key;
	}
	return;
}

void  xpad_print(xpad_r_t *state, char* start){
	if (!state){
		printf("%s Nothing returned"NL, start);
		return;
	}
	printf("%s: Label", start);
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf(" %c ", state->label[i]);
	}
	printf(NL);
	printf("%s: State", start);
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%s", x_state_c[state->state[i]]);
	}
	printf(NL);

}

void  xpad_iprint(xpad_t *state, char* start){
	const uint8_t maxcnt = MINIMAL_LINESTART+7;
	char text[maxcnt+1];
	if (!state){
		printf("%s Nothing returned"NL, start);
		return;
	}
	snprintf(text,maxcnt, "%s%s", start, "Label  " );
	printf(text);
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf(" %C ", state->label[i]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "State   " );
	printf(text);
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%s", x_state_c[state->state[i]]);
	}
	printf(NL);
	snprintf(text,maxcnt, "%s%s", start, "key.last" );

	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%03d", state->key[i].last);
	}
	printf(NL);
	snprintf(text,maxcnt, "%s%s", start, "key.cur" );

	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%03d", state->key[i].current);
	}
	printf(NL);
	snprintf(text,maxcnt, "%s%s", start, "key.cnt" );

	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%03d", state->key[i].cnt);
	}
	printf(NL);

}


