/*
 * hal_port.h
 *
 *  Created on: Dec 24, 2024
 *      Author: badi
 */

#ifndef GPIO_PORT_H_
#define GPIO_PORT_H_
#include "_gpio.h"

typedef struct gpio_port_s {
  gpio_pin_t pin[10];
  uint8_t cnt;
} gpio_port_t;

em_msg GpioPortInit(gpio_port_t *port);
void GpioPortToggle(gpio_port_t *port);

#endif /* GPIO_PORT_H_ */
