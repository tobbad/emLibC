/*
 * gpio.h
 *
 *  Created on: 28.12.2019
 *      Author: badi
 */

#ifndef LIB_GPIO_H_
#define LIB_GPIO_H_
#include "common.h"
#include "hal_port.h"


typedef struct Gpio_t_
{
	GPIO_TypeDef* port;
	uint16_t pin;
} GpioPin_t;

em_msg GpioPinRead(GpioPin_t *pin, bool *value);
em_msg GpioPinWrite(GpioPin_t *pin, bool value);
em_msg GpioPinToggle(GpioPin_t *pin);

#endif /* LIB_GPIO_H_ */
