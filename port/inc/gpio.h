/*
 * gpio.h
 *
 *  Created on: 28.12.2019
 *      Author: badi
 */

#ifndef LIB_GPIO_H_
#define LIB_GPIO_H_

#include <stdint.h>
#include <stdbool.h>
#include "verr.h"
#include "port.h"
#include "hal_port.h"

#define PORTW_MAX 16


typedef enum {
	PIN_NA=-1,
	PIN_0,
	PIN_1,
	PIN_2,
	PIN_3,
	PIN_4,
	PIN_5,
	PIN_6,
	PIN_7,
	PIN_8,
	PIN_9,
	PIN_10,
	PIN_11,
	PIN_12,
	PIN_13,
	PIN_14,
	PIN_15,
	PIN_MAX
} pin_t;

typedef enum {
	PORT_NA=0,
	PORTA,
	PORTB,
	PORTC,
	PORTD,
	PORTE,
	PORTF,
	PORTG,
	PORTH,
	PORTI,
	PORT_CNT,
} port_t;


typedef enum  {
	INPUT     = LL_GPIO_MODE_INPUT     ,
	OUTPUT    = LL_GPIO_MODE_OUTPUT    ,
	ANALOG    = LL_GPIO_MODE_ANALOG    ,
	ALT_FUNC  = LL_GPIO_MODE_ALTERNATE ,
	PIN_MODE_CNT
} gpio_mode_t;

typedef enum  {
	PIN_PP = 0, /* Push pull type */
	PIN_OD = 1, /* Open drain type */
	PIN_CNT
} gpio_pin_t;

typedef enum  {
	s_LOW     = 0,
	s_MEDIUM,
	s_HIGH,
	s_VERY_HIGH,
	s_PIN_SPEED_CNT
} gpio_speed_t;

typedef enum  {
	PULL_NONE = 0,
	PULL_UP = 1,
	PULL_DOWN = 2,
	PIN_PUPD_CNT
} gpio_pupd_t;

typedef enum  {
	PIN_AF0 ,
	PIN_AF1 ,
	PIN_AF2 ,
	PIN_AF3 ,
	PIN_AF4 ,
	PIN_AF5 ,
	PIN_AF6 ,
	PIN_AF7 ,
	PIN_AF8 ,
	PIN_AF9 ,
	PIN_AF10,
	PIN_AF11,
	PIN_AF12,
	PIN_AF13,
	PIN_AF14,
	PIN_AF15,
	PIN_AF_CNT,
} gpio_af_t;

typedef struct GpioMode_t_
{
	gpio_mode_t mode;
	gpio_pin_t pin;
	gpio_speed_t speed;
	gpio_pupd_t pupd;
	gpio_af_t af;
} GpioConf_t;


typedef struct Gpio_t_
{
	port_t port;
	pin_t pin;
} Gpio_t;

typedef struct GpioPin_t_
{
	GpioConf_t conf;
	port_t port;
	pin_t pin;
	hal_pin_t ll;
} GpioPin_t;

typedef struct GpioPort_t_
{
	GpioConf_t conf;
	Gpio_t gpio[PORTW_MAX];
} GpioPort_t;
extern uint32_t LL_GPIO_Pin_Map[];
em_msg GpioPinInit(GpioPin_t *pin);
em_msg GpioPortInit(GpioPort_t *port);

//em_msg GpioSetPortMode(GpioPort_t *port, gpio_mode_t mode);

em_msg GpioPinRead(GpioPin_t *pin, bool *value);
em_msg GpioPinWrite(GpioPin_t *pin, bool value);
em_msg GpioPinToggle(GpioPin_t *pin);

/*
 * It is the responsability of the caller, that the port is output
 * direction.
 */
//em_msg GpioPortRead(GpioPort_t *port, uint32_t *value);
//em_msg GpioPortWrite(GpioPort_t *port, uint32_t value);


#endif /* LIB_GPIO_H_ */
