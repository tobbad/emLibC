/*
 * gpio_port.c
 *
 *  Created on: Dec 24, 2024
 *      Author: badi
 */
#include "gpio_port.h"
#include "_gpio.h"
#include "common.h"
em_msg GpioPortInit(gpio_port_t *port) {
    for (uint8_t i = 0; i < port->cnt; i++) {
        if (GpioPinInit(&port->pin[i]) == EM_ERR) {
            printf("Pin %d is not good" NL, i);
        }
    }
    return EM_OK;
}

void GpioPortToggle(gpio_port_t *port) {
    for (uint8_t i = 0; i < port->cnt; i++) {
        GpioPinToggle(&port->pin[i]);
    }
    return;
}
