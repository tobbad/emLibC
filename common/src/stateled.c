/*
 * d_ledline.c
 *
 *  Created on: Feb 19, 2025
 *      Author: TBA
 */
#include "common.h"
#include "state.h"
#include "stateled.h"
#include "gpio.h"
#include "gpio_port.h"
#define CYCLE_SIZE 10
typedef struct led_line_s{
    bool init;
    state_t lstate;
    state_t *state;
    gpio_port_t *port;
    uint8_t cycle_size;
} led_line_t;

static led_line_t my_stateled;
static  gpio_port_t def_port ={
	.cnt =10,
	.pin = {
		{ .port = GPIOC, .pin = GPIO_PIN_8,  .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOC, .pin = GPIO_PIN_6,  .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_15, .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_14, .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_13, .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_12, .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
        { .port = GPIOB, .pin = GPIO_PIN_2,  .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
        { .port = GPIOC, .pin = GPIO_PIN_14, .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
        { .port = GPIOB, .pin = GPIO_PIN_5,  .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
        { .port = GPIOB, .pin = GPIO_PIN_6,  .cState= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
	},
};

void stateled_init(state_t *state, gpio_port_t *port, uint8_t cycle_size){
	my_stateled.cycle_size = cycle_size;
	if (state!=NULL){
	    my_stateled.state = state;
	    my_stateled.lstate = *state;
	}
    if (port!=NULL){
        my_stateled.port = port;
    }else{
        my_stateled.port =  &def_port;
    }
    GpioPortInit(my_stateled.port);
    my_stateled.init=true;
}

void stateled_toggle(){
    if (!my_stateled.init) return;
	GpioPortToggle(my_stateled.port);
};

void stateled_update(){
    if (my_stateled.init) {
        static uint32_t d;
    	static uint32_t ltick=0;
    	uint32_t ctick = HAL_GetTick();
    	d = ctick-ltick;
        static uint8_t cnt=0;
        cnt++;
        cnt = ((cnt)%my_stateled.cycle_size);
        bool bstate = (cnt < my_stateled.cycle_size >> 1);
        if (my_stateled.init) {
            if (!state_is_same(my_stateled.state, &my_stateled.lstate)){
                my_stateled.lstate = *my_stateled.state;
                //printf("Ledline Update"NL);
            }
            for (uint8_t i=0;i<my_stateled.port->cnt;i++){
            	int8_t stateNr = i+my_stateled.lstate.first;
            	key_state_e tState=my_stateled.lstate.state[stateNr];
                if (tState==OFF){
                    GpioPinWrite(&my_stateled.port->pin[i], false);
                } else if (tState==BLINKING){
                    GpioPinWrite(&my_stateled.port->pin[i], bstate);
                } else if (tState==ON){
                    GpioPinWrite(&my_stateled.port->pin[i], true);
                }
            }
        }
     	ltick=ctick;
    }
}



