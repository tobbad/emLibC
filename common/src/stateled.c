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

static led_line_t my_led_line;
static  gpio_port_t def_port ={
	.cnt =8,
	.pin = {
		{ .port = GPIOC, .pin = GPIO_PIN_8,  .cState= false, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOC, .pin = GPIO_PIN_6,  .cState= false, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_15, .cState= false, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_14, .cState= false, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_13, .cState= false, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_12, .cState= false, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_2,  .cState= false, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOC, .pin = GPIO_PIN_14, .cState= false, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
	},
};

void stateled_init(state_t *state, gpio_port_t *port, uint8_t cycle_size){
    static state_t mstate;
	my_led_line.cycle_size = cycle_size;
	if (state!=NULL){
        my_led_line.state = state;
        my_led_line.lstate = *state;
	} else {
	    my_led_line.state = mState;
        my_led_line.lstate = &mstate;


	}
    if (port!=NULL){
    	my_led_line.port = port;
    }else{
    	my_led_line.port =  &def_port;
    }
    GpioPortInit(my_led_line.port);
    my_led_line.init=true;
}
void stateled_toggle(){

};

void stateled_update(){
    if (my_led_line.init) {
        static uint32_t d;
    	static uint32_t ltick=0;
    	uint32_t ctick = HAL_GetTick();
    	d = ctick-ltick;
        static uint8_t cnt=0;
        cnt++;
        cnt = ((cnt)%my_led_line.cycle_size);
        bool bstate = (cnt < my_led_line.cycle_size >> 1);
        if (my_led_line.init) {
            if (!state_is_same(my_led_line.state, &my_led_line.lstate)){
                my_led_line.lstate = *my_led_line.state;
                //printf("Ledline Update"NL);
            }
            for (uint8_t i=0;i<my_led_line.port->cnt;i++){
            	int8_t stateNr = i+my_led_line.lstate.first;
            	key_state_e tState=my_led_line.lstate.state[stateNr];
                if (tState==OFF){
                    GpioPinWrite(&my_led_line.port->pin[i], false);
                } else if (tState==BLINKING){
                    GpioPinWrite(&my_led_line.port->pin[i], bstate);
                } else if (tState==ON){
                    GpioPinWrite(&my_led_line.port->pin[i], true);
                }
            }
        }
     	ltick=ctick;
    }
}



