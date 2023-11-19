/*
 * port.c
 *
 *  Created on: 28.12.2019
 *      Author: badi
 */
//#include "stm32f4xx_hal.h"
#include <stdio.h>
#include "stm32l4xx_ll_gpio.h"
#include "gpio.h"


static GPIO_TypeDef* PORT_MAP[PORT_CNT] =
{
	NULL,
	GPIOA,
	GPIOB,
	GPIOC,
#ifdef GPIOD
	GPIOD,
#else
	NULL,
#endif
#if defined(GPIOE)
    GPIOE,
#else
    NULL,
#endif
#if defined(GPIOF)
    GPIOF,
#else
    NULL,
#endif
#if defined(GPIOG)
    GPIOG,
#else
    NULL,
#endif
#if defined(GPIOH)
    GPIOH,
#else
    NULL,
#endif
#if defined(GPIOI)
    GPIOI,
#else
    NULL,
#endif
};

static uint32_t PIN_MAP[PIN_MAX] =
{
	LL_GPIO_PIN_0 ,
	LL_GPIO_PIN_1 ,
	LL_GPIO_PIN_2 ,
	LL_GPIO_PIN_3 ,
	LL_GPIO_PIN_4 ,
	LL_GPIO_PIN_5 ,
	LL_GPIO_PIN_6 ,
	LL_GPIO_PIN_7 ,
	LL_GPIO_PIN_8 ,
	LL_GPIO_PIN_9 ,
	LL_GPIO_PIN_10,
	LL_GPIO_PIN_11,
	LL_GPIO_PIN_12,
	LL_GPIO_PIN_13,
	LL_GPIO_PIN_14,
    LL_GPIO_PIN_15,
};

static uint32_t MODE_MAP[PIN_MODE_CNT] =
{
	LL_GPIO_MODE_INPUT     ,  /* Select input mode */
	LL_GPIO_MODE_OUTPUT    ,  /* Select output mode */
	LL_GPIO_MODE_ALTERNATE ,  /* Select alternate function mode */
    LL_GPIO_MODE_ANALOG    ,  /* Select analog mode */
};

static uint32_t PIN_TYPE[PIN_CNT] =
{
	LL_GPIO_OUTPUT_PUSHPULL , /* Select push-pull as output type */
	LL_GPIO_OUTPUT_OPENDRAIN, /* Select open-drain as output type */
};

static uint32_t SPEED_MAP[PIN_SPEED_CNT] =
{
    LL_GPIO_SPEED_FREQ_LOW      , /* Select I/O low output speed    */
	LL_GPIO_SPEED_FREQ_MEDIUM   , /* Select I/O medium output speed */
	LL_GPIO_SPEED_FREQ_HIGH     , /* Select I/O fast output speed   */
	LL_GPIO_SPEED_FREQ_VERY_HIGH, /* Select I/O high output speed   */
};

static uint32_t PUPD_MAP[PIN_PUPD_CNT] =
{
    LL_GPIO_PULL_NO  , /* Select I/O no pull */
    LL_GPIO_PULL_UP  , /* Select I/O pull up */
    LL_GPIO_PULL_DOWN, /* Select I/O pull down */
};

static uint32_t AF_MAP[PIN_AF_CNT] =
{
    LL_GPIO_AF_0 , /* Select alternate function 0 */
    LL_GPIO_AF_1 , /* Select alternate function 1 */
    LL_GPIO_AF_2 , /* Select alternate function 2 */
    LL_GPIO_AF_3 , /* Select alternate function 3 */
    LL_GPIO_AF_4 , /* Select alternate function 4 */
    LL_GPIO_AF_5 , /* Select alternate function 5 */
    LL_GPIO_AF_6 , /* Select alternate function 6 */
    LL_GPIO_AF_7 , /* Select alternate function 7 */
    LL_GPIO_AF_8 , /* Select alternate function 8 */
    LL_GPIO_AF_9 , /* Select alternate function 9 */
    LL_GPIO_AF_10, /* Select alternate function 10 */
    LL_GPIO_AF_11, /* Select alternate function 11 */
    LL_GPIO_AF_12, /* Select alternate function 12 */
    LL_GPIO_AF_13, /* Select alternate function 13 */
    LL_GPIO_AF_14, /* Select alternate function 14 */
    LL_GPIO_AF_15, /* Select alternate function 15 */
};

