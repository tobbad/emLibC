/*
 * hal_port.h
 *
 *  Created on: Dec 24, 2024
 *      Author: badi
 */

#ifndef GPIO_PORT_H_
#define GPIO_PORT_H_
#include "gpio.h"

typedef struct gpio_port_s{
	gpio_pin_t  pin[8];
	uint8_t     cnt;
} gpio_port_t;

em_msg GpioPortInit(gpio_port_t *port);

//em_msg GpioSetPortMode(gpio_port_t *port, gpio_mode_t mode);
//em_msg GpioSetPortMode(GpioPort_t *port, gpio_mode_t mode);


#endif /* GPIO_PORT_H_ */