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
em_msg GpioPortSet(gpio_port_t *port, uint8_t val){
    em_msg res=EM_OK;
    for (uint8_t i = 0; i < sizeof(val); i++) {
        if (val & (1<<i)){
           res |= GpioPinWrite(&port->pin[i], 1);
        } else {
            res |= GpioPinWrite(&port->pin[i], 0);

        }
    }
    return res;
}

em_msg GpioPortGet(gpio_port_t *port, uint16_t *val){
    em_msg res=EM_OK;
    bool pinValue;
    for (uint8_t i = 0; i < port->cnt; i++) {
        res |= GpioPinRead(&port->pin[i], &pinValue);
        *val |= pinValue<<i;
    }
    return res;
}
