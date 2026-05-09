/*
 * port.c
 *
 *  Created on: 28.12.2019
 *      Author: badi
 */
#include "_gpio.h"
#include "common.h"
#include "emLibC_options.h"
#define GPIO_NUMBER 16
em_msg GpioCheckPort(GPIO_TypeDef * port){
    em_msg res = EM_OK;
    if (port==GPIOA) return res;
    if (port==GPIOB) return res;
    if (port==GPIOC) return res;
    if (port==GPIOD) return res;
    if (port==GPIOH) return res;
    return EM_ERR;
}


em_msg GpioPinInit(gpio_pin_t *pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t res = EM_ERR;
    if (pin->port == NULL) {
        printf("Got NULL pointer as port " NL);
        return res;
    }
    if (pin != NULL) {
        if (pin->port == GPIOA) {
            __HAL_RCC_GPIOA_CLK_ENABLE();
        } else if (pin->port == GPIOB) {
            __HAL_RCC_GPIOB_CLK_ENABLE();
        } else if (pin->port == GPIOC) {
            __HAL_RCC_GPIOC_CLK_ENABLE();
        } else if (pin->port == GPIOD) {
            __HAL_RCC_GPIOD_CLK_ENABLE();
        } else if (pin->port == GPIOH) {
            __HAL_RCC_GPIOH_CLK_ENABLE();
        } else {
            printf("Unknown port 0x%8p" NL, pin->port);
            return res;
        }

        GPIO_InitStruct.Pin = pin->pin;
        GPIO_InitStruct.Mode = pin->conf.Mode;
        GPIO_InitStruct.Pull = pin->conf.Pull;
        GPIO_InitStruct.Speed = pin->conf.Speed;
        HAL_GPIO_Init(pin->port, &GPIO_InitStruct);
        if (pin->conf.Mode == GPIO_MODE_OUTPUT_PP) {
            GPIO_PinState state = pin->inv ^ pin->def;
            pin->state = state;
            HAL_GPIO_WritePin(pin->port, pin->pin, state);
        }
        res = EM_OK;
    }
    return res;
}

em_msg GpioPinWrite(gpio_pin_t *pin, bool value) {
    uint8_t res = EM_ERR;
    if (!pin) return res;
    if (GpioCheckPort(pin->port) == EM_OK) {
        /* Following check consumes 20ms on 384000 (5*240x320) calls BSSR*/
        res = EM_OK;
        GPIO_PinState state = value ? GPIO_PIN_SET : GPIO_PIN_RESET;
        pin->state = pin->inv ^ state;
        HAL_GPIO_WritePin(pin->port, pin->pin, pin->state);
    }
    return res;
}
em_msg GpioPinRead(gpio_pin_t *pin, bool *value) {
    uint8_t res = EM_ERR;
    if (!pin) return res;
    GPIO_PinState state;
    if (GpioCheckPort(pin->port) == EM_OK) {
#ifdef USE_HAL_LL
        if ((pin->port->IDR & pin->pin) != 0x00u)    	  {
          pin->state = GPIO_PIN_SET;
        }else{
          pin->state = GPIO_PIN_RESET;
        }
#else
        __disable_irq();
         state = HAL_GPIO_ReadPin(pin->port, pin->pin);
        __enable_irq();
#endif
        state = pin->inv ^ state;
        *value = pin->state == GPIO_PIN_SET ? true : false;
        res = EM_OK;
    }
    return res;
}

em_msg GpioPinToggle(gpio_pin_t *pin) {
    uint8_t res = EM_ERR;
    if (!pin) return res;
    if (GpioCheckPort(pin->port) == EM_OK) {
    pin->state = pin->state ^= 1u ;
#if USE_HAL_LL
	if (pin->state) {
	  pin->port->BSRR = pin->pin;                   // SET — atomar
	} else {
	  pin->port->BSRR = (uint32_t)pin->pin << 16u;  // RESET — atomar
	}
#else
	__disable_irq();
	HAL_GPIO_WritePin(pin->port, pin->pin, pin->state);
	__enable_irq();
#endif
        //
        res  = EM_OK;
    }
    return res;
}
