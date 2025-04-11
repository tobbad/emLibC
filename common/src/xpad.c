/*
 * keypad.c
 *
 *  Created on: Jun 2, 2024
 *      Author: badi
 */
#include "common.h"
#include "state.h"
#include "device.h"
#include "gpio.h"
#include "gpio_port.h"
#include "keyboard.h"
#include "xpad.h"
#include "state.h"


#define MINIMAL_LINESTART 16
#define ZEILEN_CNT 4
#define SPALTEN_CNT 4
#define EIGHT_BUTTON_CNT 8

xpad_t my_xpad[DEVICE_CNT];
xpad_t *mpy_xpad[DEVICE_CNT];



static xpad_dev_t default_xscan_dev= {
	.spalte= {// Output
		.cnt=SPALTEN_CNT ,
		.pin={
			{	.port=GPIOA, .pin=GPIO_PIN_0, .conf= {.Mode=GPIO_MODE_OUTPUT_PP, .Pull=GPIO_PULLUP,}}, // spalte 1
			{	.port=GPIOA, .pin=GPIO_PIN_4, .conf= {.Mode=GPIO_MODE_OUTPUT_PP, .Pull=GPIO_PULLUP,}}, // spalte 2
			{	.port=GPIOB, .pin=GPIO_PIN_3, .conf= {.Mode=GPIO_MODE_OUTPUT_PP, .Pull=GPIO_PULLUP,}}, // spalte 3
			{	.port=GPIOC, .pin=GPIO_PIN_1, .conf= {.Mode=GPIO_MODE_OUTPUT_PP, .Pull=GPIO_PULLUP,}}, // spalte 4
		},
	},
	.zeile= { // Input
		.cnt=ZEILEN_CNT ,
		.pin={
			{	.port=GPIOA, .pin= GPIO_PIN_10,  .conf= {.Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP,}}, // zeilen 1
			{	.port=GPIOB, .pin= GPIO_PIN_3 ,  .conf= {.Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP,}}, // zeilen 2
			{	.port=GPIOB, .pin= GPIO_PIN_5 ,  .conf= {.Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP,}}, // zeilen 3
			{	.port=GPIOB, .pin= GPIO_PIN_10,  .conf= {.Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP,}}, // zeilen 4
		},
	},
	.dev_type=XSCAN,
    .state = {.label = {'1', '2', '3', 'a', '4', '5', '6', 'b', '7', '8', '9', 'c', '0', 'f', 'e' ,'d'},
              .state={OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},
	          .cnt=ZEILEN_CNT*SPALTEN_CNT,
			  .first = 0,},


};