struct drv_data_t_
{
	GPIO_TypeDef *port;
	uint32_t 	 pinSet;
	uint32_t 	 pinRest;
} drv_data_t;

/*
 * Inline will reduce call by 40 ms on 240*320 pixel
 */
inline static em_msg CheckConf(GpioConf_t *conf)
{
	em_msg res = EM_ERR;
	if ((conf->mode < PIN_MODE_CNT) &&
		(conf->pin_t < PIN_CNT) &&
		(conf->speed < PIN_SPEED_CNT) &&
		(conf->pupd<PIN_PUPD_CNT) &&
		(conf->af < PIN_AF_CNT))
	{
		res = EM_OK;
	}
	return res;
}

/*
 * Inline will reduce call by 40 ms on 240*320 pixel
 */
inline static em_msg CheckGpio(GpioPin_t *pin)
{
	em_msg res = CheckConf(&pin->conf);
	if ((EM_OK == res) &&
		(pin->port > PORT_NA) && (pin->port < PORT_CNT) &&
		(pin->pin > PIN_NA) && (pin->pin < PIN_MAX))
	{
		res = EM_OK;
	}
	return res;
}

static void MapConf(GpioConf_t *conf, GPIO_InitTypeDef  *ll_gpio)
{
	ll_gpio->Mode = MODE_MAP[conf->mode];
	ll_gpio->Speed = SPEED_MAP[conf->speed];
	ll_gpio->Pin = PIN_TYPE[conf->pin_t];
	ll_gpio->Pull = PUPD_MAP[conf->pupd];
	ll_gpio->Alternate = AF_MAP[conf->af];
}

em_msg GpioPinInit(GpioPin_t *pin)
{
	em_msg res = CheckGpio(pin);
	if (EM_OK == res)
	{
		GPIO_InitTypeDef  ll_gpio = {0};
		GPIO_TypeDef *port = PORT_MAP[pin->port];
		MapConf(&pin->conf, &ll_gpio);
        ll_gpio.Pin = PIN_MAP[pin->pin];
		LL_GPIO_Init(port, &ll_gpio);
		pin->ll.port = port;
		pin->ll.pin_mask = ll_gpio.Pin;
	}
	else
	{
		pin->ll.port = NULL;
		pin->ll.pin_mask = 0;
	}
	return res;
}

em_msg GpioPortInit(GpioPort_t *port)
{
	em_msg res = CheckConf(&port->conf);
	if (EM_OK == res)
	{
		GpioPin_t pin;
		pin.conf = port->conf;
		for (uint8_t bitNr=0;bitNr<PORTW_MAX;bitNr++)
		{
			pin.pin = port->gpio[bitNr].pin;
			pin.port = port->gpio[bitNr].port;
			res = GpioPinInit(&pin);
			if (EM_ERR == res)
			{
				break;
			}
		}
	}
	return res;
}

em_msg GpioSetPortMode(GpioPort_t *port, gpio_mode_t mode)
{
	em_msg res = CheckConf(&port->conf);
	if (EM_OK == res)
	{
		res = EM_ERR;
		if (mode < PIN_MODE_CNT)
		{
			if (port->conf.mode != mode)
			{
				port->conf.mode = mode;
				GpioPortInit(port);
			}
			res = EM_OK;
		}
	}
	return res;
}

/*
 * Calling inlined CheckGpio consumes 30 ms on 320x240 Pixels
 * Avoid repeated lookup of GPIO_TypeDef and hal_pin reduces
 * 30ms on 320x240 Pixels
 * OUTPUT:
 * Do nothing: 136ms
 * Hard set output always: 163 ms
 * Set output when needed: 158
 * Set output when needed low level: 149
 */
