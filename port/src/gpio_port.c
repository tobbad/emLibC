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
    EM_RETURN_IF_NULL(port, EM_ERR);
    for (port->mask_size = 0; (mask & (1 << port->mask_size)) != 0; port->mask_size++) ;
    if (port->mask_size == 0) {
        return EM_ERR;
    } else {
        return EM_OK;
    }
    return EM_ERR;
}

em_msg GpioPortInit(gpio_port_t *port) {
    EM_RETURN_IF_NULL(port, EM_ERR);
    for (uint8_t i = 0; i < port->cnt; i++) {
        if (GpioPinInit(&port->pin[i]) == EM_ERR) {
            printf("Pin %d is not good" NL, i);
        }
    }
    port->mask = 0xFFFF;
    return EM_OK;
}
em_msg GpioPort_setMask(gpio_port_t *port, uint16_t mask) {
    EM_RETURN_IF_NULL(port, EM_ERR);
    port->mask = mask;
    return EM_OK;
}

em_msg GpioPortToggle(gpio_port_t *port) {
    EM_RETURN_IF_NULL(port, EM_ERR);
    em_msg res = EM_OK;
    for (uint8_t i = 0; i < port->cnt; i++) {
        res |= GpioPinToggle(&port->pin[i]);
    }
    return res;
}

em_msg GpioPortSet(gpio_port_t *port, uint8_t val) {
    EM_RETURN_IF_NULL(port, EM_ERR);
    em_msg res = GpioPortCheck_mask(port, port->mask);
    if (res == EM_OK) {
        /* mask_size kann bis 16 laufen, pin[] fasst aber nur port->cnt. */
        uint8_t n = MIN(port->mask_size, port->cnt);
        for (uint8_t i = 0; i < n; i++) {
            if ((val & (1 << i)) && (port->mask & (1 << i))) {
                res |= GpioPinWrite(&port->pin[i], 1);
            } else {
                res |= GpioPinWrite(&port->pin[i], 0);
            }
        }
    }
    return res;
}

em_msg GpioPortGet(gpio_port_t *port, uint16_t *val) {
    EM_RETURN_IF_NULL(port, EM_ERR);
    EM_RETURN_IF_NULL(val, EM_ERR);
    em_msg res = GpioPortCheck_mask(port, port->mask);
    if (res == EM_OK) {
        bool pinValue = 0;
        for (uint8_t i = 0; i < port->cnt; i++) {
            res |= GpioPinRead(&port->pin[i], &pinValue);
            if (i <= port->mask_size) {
                *val |= ((pinValue << i) & (port->mask & (i << i)));
            } else {
                *val |= (pinValue << i);
            }
        }
    }
    return res;
}
