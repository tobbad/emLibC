/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "main.h"
#define ZEI_SPA_2_INDEX(zeile, spalte ) (uint8_t)(spalte*ZEILEN_CNT+zeile)
#define MINIMAL_LINESTART 16

static mkey_t reset_key ={0,0,0, true};

xpad_t default_keyboard ={
	.spalte ={ //Output column
		{.port=PORTB, .pin=PIN_10, .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}},// Row 1
		{.port=PORTB, .pin=PIN_5,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}}, // Row 2
		{.port=PORTB, .pin=PIN_3,  .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}}, // Row 3
		{.port=PORTA, .pin=PIN_10, .conf= {.mode=OUTPUT, .pin=PIN_PP,  .speed=s_HIGH, .pupd=PULL_NONE, .af=PIN_AF0}},// Row 4
	},
	.zeile = { // Input row
		{.port = PORTA, .pin = PIN_0,  .conf= {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}}, //Col 1
		{.port = PORTA, .pin = PIN_4,  .conf= {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}}, //Col 3
		{.port = PORTB, .pin = PIN_0,  .conf= {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}}, //Col 2
		{.port = PORTC, .pin = PIN_1,  .conf= {.mode=INPUT, .pin= PIN_OD,  .speed =s_HIGH, .pupd = PULL_UP}}, //Col 4
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
	.label =  {1, 2,   3,   0xa , // Zeile 1
	           4, 5,   6,   0xb,  // Zeile 2
	           7, 8,   9,   0xc,  // Zeile 3
			   0, 0xf, 0xe, 0xd}, // Zeile 4

	.val2idx = {-1, -1, -1, -1,
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
static void xpad_set_spalte(kybd_h dev, uint8_t spalten_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on xpad_set_spalte"NL,HAL_GetTick());
		return;
	}
	GpioPinWrite(&my_xpad[dev]->spalte[spalten_nr], GPIO_PIN_SET);
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
static void xpad_reset_spalte(kybd_h dev, uint8_t spalten_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on xpad_reset_spalte"NL,HAL_GetTick());
		return;
	}
	GpioPinWrite(&my_xpad[dev]->spalte[spalten_nr], GPIO_PIN_RESET );
	return;
}
static uint8_t  xpad_read_spalte(kybd_h dev, uint8_t zeilen_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on read_spalten"NL,HAL_GetTick());
		return 0;
	}
	uint8_t res=0;
	for (uint8_t s=0;s<SPALTEN_CNT; s++){
		bool pin=0;
		GpioPinRead(&my_xpad[dev]->spalte[s], &pin);
		pin =!pin;
		uint8_t index = ZEI_SPA_2_INDEX(s, zeilen_nr);
		my_xpad[dev]->key[index].current=pin;
		my_xpad[dev]->key[index].unstable = 	my_xpad[dev]->key[index].current^my_xpad[dev]->key[index].last;
		if (my_xpad[dev]->key[index].unstable){
			my_xpad[dev]->key[index].cnt=0;
			my_xpad[dev]->key[index].last = my_xpad[dev]->key[index].current;
            res = res | (pin<<s);
			continue;
		}
		if (my_xpad[dev]->key[index].unstable&&(my_xpad[dev]->key[index].current==my_xpad[dev]->key[index].last)){
			my_xpad[dev]->key[index].cnt++;
		}
		if (my_xpad[dev]->key[index].cnt>STABLE_CNT){
			// We got a stable state
            uint8_t value = my_xpad[dev]->label[index];
            my_xpad[dev]->key[index].last = pin;
            my_xpad[dev]->key[index].current=pin;
            if (pin){
                my_xpad[dev]->state[index] = ((my_xpad[dev]->state[index]+1)%KEY_STAT_CNT);
				my_xpad[dev]->dirty=true;
				printf("%010ld: Pushed   value %X @(r= %d, c=%d)",HAL_GetTick(), value, s, zeilen_nr );
            } else{
				printf("%010ld: Released value %X @(r= %d, c=%d)",HAL_GetTick(), value, s, zeilen_nr );
            }
            my_xpad[dev]->key[index].stable=pin;
	    }
        res = res | (pin<<s);
		my_xpad[dev]->key[index].last = my_xpad[dev]->key[index].current;
	}
	return res;
}
static uint8_t  xpad_read_zeile(kybd_h dev, uint8_t spalten_nr){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on read_zeile"NL,HAL_GetTick());
		return 0;
	}
	uint8_t res=0;
	for (uint8_t z=0;z<SPALTEN_CNT; z++){
		bool pin=0;
		GpioPinRead(&my_xpad[dev]->zeile[z], &pin);
		pin =!pin;
		uint8_t index = ZEI_SPA_2_INDEX(spalten_nr, z);
		my_xpad[dev]->key[index].current=pin;
		my_xpad[dev]->key[index].unstable = 	my_xpad[dev]->key[index].current^my_xpad[dev]->key[index].last;
		if (my_xpad[dev]->key[index].unstable){
			my_xpad[dev]->key[index].cnt=0;
			my_xpad[dev]->key[index].last = my_xpad[dev]->key[index].current;
            res = res | (pin<<z);
			continue;
		}
		if (my_xpad[dev]->key[index].unstable&&(my_xpad[dev]->key[index].current==my_xpad[dev]->key[index].last)){
			my_xpad[dev]->key[index].cnt++;
		}
		if (my_xpad[dev]->key[index].cnt>STABLE_CNT){
			// We got a stable state
            uint8_t value = my_xpad[dev]->label[index];
            my_xpad[dev]->key[index].last = pin;
            my_xpad[dev]->key[index].current=pin;
            if (pin){
                my_xpad[dev]->state[index] = ((my_xpad[dev]->state[index]+1)%KEY_STAT_CNT);
				my_xpad[dev]->dirty=true;
				printf("%010ld: Pushed   value %X @(z= %d, s=%d)",HAL_GetTick(), value, z, spalten_nr );
            } else{
				printf("%010ld: Released value %X @(z= %d, s=%d)",HAL_GetTick(), value, z, spalten_nr );
            }
            my_xpad[dev]->key[index].stable=pin;
	    }
        res = res | (pin<<z);
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
		GpioPinInit(&my_xpad[dev]->spalte[spa_idx]); //output
	}
	for (uint8_t zei_idx= 0; zei_idx<ZEILEN_CNT; zei_idx++){
		GpioPinInit(&my_xpad[dev]->zeile[zei_idx]);// input
	}
	for (uint8_t z=0;z<ZEILEN_CNT;z++){
		  xpad_set_zeile(dev, z);
	}
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		for (uint8_t j=0;j<X_BUTTON_CNT;j++){
		if (my_xpad[dev]->label[j]==i){
				my_xpad[dev]->val2idx[i] = j;
			}
		}
	}
	printf(NL);
	printf("Value  Index"NL);
	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%5X ", i);
		printf(" -> %5X"NL, my_xpad[dev]->val2idx[i]);
	}
}

