/*
 * stateled.c
 *
 *  Created on: Feb 19, 2025
 *      Author: TBA
 */
#include "stateled.h"
#include "_gpio.h"
#include "common.h"
#include "emLibC_options.h"
#include "gpio_port.h"
#include "state.h"
#define INIT_LED_TIME 100
typedef struct led_line_s {
    bool state_init;
    state_t lstate;
    state_t *state;
    uint8_t cycle_size;
    bool init[2];
    gpio_port_t *port[2];
    uint16_t mask[2];
    uint8_t bli_cnt[2];
    uint8_t cnt[2];
} led_line_t;

static led_line_t my_stateled;
// clang-format off
static  gpio_port_t def_port ={
    .mask = 0x000F,
    .mask_size =4,
	.cnt =10,
	.pin = {
        { .port = GPIOC, .pin = GPIO_PIN_8,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 0 data 0                         led 0
        { .port = GPIOC, .pin = GPIO_PIN_6,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 1 data 1                         led 1
        { .port = GPIOB, .pin = GPIO_PIN_15, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 2 data 2                         led 2
        { .port = GPIOB, .pin = GPIO_PIN_14, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 3 data 3                         led 3
        { .port = GPIOB, .pin = GPIO_PIN_13, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 4 subslot toggel                 led 4
        { .port = GPIOB, .pin = GPIO_PIN_12, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 5 slot toggle                    led 5
        { .port = GPIOB, .pin = GPIO_PIN_2,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 6 cycle toggle                   led 6
        { .port = GPIOC, .pin = GPIO_PIN_14, .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 7 1 when tx active               led 7
        { .port = GPIOB, .pin = GPIO_PIN_5,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 8 1 when rx active               led fehler
        { .port = GPIOB, .pin = GPIO_PIN_6,  .def= false,  .inv= true, .conf = { .Mode = GPIO_MODE_OUTPUT_PP, .Speed=GPIO_SPEED_FREQ_LOW, .Pull = GPIO_NOPULL } }, // 9 0->1->0 when cycle is updated  led normal
	},
};


static bool stateled_port_valid(dev_handle_t dev) {
    return (dev>STATELED_DEV_NA) && (dev<STATELED_CNT)&&(my_stateled.port[dev-1]&&my_stateled.port[dev-1]->cnt>0);
}

/* led_nr indiziert das feste Feld gpio_port_t.pin[] -- gegen cnt absichern. */
static bool stateled_pin_valid(dev_handle_t dev, uint8_t led_nr) {
    bool res = stateled_port_valid(dev);
    return res & (my_stateled.port[dev-1] != NULL) && (led_nr < my_stateled.port[dev-1]->cnt);
}

// clang-format on

dev_handle_t stateled_init(state_t *state, gpio_port_t *port, uint16_t cycle_size, uint8_t bli_cnt) {
    dev_handle_t dev = STATELED_DEV_NA;
    my_stateled.cycle_size = cycle_size;
    if (state != NULL) {
        my_stateled.state = state;
        my_stateled.lstate = *state;
    }
    if (port != NULL) {
        dev = STATELED_EXTERN ;
        my_stateled.port[dev - 1] = port;
    } else {
        dev = STATELED_INBUILD;
        my_stateled.port[dev - 1] = &def_port;
    }
    my_stateled.init[dev-1] = true;
    my_stateled.bli_cnt[dev-1] = bli_cnt * my_stateled.port[dev-1]->cnt;
    my_stateled.cnt[dev-1] = 0;
    // GpioPortInit() opens the mask to all pins, so narrow it afterwards:
    // stateled_set() may only drive the data pins 0..3.
    GpioPortInit(my_stateled.port[dev-1]);
    stateled_set_mask(dev ,0xF);
    for (uint8_t i = 0; i < my_stateled.port[dev-1]->cnt; i++) {
#if 0
        stateled_on(dev, i);
        HAL_Delay(INIT_LED_TIME);
        stateled_off(dev, i);
#else
        stateled_set(dev, i);
        HAL_Delay(INIT_LED_TIME);
#endif
    }
    return dev;
}

void stateled_deinit(dev_handle_t dev) {
    if (!stateled_port_valid(dev))
        return;
    my_stateled.port[dev-1] = NULL;
};


void stateled_toggle_polarity(dev_handle_t dev){
    // clang-format off
    if (!my_stateled.init[dev-1]) return;
    if (!stateled_port_valid(dev)) return ;
    // clang-format on
    GpioPortTogglePolarity(my_stateled.port[dev-1]);
};

em_msg stateled_set_mask(dev_handle_t dev, uint16_t mask) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return EM_ERR;
    if (!stateled_port_valid(dev)) return EM_ERR;
    // clang-format on
    return GpioPort_setMask(my_stateled.port[dev-1], mask);
}

em_msg stateled_set(dev_handle_t dev, uint16_t val) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return EM_ERR;
    // clang-format on
    return GpioPortSet(my_stateled.port[dev-1], val);
}

em_msg stateled_toggle_port(dev_handle_t dev) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return EM_ERR;
    if (!stateled_port_valid(dev)) return EM_ERR;
    // clang-format on
    return GpioPortToggle(my_stateled.port[dev-1]);
};

void stateled_iterate(dev_handle_t dev) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return;
    if (!stateled_port_valid(dev)) return ;
    if ((my_stateled.port[dev-1] == NULL) || (my_stateled.port[dev-1]->cnt == 0)) return;
    // clang-format on
    static uint8_t idx = 0;
    uint8_t last = idx;
    /* modulo port->cnt (nicht MAX_BUTTON_CNT) -- sonst idx > pin[]-Grenze. */
    idx = (idx + 1) % my_stateled.port[dev-1]->cnt;
    stateled_off(dev, last);
    stateled_toggle_pin(dev, idx);
};

em_msg stateled_on(dev_handle_t dev, uint8_t led_nr) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return EM_ERR;
    if (!stateled_pin_valid(dev, led_nr)) return EM_ERR;
    // clang-format on
    return GpioPinWrite(&my_stateled.port[dev-1]->pin[led_nr], true);
};

em_msg stateled_off(dev_handle_t dev, uint8_t led_nr) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return EM_ERR;
    if (!stateled_pin_valid(dev, led_nr)) return EM_ERR;
    // clang-format on
    return GpioPinWrite(&my_stateled.port[dev-1]->pin[led_nr], false);
};

em_msg stateled_toggle_pin(dev_handle_t dev, stateled_e led_nr) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return EM_ERR;
    if (!stateled_pin_valid(dev, (uint8_t)led_nr)) return EM_ERR;
    // clang-format on
    return GpioPinToggle(&my_stateled.port[dev-1]->pin[led_nr]);
};

em_msg stateled_dtoggle_pin(dev_handle_t dev, stateled_e led_nr) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return EM_ERR;
    if (!stateled_pin_valid(dev, (uint8_t)led_nr)) return EM_ERR;
    // clang-format on
    GpioPinToggle(&my_stateled.port[dev-1]->pin[led_nr]);
    return GpioPinToggle(&my_stateled.port[dev-1]->pin[led_nr]);
};

em_msg stateled_all_off(dev_handle_t dev) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return EM_ERR;
    if (!stateled_port_valid(dev)) return EM_ERR;
    // clang-format on
    for (uint8_t i = 0; i < my_stateled.port[dev-1]->cnt; i++) {
        stateled_off(dev, i);
    }
    return EM_OK;
};

