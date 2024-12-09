/*
 * keyboard.c
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#include "common.h"
#include "xpad.h"
#include "keyboard.h"

kybd_t * my_kybd[KYBD_CNT];

kybd_t my_dev[KYBD_CNT];

char* key_state_3c[]={
	"   ",
	"BLI",
	"ON ",
	"NA ",
};
char* key_state_2c[]={
	"  ",
	"BL",
	"ON ",
	"NA ",
};


static int8_t keyboard_find_dev();

kybdh_t keyboard_init(kybd_t *kybd, xpad_t *scan_dev){
	kybd_type_e res=0;
	if (kybd!=NULL){
		res = keyboard_find_dev();
		if (res>EM_ERR){
			my_kybd[res] = kybd;
			if (kybd->dev_type<TERMINAL) {
				kybd->init(res, scan_dev);
			}
		}
	}
	return (kybd_type_e)res;
}

uint16_t keyboard_scan(kybdh_t dev){
	uint16_t res = 0;
	if ((dev>0)&&my_kybd[dev]!=NULL){
		res = my_kybd[dev]->scan(dev);
	}
	return res;
};
void keyboard_reset(kybdh_t dev, bool hard){
	if ((dev>0)&&my_kybd[dev]!=NULL){
		my_kybd[dev]->reset(dev, hard);
	}
	return;
}

void keyboard_state(kybdh_t dev, kybd_r_t *ret){
	if ((dev>0)&&my_kybd[dev]!=NULL){
		my_kybd[dev]->state(dev, ret);
	}
	return;
};
static int8_t keyboard_find_dev(){
	for (uint8_t i=0;i<KYBD_CNT;i++){
		if (my_kybd[i]==NULL) return i+1;
	}
	return EM_ERR;
};
void  keyboard_print(kybd_r_t *state, char* start){
	if (!state){
		printf("%s Nothing returned"NL, start);
		return;
	}
	printf("%s: Label  ", start);
	for (uint8_t i=0;i<state->key_cnt;i++){
	    if (state->value[i]<10){
	        printf(" %c ", '0'+state->value[i]);
	    } else {
            printf(" %c ", 'A'+(state->value[i]-10));
	    }
	}
	printf(NL);
	printf("%s: State  ", start);
	for (uint8_t i=0;i<state->key_cnt;i++){
		char * text = key_state_3c[state->state[i]];
		printf("%s", text);
	}
	printf(NL);
}

kybd_type_e  keyboard_get_dev_type(kybdh_t dev){
	if ((dev>0)&&(dev<TERMINAL)){
		return my_kybd[dev]->dev_type;
	}
	return DEV_TYPE_NA;
}