void  xpad_state(kybd_h dev, kybd_r_t *ret){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on state"NL,HAL_GetTick());
		return;
	}
	uint8_t i=0;
	for (uint8_t val=ret->first; val<ret->first+ret->key_cnt; val++){
	    uint8_t idx= my_xpad[dev]->val2idx[val];
        ret->state[i] = my_xpad[dev]->state[idx];
        ret->label[i++] = my_xpad[dev]->label[idx];
	}
	ret->dirty = my_xpad[dev]->dirty;
	return;
}

uint16_t xpad_spalten_scan(kybd_h dev){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on scan"NL,HAL_GetTick());
		return false;
	}
	uint16_t res=0;
	for (uint8_t s=0;s<SPALTEN_CNT;s++){
		uint8_t ir=0;
		xpad_reset_spalte(dev, s);
		HAL_Delay(SETTLE_TIME_MS);
		ir = xpad_read_zeile(dev, s);
		res =  res |( ir<<(4*s));
		xpad_set_spalte(dev, s);
	}
	if (res!=0){
	    printf("%010ld: Key Label is  0x%lX"NL,HAL_GetTick(), (uint32_t)res );
	}
	return res;
}
uint16_t xpad_zeilen_scan(kybd_h dev){
	if (my_xpad[dev]==NULL){
		printf("%010ld: No valid handle on scan"NL,HAL_GetTick());
		return false;
	}
	uint16_t res=0;
	for (uint8_t z=0;z<ZEILEN_CNT;z++){
		uint8_t ir=0;
		xpad_reset_zeile(dev, z);
		HAL_Delay(SETTLE_TIME_MS);
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
	.scan = &xpad_spalten_scan,
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
	snprintf(text,maxcnt, "%s%s", start, "Last    " );

	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%03d", state->key[i].last);
	}
	printf(NL);
	snprintf(text, maxcnt, "%s%s", start, "Current  " );

	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%03d", state->key[i].current);
	}
	printf(NL);
	snprintf(text,maxcnt, "%s%s", start, "Cnt      " );

	for (uint8_t i=0;i<X_BUTTON_CNT;i++){
		printf("%03d", state->key[i].cnt);
	}
	printf(NL);

}