static xpad_dev_t default_eight_dev = {
	.spalte={{{0}}},
	.zeile ={
		.cnt =EIGHT_BUTTON_CNT,
		.pin = {
	            { .port = GPIOA, .pin = GPIO_PIN_0,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
	            { .port = GPIOA, .pin = GPIO_PIN_4,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
				{ .port = GPIOB, .pin = GPIO_PIN_3,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
				{ .port = GPIOC, .pin = GPIO_PIN_1,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
	            { .port = GPIOA, .pin = GPIO_PIN_10, .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
	            { .port = GPIOB, .pin = GPIO_PIN_3,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
				{ .port = GPIOB, .pin = GPIO_PIN_5,  .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
				{ .port = GPIOB, .pin = GPIO_PIN_10, .conf = { .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLUP } },
		},
	},

	.dev_type=EIGHTKEY,
	.state = {.label = {'1', '2', '3', '4', '5', '6', '7', '8', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
	          .state={OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},
	          .cnt=EIGHT_BUTTON_CNT,
			  .first = 0,
},

};

static uint8_t index_2_zei(xpad_t *kbd, uint8_t index){
	uint8_t res = index%(kbd->zeile->cnt+1);
	return res;

}
static uint8_t index_2_spa(xpad_t *kbd, uint8_t  index){
	uint8_t zei = index_2_zei(kbd, index);
	uint8_t res = index - zei*kbd->spalte->cnt;
	return res;
}

static uint8_t zei_spa_2_index(xpad_t *kbd, uint8_t zeile, uint8_t spalte){
	return (uint8_t)(spalte+zeile*kbd->spalte->cnt);
}


static uint8_t lable2uint8(char label){
	uint8_t value = label -'0';
	if (value>9){
		label &= 0xEF;
		value = label-'a'+10;
	}
	return value;

}



static void xpad_init(dev_handle_t dev, dev_type_e dev_type, xpad_t *device) {
	if (dev_type == DEV_TYPE_NA)
		return;
	my_xpad[dev].dev_type= dev_type;
	mpy_xpad[dev] = &my_xpad[dev];
	if (device != NULL) {
		my_xpad[dev].spalte = device->spalte;
		my_xpad[dev].zeile =  device->zeile;
		memcpy(&my_xpad[dev].state, &device->state, sizeof(state_t));
	} else {
		if (dev_type ==XSCAN) {
			my_xpad[dev].spalte = &default_xscan_dev.spalte;
			my_xpad[dev].zeile  = &default_xscan_dev.zeile;
			my_xpad[dev].state  = default_xscan_dev.state;
	        memcpy(&my_xpad[dev].state, &default_xscan_dev.state,  sizeof(state_t));
		} else if(dev_type==EIGHTKEY) {
			my_xpad[dev].spalte  = &default_eight_dev.spalte;
			my_xpad[dev].zeile   = &default_eight_dev.zeile;
			memset(&my_xpad[dev].state, 0 ,sizeof(state_t));
			memcpy(&my_xpad[dev].state , &default_eight_dev.state, sizeof(state_t));
		} else if (dev_type == TERMINAL) {
			printf("Setup terminal %d"NL, dev_type);
		}
	}
	if (my_xpad[dev].spalte->cnt > 0) {
		GpioPortInit(my_xpad[dev].spalte);
	}else{
		my_xpad[dev].spalte->cnt=1;
	}

	if (my_xpad[dev].zeile->cnt > 0) {
		GpioPortInit(my_xpad[dev].zeile);
	}else{
		my_xpad[dev].zeile->cnt=1;
	}

	memset(my_xpad[dev].val2idx, 0xff, MAX_BUTTON_CNT);
	for (uint8_t val=0;val<MAX_BUTTON_CNT;val++){
		for (uint8_t i=0;i<MAX_BUTTON_CNT;i++){
			uint8_t value=lable2uint8(my_xpad[dev].state.label[i]);
			if (val==value){
				my_xpad[dev].val2idx[val]= i;
				break;
			}
		}
	}
	for (uint8_t val=my_xpad[dev].state.first;val<my_xpad[dev].state.cnt+my_xpad[dev].state.first;val++){
		printf("val= 0x%01x ->  0x%01x"NL, val, my_xpad[dev].val2idx[val] );
	}
}

static void xpad_set_spalten_pin(dev_handle_t dev, uint8_t spalten_nr) {
	if (mpy_xpad[dev] == NULL) {
		printf("No valid handle on xpad_set_spalte"NL);
		return;
	}
	gpio_pin_t *pin = &my_xpad[dev].spalte->pin[spalten_nr];
	GpioPinWrite(pin, GPIO_PIN_SET);

	return;
}

static void xpad_reset_spalten_pin(dev_handle_t dev, uint8_t spalten_nr) {
	if (mpy_xpad[dev] == NULL) {
		printf("No valid handle on xpad_set_spalte"NL);
		return;
	}
	gpio_pin_t *pin = &my_xpad[dev].spalte->pin[spalten_nr];
	GpioPinWrite(pin, GPIO_PIN_RESET);
	return;
}

static uint8_t xpad_update_key(uint8_t dev, uint8_t index, bool pinVal) {
	my_xpad[dev].key[index].current = pinVal;
//	if (pinVal) {
//		printf("Pushed @ (index= %d) to %d"NL, index, pinVal);
//	}
	//uint8_t z,s = INDEX_2_ZEI_SPA(index);
	uint8_t z = index_2_zei(&my_xpad[dev], index);
	uint8_t s = index_2_spa(&my_xpad[dev], index);
	uint8_t res=0;
//	if (my_xpad[dev].key[index].current ^ my_xpad[dev].key[index].last) {
//		printf("Detected Key @ (index =%d) with label %c"NL, index, my_xpad[dev].state.label[index]);
//	}
	my_xpad[dev].key[index].unstable = my_xpad[dev].key[index].unstable
			|| (my_xpad[dev].key[index].current ^ my_xpad[dev].key[index].last);

	if (my_xpad[dev].key[index].unstable) {
		if (my_xpad[dev].key[index].current == my_xpad[dev].key[index].last) {
			my_xpad[dev].key[index].cnt++;
			//printf("Increased index %d to %d (pinVal=%d)"NL, index, my_xpad[dev].key[index].cnt, pinVal);
		} else {
			my_xpad[dev].key[index].cnt=0;
		}
	}
	if (my_xpad[dev].key[index].cnt > STABLE_CNT) {
		// We got a stable state
		my_xpad[dev].key[index].unstable = false;
		char label = my_xpad[dev].state.label[index];
		my_xpad[dev].key[index].last = pinVal;
		my_xpad[dev].key[index].stable = pinVal;
		//printf("Reached index %d to %d (pinVal=%d)"NL, index,STABLE_CNT, pinVal);
		if (pinVal) {
			my_xpad[dev].state.state[index] = ((my_xpad[dev].state.state[index] + 1)
					% KEY_STAT_CNT);
			my_xpad[dev].dirty = true;
			printf("Pushed   Key @ (index =%d, z=%d, s=%d, value = %c)"NL, index, z, s, label);
		} else {
			printf("Released Key @ (index =%d, z=%d, s=%d, value = %c)"NL, index, z , s, label);
		}
		res = res | (pinVal << z);
		my_xpad[dev].key[index].cnt = 0;
	}

//	if (my_xpad[dev].key[index].current ^ my_xpad[dev].key[index].last) {
//		my_xpad[dev].key[index].cnt = 0;
//	}
	my_xpad[dev].key[index].last = my_xpad[dev].key[index].current;
	return res;
}

static uint16_t xpad_eight_scan(dev_handle_t dev) {
	if (mpy_xpad[dev]==NULL) {
		printf("No valid handle on read_row"NL);
		return false;
	}
	if (my_xpad[dev].dev_type!=EIGHTKEY) {
		printf("Handle for this keyboard not valid"NL);
		return false;
	}
	uint16_t res=0;
	for (uint8_t zeile=0;zeile<my_xpad[dev].zeile->cnt; zeile++) {
		bool pin=0;
		GpioPinRead(&my_xpad[dev].zeile->pin[zeile], &pin);
		pin =!pin;
		res = res|(xpad_update_key(dev, zeile, pin));
	}
	return res;
}
static uint16_t xpad_read_zeile(dev_handle_t dev, uint8_t spalten_nr) {
	if (mpy_xpad[dev] == NULL) {
		printf("No valid handle on read_zeile"NL);
		return 0;
	}
	uint8_t res = 0;
	for (int8_t z =0 ; z<my_xpad[dev].zeile->cnt; z++) {
		bool pinVal = 0;
		GpioPinRead(&my_xpad[dev].zeile->pin[z], &pinVal);
		pinVal = !pinVal;
		uint8_t index = zei_spa_2_index(&my_xpad[dev], z, spalten_nr);
		res = res | xpad_update_key(dev, index, pinVal);
	}
	return res;
}

static uint16_t xpad_spalten_scan(dev_handle_t dev) {
	if (mpy_xpad[dev] == NULL) {
		printf("No valid handle on scan"NL);
		return false;
	}
	if (my_xpad[dev].dev_type != XSCAN) {
		printf("Handle (%d) for this keyboard not valid"NL, my_xpad[dev].dev_type);
		return false;
	}
	uint16_t res = 0;
	for (uint8_t s = 0; s < my_xpad[dev].spalte->cnt; s++) {
		uint8_t ir = 0;
		xpad_reset_spalten_pin(dev, s);
		HAL_Delay(SETTLE_TIME_MS);
		ir = xpad_read_zeile(dev, s);
		res = res | (ir << (4 * s));
		xpad_set_spalten_pin(dev, s);
	}
	if (res != 0) {
		printf("Key Label is  0x%04lX"NL, (uint32_t) res);
	}
	return res;
}


static void xpad_state(dev_handle_t dev, state_t *oState) {
	if (mpy_xpad[dev] == NULL) {
		printf("No valid handle on state"NL);
		return;
	}
	uint8_t oIdx = oState->first;
	uint8_t iIdx = my_xpad[dev].state.first;
	state_print(&my_xpad[dev].state, NULL);
	for (; iIdx<oState->first+oState->cnt; iIdx++, oIdx++) {
		oState->state[oIdx]   = my_xpad[dev].state.state[iIdx];
		oState->label[oIdx]   = my_xpad[dev].state.label[iIdx];
	}
	oState->dirty = my_xpad[dev].dirty;
	return;
}

static void xpad_reset(dev_handle_t dev, bool hard) {
	if (mpy_xpad[dev] == NULL) {
		printf("No valid handle on reset"NL);
		return;
	}
	my_xpad[dev].dirty = false;
	for (uint8_t i = 0; i < MAX_BUTTON_CNT; i++) {
		if (hard) {
			my_xpad[dev].state.state[i] = OFF;
			my_xpad[dev].key[i] = reset_key;
		}
	}
	return;
}

kybd_t xscan_dev = {
	.init = &xpad_init,
	.scan = &xpad_spalten_scan,
	.reset= &xpad_reset,
	.state = &xpad_state,
	.dev_type = XSCAN,
	.cnt = 16,
	.first = 0,
};

kybd_t eight_dev = {
	.init = &xpad_init,
	.scan = &xpad_eight_scan,
	.reset =&xpad_reset,
	.state = &xpad_state,
	.dev_type = EIGHTKEY,
	.cnt = 8,
	.first = 0,
};

