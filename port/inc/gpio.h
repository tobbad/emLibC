/*
 * gpio.h
 *
 *  Created on: 28.12.2019
 *      Author: badi
 */

#ifndef GPIO_H_
#define GPIO_H_
#include "common.h"
#include "hal_port.h"

typedef struct gpio_pin_s{
	GPIO_TypeDef* port;
	uint16_t pin;
	bool def;
	bool inv;
	GPIO_InitTypeDef conf;
} gpio_pin_t;

em_msg GpioPinInit(gpio_pin_t *pin);
em_msg GpioPinRead(gpio_pin_t *pin, bool *value);
em_msg GpioPinWrite(gpio_pin_t *pin, bool value);
em_msg GpioPinToggle(gpio_pin_t *pin);

#endif /* GPIO_H_ */
