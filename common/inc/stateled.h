/*
 * d_ledline.h
 *
 *  Created on: Feb 19, 2025
 *      Author: TBA
 */

#ifndef INC_D_LEDLINE_H_
#define INC_D_LEDLINE_H_
#include "gpio_port.h"
#include "state.h"

typedef enum  {
    led_0,
    led_1,
    led_2,
    led_3,
    led_4,
    led_5,
    led_6,
    led_7,
    led_fehler,
    led_normal,
}stateled_e;
#define OFFSET 8
void stateled_init(state_t *state, gpio_port_t *port, uint16_t cycle_size, uint8_t bli_cnt);
void stateled_deinit();
em_msg stateled_set_mask(uint16_t mask);
em_msg stateled_set(uint16_t val);
em_msg stateled_toggle_port();
em_msg stateled_toggle_pin(stateled_e pinNr);
void stateled_iterate();
void stateled_show(system_state_e state);
em_msg stateled_on(uint8_t led_nr);
em_msg stateled_off(uint8_t led_nr);
em_msg stateled_all_off();
bool stateled_update(system_state_e state);

#endif /* INC_D_LEDLINE_H_ */
