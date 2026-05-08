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

#define OFFSET 8
typedef enum { FEHLER = OFFSET, NORMAL } stateled_e;
void stateled_init(state_t *state, gpio_port_t *port, uint16_t cycle_size, uint8_t bli_cnt);
void stateled_toggle_port();
void stateled_toggle_pin(stateled_e pinNr);
void stateled_iterate();
void stateled_show(system_state_e state);
void stateled_toggle_pin(stateled_e pinNr);
void stateled_on(uint8_t led_nr);
void stateled_off(uint8_t led_nr);
void stateled_all_off();
bool stateled_update(system_state_e state);

#endif /* INC_D_LEDLINE_H_ */
