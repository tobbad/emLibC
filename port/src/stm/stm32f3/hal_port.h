/*
 * hal_port.h
 *
 *  Created on: 12.01.2020
 *      Author: badi
 */

#ifndef LIB_PORT_STM32F3_HAL_PORT_H_
#define LIB_PORT_STM32F3_HAL_PORT_H_

#include "stm32f3xx.h"


typedef struct hal_pin_t_
{
	uint16_t pin_mask;
	GPIO_TypeDef *port;
} hal_pin_t;

#define HAL_PIN_STATE(pin, state)	(pin)->ll.port->BSRR = (pin)->ll.pin_mask<<(state?0:16);
#define HAL_PIN_SET(pin)	HAL_PIN_STATE(pin, true)
#define HAL_PIN_RESET(pin)	HAL_PIN_STATE(pin, false)

#endif /* LIB_PORT_STM32F3_HAL_PORT_H_ */
