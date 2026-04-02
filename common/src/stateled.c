/*
 * stateled.c
 *
 *  Created on: Feb 19, 2025
 *      Author: TBA
 */
#include "rb_system.h"
#include "stateled.h"
#include "_gpio.h"
#include "common.h"
#include "gpio_port.h"
#include "state.h"
#define INIT_LED_TIME 100
typedef struct led_line_s {
    bool init;
    state_t lstate;
    state_t *state;
    gpio_port_t *port;
    uint8_t cycle_size;
    uint8_t bli_cnt;
} led_line_t;

static led_line_t my_stateled;
// clang-format off
static  gpio_port_t def_port ={
	.cnt =10,
	.pin = {
		{ .port = GPIOC, .pin = GPIO_PIN_8,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOC, .pin = GPIO_PIN_6,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_15, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_14, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_13, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
		{ .port = GPIOB, .pin = GPIO_PIN_12, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
        { .port = GPIOB, .pin = GPIO_PIN_2,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
        { .port = GPIOC, .pin = GPIO_PIN_14, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
        { .port = GPIOB, .pin = GPIO_PIN_5,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
        { .port = GPIOB, .pin = GPIO_PIN_6,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } },
	},
};
// clang-format on

void stateled_init(state_t *state, gpio_port_t *port, uint16_t cycle_size, uint8_t bli_cnt) {
    my_stateled.cycle_size = cycle_size;
    if (state != NULL) {
        my_stateled.state = state;
        my_stateled.lstate = *state;
    }
    if (port != NULL) {
        my_stateled.port = port;
    } else {
        my_stateled.port = &def_port;
    }
    my_stateled.bli_cnt = bli_cnt*my_stateled.port->cnt;
    GpioPortInit(my_stateled.port);
    my_stateled.init = true;
    for (uint8_t i = 0; i < my_stateled.port->cnt; i++) {
        stateled_on(i);
        HAL_Delay(INIT_LED_TIME);
        stateled_off(i);
    }
}

void stateled_toggle_port() {
    // clang-format off
    if (!my_stateled.init) return;
    // clang-format on
    GpioPortToggle(my_stateled.port);
};

void stateled_iterate() {
    // clang-format off
    if (!my_stateled.init) return;
    // clang-format on
    static uint8_t idx = 0;
    uint8_t last = idx;
    idx = (idx+1)% my_stateled.state->cnt;
    stateled_off(last);
    stateled_toggle_pin(idx);
};

void stateled_on(uint8_t led_nr) {
    // clang-format off
    if (!my_stateled.init) return;
    // clang-format on
    if (led_nr>SLOT_CNT){
        led_nr -= OFFSET;
    }
    GpioPinWrite(&my_stateled.port->pin[led_nr], true);
};
void stateled_off(uint8_t led_nr) {
    // clang-format off
    if (!my_stateled.init) return;
    // clang-format on
    GpioPinWrite(&my_stateled.port->pin[led_nr], false);
};

void stateled_toggle_pin(uint8_t led_nr) {
    // clang-format off
    if (!my_stateled.init) return;
    // clang-format on
    GpioPinToggle(&my_stateled.port->pin[led_nr]);
};

void stateled_all_off() {
    // clang-format off
    if (!my_stateled.init) return;
    // clang-format on
    for (uint8_t i = 0; i < my_stateled.port->cnt; i++) {
       stateled_off(i);
    }
};
void stateled_show(uint8_t cnt) {
    // clang-format off
    if (!my_stateled.init) return;
    // clang-format on
    bool bstate = (cnt < (my_stateled.cycle_size >> 1));
    if (!state_is_same(my_stateled.state, &my_stateled.lstate)) {
        my_stateled.lstate = *my_stateled.state;
        // printf("Ledline Update"NL);
    } else {
        for (uint8_t i = 0; i < my_stateled.port->cnt; i++) {
            int8_t stateNr = i + my_stateled.lstate.first;
            key_state_e tState = my_stateled.lstate.state[stateNr];
            if (tState == ON){
                stateled_on(i);
            }else if (tState == OFF) {
                stateled_off(i);
            } else if (tState == BLINKING) {
                GpioPinWrite(&my_stateled.port->pin[i], bstate);
            }
        }
    }
};

bool stateled_update(system_state_e state, bool doDot) {
    // clang-format off
    if (!my_stateled.init) return false;
    // clang-format on
    static uint8_t cnt = 0;
    static uint8_t bli_cnt=0;
    cnt++;
    cnt = ((cnt) % my_stateled.cycle_size);
    // clang-format off
    if (state == SYNC_RESET) {
        return false;
    }  else if ((state == SYNCHRONIZE) || (state == SYNCHRONIZED_PARTLY)){
         if (cnt == 0){
            stateled_iterate();
            if (doDot){
                printf(".");
                fflush(stdout);
            }
            bli_cnt++;
        }
        if  (bli_cnt==my_stateled.bli_cnt){
            stateled_off(0);
            bli_cnt  = 0;
            return true;
        }
   } else if (state == SYNC_ERROR){
        if (cnt == 0){
            stateled_toggle_port();
        }
    } else if (state == SYNC_READY) {
        stateled_show(cnt);
    }
    return false;
}
