/*
 * d_ledline.h
 *
 *  Created on: Feb 19, 2025
 *      Author: TBA
 */

#ifndef INC_D_LEDLINE_H_
#define INC_D_LEDLINE_H_
#include "state.h"
#include "gpio_port.h"

void d_ledline_init(state_t *state, gpio_port_t *port, uint8_t cycle_size);
void d_ledline_update();


#endif /* INC_D_LEDLINE_H_ */
