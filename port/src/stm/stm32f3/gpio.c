/*
 * port.c
 *
 *  Created on: 28.12.2019
 *      Author: badi
 */
#include <stdio.h>
#include "common.h"
#include "gpio.h"

em_msg GpioPinInit(gpio_pin_t *pin){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	uint8_t res = EM_ERR;
	if (pin != NULL){
		if (pin->port ==GPIOA){
			__HAL_RCC_GPIOA_CLK_ENABLE();
		} else if (pin->port ==GPIOB){
			  __HAL_RCC_GPIOB_CLK_ENABLE();
		} else if (pin->port ==GPIOC){
			__HAL_RCC_GPIOC_CLK_ENABLE();
		} else {
			printf("unknwn port 0x%08x");
		}

		GPIO_InitStruct.Pin = pin->pin;
		GPIO_InitStruct.Mode = pin->conf.Mode;
		GPIO_InitStruct.Pull = pin->conf.Pull;
		GPIO_InitStruct.Speed = pin->conf.Speed;
		HAL_GPIO_Init(pin->port, &GPIO_InitStruct);
		if (pin->conf.Mode==GPIO_MODE_OUTPUT_PP){
			HAL_GPIO_WritePin(pin->port, pin->pin, pin->def);
		}
		res = EM_OK;
	}
	return res;
}

em_msg GpioPinWrite(gpio_pin_t *pin, bool value){
	uint8_t res = EM_ERR;
	if (pin->port != NULL){
		/* Following check consumes 20ms on 384000 (5*240x320) calls BSSR*/
		GPIO_PinState state = value?GPIO_PIN_SET:GPIO_PIN_RESET;
		HAL_GPIO_WritePin(pin->port, pin->pin, state);
		res = EM_OK;
	}
	return res;
}

em_msg GpioPinRead(gpio_pin_t *pin, bool *value){
	uint8_t res = EM_ERR;
	if (pin->port != NULL)
	{
		GPIO_PinState state = HAL_GPIO_ReadPin(pin->port, pin->pin);
		*value = state==GPIO_PIN_SET?true:false;
		res = EM_OK;
	}
	return res;
}

em_msg GpioPinToggle(gpio_pin_t *pin) {
	uint8_t res = EM_OK;
    if (pin->port != NULL) {
        bool value;
        res = GpioPinRead(pin, &value);
        if (EM_OK == res) {
            value = !value;
            res = GpioPinWrite(pin, value);
        }
    }
    return res;

}




