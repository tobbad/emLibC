/*
 * gpio_port.c
 *
 *  Created on: Dec 24, 2024
 *      Author: badi
 */
#include "gpio_port.h"
#include "_gpio.h"
#include "common.h"

em_msg GpioPortCheck_mask(gpio_port_t *port, uint16_t mask) {
    for (port->mask_size=0; (mask & (1<<port->mask_size)) != 0; port->mask_size++);
    if (port->mask_size==0){
        return EM_ERR;
    } else {
        return EM_OK;
    }
    return EM_ERR;
}

em_msg GpioPortInit(gpio_port_t *port) {
    for (uint8_t i = 0; i < port->cnt; i++) {
        if (GpioPinInit(&port->pin[i]) == EM_ERR) {
            printf("Pin %d is not good" NL, i);
        }
    }
    port->mask =0xFFFF;
    return EM_OK;
}
em_msg GpioPort_setMask(gpio_port_t *port, uint16_t mask) {
    port->mask =mask;
    return EM_OK;
}

em_msg GpioPortToggle(gpio_port_t *port) {
    em_msg res;
    for (uint8_t i = 0; i < port->cnt; i++) {
        res |= GpioPinToggle(&port->pin[i]);
    }
    return res;
}

em_msg GpioPortSet(gpio_port_t *port, uint8_t val){
    em_msg res= GpioPortCheck_mask(port, port->mask);
    if (res==EM_OK) {
        for (uint8_t i = 0; i < port->mask_size; i++) {
            if ((val & (1<<i))&&(port->mask & (1<<i))){
               res |= GpioPinWrite(&port->pin[i], 1);
            } else {
                res |= GpioPinWrite(&port->pin[i], 0);

            }
        }
    }
    return res;
}

em_msg GpioPortGet(gpio_port_t *port, uint16_t *val){
    em_msg res= GpioPortCheck_mask(port, port->mask);
    if (res==EM_OK) {
        bool pinValue;
        for (uint8_t i = 0; i < port->cnt; i++) {
            res |= GpioPinRead(&port->pin[i], &pinValue);
            if (i <= port->mask_size){
                *val |= (pinValue<<i)&&(port->mask & (i<<i));
            } else {
                *val |= (pinValue<<i);
            }
        }
    }
    return res;
}
