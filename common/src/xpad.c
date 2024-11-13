/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "main.h"
#define COL_ROW_2_INDEX(row, col ) (uint8_t)(col*ROW_CNT+row)
#define MINIMAL_LINESTART 16

static mkey_t reset_key ={0,0,0, true};

xpad_t default_keylbl2idx ={
	.row ={ //Inputs
		{.port=PORTA, .pin=PIN_8, .conf= {.mode=INPUT,  .pin =PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port=PORTB, .pin=PIN_4, .conf= {.mode=INPUT,  .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port=PORTB, .pin=PIN_5, .conf= {.mode=INPUT,  .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
		{.port=PORTB, .pin=PIN_3, .conf= {.mode=INPUT,  .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}},
	},
	.col = { // Outputs
		{.port = PORTA, .pin = PIN_0,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}} ,
		{.port = PORTA, .pin = PIN_4,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}} ,
		{.port = PORTB, .pin = PIN_0,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}} ,
		{.port = PORTC, .pin = PIN_1,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}}
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
	.label =  {0xa, 3, 2, 1,
	           0xb, 6, 5, 4,
	           0xc, 9, 8, 7,
			   0xd, 0xe, 0xf, 0 },
	.lbl2idx = {-1, -1, -1, -1,
			-1, -1, -1, -1,
			-1, -1, -1, -1,
			-1, -1, -1 , -1},
	.first = 1,
	.key_cnt = X_BUTTON_CNT,
	.first = 1,
	.dirty = false,

};

xpad_t* my_xpad[KYBD_CNT];

static uint8_t xpad_read_row(kybd_h dev, uint8_t col_nr);
static void xpad_set_col(kybd_h dev, uint8_t col_nr);
static void xpad_reset_col(kybd_h dev, uint8_t col_nr);


void xpad_init(kybd_h dev, void *xpad){
	if (xpad != NULL){
		my_xpad[dev] = (xpad_t *)xpad;
	} else {
		my_xpad[dev] = &default_keylbl2idx;
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
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		uint8_t label= my_xpad[dev]->label[i];
		my_xpad[dev]->lbl2idx[i] = label;
	}
	printf(NL);
	printf("Label  Index"NL);
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%5X ", i);
		printf(" -> %5X"NL, my_xpad[dev]->lbl2idx[i]);
	}
}

void  xpad_state(kybd_h dev, kybd_r_t *ret){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on state"NL,HAL_GetTick());
		return;
	}
	for (uint8_t lbl=ret->first; lbl<ret->first+ret->key_cnt; lbl++){
	    uint8_t idx= my_xpad[dev]->lbl2idx[lbl];
	    if (!my_xpad[dev]->key[idx].stable) continue;
        ret->state[idx-ret->first] = my_xpad[dev]->state[idx];
	}
	return;
}

uint16_t xpad_scan(kybd_h dev){
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
	if (res!=0){
	    printf("%010ld: Key state is 0x%04X"NL,HAL_GetTick(), res );
	}
	return res;
}

void xpad_reset(kybd_h dev){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on reset"NL,HAL_GetTick());
		return;
	}
	my_xpad[dev]->dirty=  false;
    for (uint8_t i=0; i<X_BUTTON_CNT; i++){
    	my_xpad[dev]->state[i] = OFF;
    	my_xpad[dev]->key[i] =reset_key;
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
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf(" %C ", state->label[i]);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "State   " );
	printf(text);
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%s", key_state_c[state->state[i]]);
	}
	printf(NL);
	snprintf(text,maxcnt, "%s%s", start, "last    " );

	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%03d", state->key[i].last);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "current  " );

	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%03d", state->key[i].current);
	}
	printf(NL);
	snprintf(text,maxcnt, "%s%s", start, "cnt      " );

	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%03d", state->key[i].cnt);
	}
	printf(NL);

}

static uint8_t  xpad_read_row(kybd_h dev, uint8_t col_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on read_row"NL,HAL_GetTick());
		return 0;
	}
	uint8_t res=0;
	for (uint8_t r=0;r<ROW_CNT; r++){
		bool pin=0;
		GpioPinRead(&my_xpad[dev]->row[r], &pin);
		pin =!pin;
		uint8_t index = COL_ROW_2_INDEX(r,col_nr);
		my_xpad[dev]->key[index].current=pin;
		bool changed = my_xpad[dev]->key[index].current^my_xpad[dev]->key[index].last;
		if (changed){
			my_xpad[dev]->key[index].cnt=0;
		}
		my_xpad[dev]->key[index].unstable = pin || my_xpad[dev]->key[index].unstable;
		if (my_xpad[dev]->key[index].unstable){
			my_xpad[dev]->key[index].cnt++;
		}
		if (my_xpad[dev]->key[index].cnt>STABLE_CNT){
            my_xpad[dev]->key[index].unstable = false;
            my_xpad[dev]->key[index].stable = true;
            uint8_t value = my_xpad[dev]->label[index];
            if (pin){
                my_xpad[dev]->state[index] = ((my_xpad[dev]->state[index]+1)%KEY_STAT_CNT);
                printf("%010ld: Detected value %d @(r= %d, c=%d)",HAL_GetTick(), value, r, col_nr );
            }
            my_xpad[dev]->dirty=true;
            if (pin==false){
                my_xpad[dev]->state[index] = ((my_xpad[dev]->state[index]+1)%KEY_STAT_CNT);
            } else {
            }
            if (pin){
                printf(": OK"NL);
            }
            res = res | (pin<<r);
	    }


		my_xpad[dev]->key[index].last = my_xpad[dev]->key[index].current;
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