em_msg GpioPinWrite(GpioPin_t *pin, bool value)
{
	em_msg res = EM_ERR;//CheckGpio(pin);
	if (pin->ll.port != NULL)
	{
		/* Following check consumes 20ms on 384000 (5*240x320) calls */
#if 0
		if (OUTPUT != pin->conf.mode)
		{
#if 0
			pin->conf.mode = OUTPUT;
			GpioPinInit(pin);
#else
			pin->conf.mode = OUTPUT;
			pin->ll.port->OTYPER |= pin->ll.pin_mask;
#endif
		}
#endif
		GPIO_PinState state = value?GPIO_PIN_SET:GPIO_PIN_RESET;
		HAL_GPIO_WritePin(pin->ll.port, pin->ll.pin_mask, state);
		res = EM_OK;
	}
	return res;
}

em_msg GpioPinRead(GpioPin_t *pin, bool *value)
{
	em_msg res = CheckGpio(pin);
	if (EM_OK == res)
	{
		GPIO_TypeDef *hal_gpio = PORT_MAP[pin->port];
		uint16_t hal_pin = PIN_MAP[pin->pin];
		GPIO_PinState state = HAL_GPIO_ReadPin(hal_gpio, hal_pin);
		*value = state==GPIO_PIN_SET?true:false;
		res = EM_OK;
	}
	return res;
}

em_msg GpioPortRead(GpioPort_t *port, uint32_t *value)
{
	em_msg res = CheckConf(&port->conf);
	if (EM_OK == res)
	{
		GpioPin_t pin;
		pin.conf = port->conf;
		*value = 0;
		for (uint8_t bitNr=0;bitNr<PORTW_MAX;bitNr++)
		{
			bool bitVal = false;
			pin.pin = port->gpio[bitNr].pin;
			pin.port = port->gpio[bitNr].port;
			res = GpioPinRead(&pin, &bitVal);
			if (EM_OK == res)
			{
				*value |= (bitVal?1:0)<<bitNr;
			}
		}
	}
	return res;
}

#if 0
em_msg GpioPortWrite(GpioPort_t *port, uint32_t value)
{
	em_msg res = CheckConf(&port->conf);
	if (EM_OK == res)
	{
		GpioPin_t pin;
		pin.conf = port->conf;
		for (uint8_t bitNr=0;bitNr<PORTW_MAX;bitNr++)
		{
			bool bitVal = (value&(1<<bitNr));
			pin.pin = port->gpio[bitNr].pin;
			pin.port = port->gpio[bitNr].port;
			res = GpioPinWrite(&pin, bitVal);
		}
	}
	return res;
}
#else
em_msg GpioPortWrite(GpioPort_t *port, uint32_t value)
{
	em_msg res = CheckConf(&port->conf);
	if (EM_OK == res)
	{
		GPIO_TypeDef *hal_gpio = PORT_MAP[PORTD];
		uint32_t p_mask = 0x0000000F;
		uint32_t p_val = value & p_mask;
		uint32_t p_set = p_val & p_mask;
		uint32_t p_reset = (~p_val) & p_mask;
		uint32_t p_reg = (p_reset << 16) | (p_set & 0x0000FFFF);
		hal_gpio->BSRR = p_reg;

		hal_gpio = PORT_MAP[PORTE];
		p_mask = 0x0000FFF0;
		p_val = value & p_mask;
		p_set = p_val & p_mask;
		p_reset = (~p_val) & p_mask;
		p_reg = (p_reset << 16) | (p_set & 0x0000FFFF);
		hal_gpio->BSRR = p_reg;
	}
	return res;
}
#endif


em_msg GpioPinToggle(GpioPin_t *pin) {
    em_msg res = CheckGpio(pin);
    if (EM_OK == res)
    {
        bool value;
        res = GpioPinRead(pin, &value);
        if (EM_OK == res) {
            value = !value;
            res = GpioPinWrite(pin, value);
        }
    }
    return res;

}


