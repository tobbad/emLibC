/*
 * port.c
 *
 *  Created on: 28.12.2019
 *      Author: badi
 */
#include <stdio.h>
#include "common.h"
#include "gpio.h"

em_msg GpioPinWrite(GpioPin_t *pin, bool value){
	uint8_t res = EM_ERR;
	if (pin->port != NULL){
		/* Following check consumes 20ms on 384000 (5*240x320) calls BSSR*/
		GPIO_PinState state = value?GPIO_PIN_SET:GPIO_PIN_RESET;
		HAL_GPIO_WritePin(pin->port, pin->Pin, state);
		res = EM_OK;
	}
	return res;
}

em_msg GpioPinRead(GpioPin_t *pin, bool *value){
	uint8_t res = EM_ERR;
	if (pin->port != NULL)
	{
		GPIO_PinState state = HAL_GPIO_ReadPin(pin->port, pin->Pin);
		*value = state==GPIO_PIN_SET?true:false;
		res = EM_OK;
	}
	return res;
}

em_msg GpioPinToggle(GpioPin_t *pin) {
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




