/*
 * sk9822.c
 *
 *  Created on: Aug 9, 2020
 *      Author: badi
 */

#include "assert.h"
#include "common.h"
#include "device.h"
#include "mkeyboard.h"
#include "sk9822.h"

#define SK9822_RED_SHIFT 0
#define SK9822_GREEN_SHIFT 8
#define SK9822_BLUE_SHIFT 16
#define SK9822_BRIGTHNESS_SHIFT 24

#define SK9822_RGB_MASK 0x00FFFFFF
#define SK9822_BRIGHTNESS_MASK_U8 0x1F
#define SK9822_BRIGHTNESS_MASK (SK9822_BRIGHTNESS_MASK_U8<<SK9822_BRIGTHNESS_SHIFT)
#define SK9822_DEFAULT_BITS 0xE0000000


typedef struct  sk9822_s{
    buffer_t * buffer;
    uint8_t led_cnt;
}sk9822_t;

sk9822_t my_sk9822[DEVICE_CNT];


em_msg sk9822_init(dev_handle_t hdl, buffer_t *buffer, uint8_t led_cnt) {
    my_sk9822[hdl].buffer = buffer;
    my_sk9822[hdl].led_cnt = led_cnt;
    sk9822_reset(hdl);
    return hdl;
}


void sk9822_reset(dev_handle_t hdl) {
    sk9822_clear(hdl);
    sk9822_set_brighness(hdl, SK9822_ALL_LEDS, 0x00);
}


void sk9822_set_rgb(dev_handle_t hdl, led_nr_t led_nr, uint8_t rgb[3]){
    uint8_t low_lim = 0;
    uint8_t high_lim = my_sk9822[hdl].led_cnt;
    uint32_t value;
    uint32_t *ptr = (uint32_t*)my_sk9822[hdl].buffer;

    if (led_nr < my_sk9822[hdl].led_cnt) {
        low_lim = led_nr;
        high_lim = led_nr+1;
    }

    for (uint8_t led_nr=low_lim;led_nr<high_lim;led_nr++) {

        value = ptr[1+led_nr] & (~SK9822_RGB_MASK);
        value |= SK9822_DEFAULT_BITS;
        value |= rgb[SK9822_RED] << SK9822_RED_SHIFT;
        value |= rgb[SK9822_GREEN] << SK9822_GREEN_SHIFT;
        value |= rgb[SK9822_BLUE] << SK9822_BLUE_SHIFT;
        ptr[1+led_nr] = value;
    }
}

void sk9822_set_brighness(dev_handle_t hdl, led_nr_t led_nr, uint8_t brightness){

    uint8_t low_lim = 0;
    uint8_t high_lim = my_sk9822[hdl].led_cnt;
    uint32_t value;
    uint32_t *ptr = (uint32_t*)my_sk9822[hdl].buffer->mem;

    brightness &= SK9822_BRIGHTNESS_MASK_U8;
    if (led_nr < my_sk9822[hdl].led_cnt) {
        low_lim = led_nr;
        high_lim = led_nr+1;
    }

    for (uint8_t led_nr=low_lim;led_nr<high_lim;led_nr++) {
        value = ptr[1+led_nr] & (~SK9822_BRIGHTNESS_MASK);
        value |= SK9822_DEFAULT_BITS;
        value |= brightness << (SK9822_BRIGTHNESS_SHIFT);
        ptr[1+led_nr] = value;
    }
}


void sk9822_clear(dev_handle_t hdl){
    memset(my_sk9822[hdl].buffer, 0, my_sk9822[hdl].buffer->size);
    uint32_t *ptr = (uint32_t*) my_sk9822[hdl].buffer->mem;
    ptr[my_sk9822[hdl].led_cnt+1] = 0xFFFFFFFF;
}

void sk9822_commit(dev_handle_t hdl) {
    //sk9822_write(4*SK9822_32BIT_BUFFERSIZE(sk9822_led_count));
}