void stateled_show(dev_handle_t dev, system_state_e state) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return;
    if (my_stateled.state==NULL) return;
    if (!stateled_port_valid(dev)) return;
    // clang-format on
    my_stateled.cnt[dev-1]++;
    my_stateled.cnt[dev-1] = ((my_stateled.cnt[dev-1]) % my_stateled.cycle_size);
    bool bstate = (my_stateled.cnt[dev-1] < (my_stateled.cycle_size >> 1));
    if (state == SYNCHRONIZE_ERROR) {
        if (my_stateled.cnt[dev-1] == 0) {
            stateled_toggle_port(dev);
        }
    } else if (state >= SYNCHRONIZE) {
        if (state_is_same(my_stateled.state, &my_stateled.lstate) == EM_ERR) {
            my_stateled.lstate = *my_stateled.state;
            // printf("Ledline Update"NL);
        } else {
            for (uint8_t i = 0; i < my_stateled.port[dev-1]->cnt; i++) {
                int8_t stateNr = i + my_stateled.lstate.first;
                if ((stateNr < 0) || (stateNr >= MAX_STATE_CNT)) {
                    continue; /* sonst OOB auf lstate.state[stateNr] */
                }
                key_state_e tState = my_stateled.lstate.state[stateNr];
                if (tState == ON) {
                    stateled_on(dev, i);
                } else if (tState == OFF) {
                    stateled_off(dev, i);
                } else if (tState == BLINKING) {
                    GpioPinWrite(&my_stateled.port[dev-1]->pin[i], bstate);
                }
            }
        }
    }
};

bool stateled_update(dev_handle_t dev, system_state_e state) {
    // clang-format off
    if (!my_stateled.init[dev-1]) return false;
    if (!stateled_port_valid(dev)) return false;
    // clang-format on
    my_stateled.bli_cnt[dev-1] = 0;
    my_stateled.cnt[dev-1]++;
    my_stateled.cnt[dev-1] = ((my_stateled.cnt[dev-1]) % my_stateled.cycle_size);
    // clang-format off
    if (state == SYNC_RESET) {
        return false;
    }  else if ((state >= SYNCHRONIZE)){
        if (my_stateled.cnt[dev-1] == 0){
            stateled_iterate(dev);
            my_stateled.bli_cnt[dev-1]++;
        }
        if  (my_stateled.bli_cnt[dev-1] == my_stateled.cycle_size){
            stateled_off(dev, 0);
            my_stateled.bli_cnt[dev-1]  = 0;
            return true;
        }
   } else if (state == SYNCHRONIZE_ERROR){
        if (my_stateled.cnt[dev-1] == 0){
            stateled_toggle_port(dev);
        }
    }
    return false;
}
