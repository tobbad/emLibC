/*
 * sk9822.h
 *
 *  Created on: Aug 9, 2020
 *      Author: badi
 */

#ifndef EMLIBC_DEVICE_LED_INC_SK9822_H_
#define EMLIBC_DEVICE_LED_INC_SK9822_H_

#include "common.h"
#include "sk9822.h"

typedef uint8_t sk9822_handle_t;
typedef uint8_t led_nr_t;

typedef enum {
    SK9822_RED = 0,
    SK9822_GREEN = 1,
    SK9822_BLUE = 2,
    SK9822_COL_CNT
} SK9822_COLOR;

#define SK9822_ALL_LEDS 255


#define SK9822_32BIT_BUFFERSIZE(LED_COUNT) (1+LED_COUNT+1)


sk9822_handle_t sk9822_init(device_t *device,  buffer_t * buffer);
void sk9822_reset(sk9822_handle_t hdl);
void sk9822_set_rgb(sk9822_handle_t hdl, led_nr_t led_nr, uint8_t rgb[SK9822_COL_CNT]);
void sk9822_set_brighness(sk9822_handle_t hdl, led_nr_t led_nr, uint8_t brighness);
void sk9822_commit(sk9822_handle_t hdl);


#endif /* EMLIBC_DEVICE_LED_INC_SK9822_H_ */
