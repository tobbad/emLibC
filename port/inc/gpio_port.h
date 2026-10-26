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
    uint16_t mask;
    uint16_t mask_size; // derived from mask (contiguous bits from bit 0) by GpioPortInit()/GpioPort_setMask(), never set by hand
    gpio_pin_t pin[10];
    uint8_t cnt;
} gpio_port_t;

em_msg GpioPortInit(gpio_port_t *port);
em_msg GpioPort_setMask(gpio_port_t *port, uint16_t mask);
em_msg GpioPortTogglePolarity(gpio_port_t *port);
em_msg GpioPortToggle(gpio_port_t *port);
em_msg GpioPortSet(gpio_port_t *port, uint8_t val);
em_msg GpioPortGet(gpio_port_t *port, uint16_t *val);

#endif /* GPIO_PORT_H_ */
