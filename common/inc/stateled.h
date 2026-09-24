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

typedef enum  {// with REGUALR_STATELD  0
    led_0,     // data 0
    led_1,     // data 1
    led_2,     // data 2
    led_3,     // data 3
    led_4,     // slot toggle
    led_5,     // cycle tpggle
    led_6,     // 1 when tx active
    led_7,     // 1 when rx active
    led_fehler,
    led_normal,
}stateled_e;

typedef enum  {     // debug enums
    ddata_0,        // data 0
    ddata_1,        // data 1
    ddata_2,        // data 2
    ddata_3,        // data 3
    ss_toggle,      // subslot toggel
    slot_toggle,    // slot toggle
    cycle_toggle,   // cycle toggle
    tx_toggle,      // 1 when tx active
    rx_toggle,	    // 1 when rx active
    cycle_update,   // 0->1->0 when cycle is updated
}dstateled_e;
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
