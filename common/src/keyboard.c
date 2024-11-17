/*
 * keyboard.c
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#include "main.h"

kybd_t * my_kybd[KYBD_CNT];

char* key_state_c[]={
	"  ",
	"BL",
	"ON",
	"NA",
};


static int8_t keyboard_find_dev();

kybd_h keyboard_init(kybd_t *kybd, void * user_data){
	kybd_h res=EMLIB_ERROR;
	if (kybd!=NULL){
		res = keyboard_find_dev();
		if (res>EMLIB_OK){
			my_kybd[res] = kybd;
			if (kybd->dev_type<DEV_TYPE_CNT) {
				kybd->init(res, user_data);
			}
		}
	}
	return res;
}

uint16_t keyboard_scan(kybd_h dev){
	uint16_t res = 0;
	if (my_kybd[dev]!=NULL){
		res = my_kybd[dev]->scan(dev);
	}
	return res;
};
void keyboard_reset(kybd_h dev){
	if (my_kybd[dev]!=NULL){
		my_kybd[dev]->reset(dev);
	}
	return;
}

void keyboard_state(kybd_h dev, kybd_r_t *ret){
	if (my_kybd[dev]!=NULL){
		my_kybd[dev]->state(dev, ret);
	}
	return;
};
static int8_t keyboard_find_dev(){
	for (uint8_t i=0;i<KYBD_CNT;i++){
		if (my_kybd[i]==NULL) return i+1;
	}
	return EMLIB_ERROR;
};
void  keyboard_print(kybd_r_t *state, char* start){
	if (!state){
		printf("%s Nothing returned"NL, start);
		return;
	}
	printf("%s: Label ", start);
	for (uint8_t i=0;i<state->key_cnt;i++){
	    if (state->label[i]<10){
	        printf(" %c ", '0'+state->label[i]);
	    } else {
            printf(" %c ", 'A'+(state->label[i]-10));
	    }
	}
	printf(NL);
	printf("%s: State ", start);
	for (uint8_t i=0;i<state->key_cnt;i++){
		char * text = key_state_c[state->state[i]];
		printf("%s", text);
	}
	printf(NL);

}
