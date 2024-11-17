/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "main.h"
#define ZEI_SPA_2_INDEX(zeile, spalte ) (uint8_t)(zeile*SPALTEN_CNT+spalte)
#define MINIMAL_LINESTART 16

static mkey_t reset_key ={0,0,0, true};

xpad_t default_keyboard ={
	.zeile ={ //Output
		{.port=PORTA, .pin=PIN_10, .conf= {.mode=INPUT,  .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}},// Row 1
		{.port=PORTB, .pin=PIN_3,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}}, // Row 2
		{.port=PORTB, .pin=PIN_5,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}}, // Row 3
		{.port=PORTB, .pin=PIN_10, .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}},// Row 4
	},
	.spalte = { // Input
		{.port = PORTC, .pin = PIN_1,  .conf= {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}}, //Col 1
		{.port = PORTB, .pin = PIN_0,  .conf= {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}}, //Col 2
		{.port = PORTA, .pin = PIN_4,  .conf= {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}}, //Col 3
		{.port = PORTA, .pin = PIN_0,  .conf= {.mode=INPUT,.pin= PIN_OD,   .speed =s_HIGH, .pupd = PULL_UP}}, //Col 4
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
	.label =  {0 ,0xf, 0xe, 0xd, 	// Zeile 1
	           7, 8, 9, 0xc,		// Reihe 2
	           4, 5, 6, 0xb,		// Reihe 3
			   1, 2, 3, 0xa},		// Reihe 3
	.lbl2idx = {-1, -1, -1, -1,
			-1, -1, -1, -1,
			-1, -1, -1, -1,
			-1, -1, -1 , -1},
	.first = 0,
	.key_cnt = X_BUTTON_CNT,
	.first = 0,
	.dirty = false,

};

xpad_t* my_xpad[KYBD_CNT];

static void xpad_set_zeile(kybd_h dev, uint8_t zeilen_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on set_zeile"NL,HAL_GetTick());
		return;
	}
	GpioPinWrite(&my_xpad[dev]->zeile[zeilen_nr], GPIO_PIN_SET);
	return;
}
static void xpad_reset_zeile(kybd_h dev, uint8_t zeilen_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on reset_zeile"NL,HAL_GetTick());
		return;
	}
	GpioPinWrite(&my_xpad[dev]->zeile[zeilen_nr], GPIO_PIN_RESET );
	return;
}
static uint8_t  xpad_read_spalte(kybd_h dev, uint8_t zeilen_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on read_row"NL,HAL_GetTick());
		return 0;
	}
	uint8_t res=0;
	for (uint8_t s=0;s<SPALTEN_CNT; s++){
		bool pin=0;
		GpioPinRead(&my_xpad[dev]->spalte[s], &pin);
		pin =!pin;
		uint8_t index = ZEI_SPA_2_INDEX(s, zeilen_nr);
		my_xpad[dev]->key[index].current=pin;
		bool changed = 	my_xpad[dev]->key[index].current^my_xpad[dev]->key[index].last;
		if (changed){
			my_xpad[dev]->key[index].cnt=0;
		}
		my_xpad[dev]->key[index].unstable = pin || my_xpad[dev]->key[index].unstable;
		if (my_xpad[dev]->key[index].unstable){
			my_xpad[dev]->key[index].cnt++;
		}
		if (my_xpad[dev]->key[index].cnt>STABLE_CNT){
            my_xpad[dev]->key[index].unstable = false;
            my_xpad[dev]->key[index].stable = pin;
            uint8_t value = my_xpad[dev]->label[index];
            if (pin){
                my_xpad[dev]->state[index] = ((my_xpad[dev]->state[index]+1)%KEY_STAT_CNT);
                if ((value>=my_xpad[dev]->first) &&(value<my_xpad[dev]->first+my_xpad[dev]->key_cnt)){
					my_xpad[dev]->dirty=true;
					printf("%010ld: Detected value %X @(r= %d, c=%d)",HAL_GetTick(), value, s, zeilen_nr );
            	}
            }
            if (pin==false){
                my_xpad[dev]->state[index] = ((my_xpad[dev]->state[index]+1)%KEY_STAT_CNT);
            } else {
            } if (pin){
                printf(": OK"NL);
            }
            res = res | (pin<<s);
	    }
		my_xpad[dev]->key[index].last = my_xpad[dev]->key[index].current;
	}
	return res;
}


void xpad_init(kybd_h dev, void *xpad){
	if (xpad != NULL){
		my_xpad[dev] = (xpad_t *)xpad;
	} else {
		my_xpad[dev] = &default_keyboard;
	}
	for (uint8_t spa_idx= 0; spa_idx<SPALTEN_CNT; spa_idx++){
		GpioPinInit(&my_xpad[dev]->spalte[spa_idx]);
	}
	for (uint8_t zei_idx= 0; zei_idx<ZEILEN_CNT; zei_idx++){
		GpioPinInit(&my_xpad[dev]->zeile[zei_idx]);
	}
	for (uint8_t z=0;z<ZEILEN_CNT;z++){
		  xpad_set_zeile(dev, z);
	}
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		for (uint8_t j=0;j<X_BUTTON_CNT;j++){
		if (my_xpad[dev]->label[j]==i){
				my_xpad[dev]->lbl2idx[i] = j;
			}
		}
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
	uint8_t i=0;
	for (uint8_t lbl=ret->first; lbl<ret->first+ret->key_cnt; lbl++){
	    uint8_t idx= my_xpad[dev]->lbl2idx[lbl];
        ret->state[i] = my_xpad[dev]->state[idx];
        ret->label[i++] = my_xpad[dev]->label[idx];
	}
	ret->dirty = my_xpad[dev]->dirty;
	return;
}

uint16_t xpad_scan(kybd_h dev){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on scan"NL,HAL_GetTick());
		return false;
	}
	uint16_t res=0;
	for (uint8_t z=0;z<ZEILEN_CNT;z++){
		uint8_t ir=0;
		xpad_reset_zeile(dev, z);
		HAL_Delay(SCAN_MS);
		ir = xpad_read_spalte(dev, z);
		res =  res |( ir<<(4*z));
		xpad_set_zeile(dev, z);
	}
	if (res!=0){
	    printf("%010ld: Key Label is  0x%lX"NL,HAL_GetTick(), (uint32_t)res );
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
		printf("%s", state2str[state->state[i]]);
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

