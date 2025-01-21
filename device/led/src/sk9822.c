/*
 * sk9822.c
 *
 *  Created on: Aug 9, 2020
 *      Author: badi
 */

#include "assert.h"
#include "common.h"
#include "device.h"
#include "keyboard.h"
#include "sk9822.h"

#define SK9822_RED_SHIFT 0
#define SK9822_GREEN_SHIFT 8
#define SK9822_BLUE_SHIFT 16
#define SK9822_BRIGTHNESS_SHIFT 24

#define SK9822_RGB_MASK 0x00FFFFFF
#define SK9822_BRIGHTNESS_MASK_U8 0x1F
#define SK9822_BRIGHTNESS_MASK (SK9822_BRIGHTNESS_MASK_U8<<SK9822_BRIGTHNESS_SHIFT)
#define SK9822_DEFAULT_BITS 0xE0000000

static buffer_t sk9822_buffer;
static device_t sk9822_dev;
static uint8_t sk9822_led_count;

typedef sk9822_s{
    uint8_t * buffer;
    device_t dev;
    uint8_t cnt;
}sk9822_t;

sk9822_t my_sk9822[DEVICE_CNT];



em_msg sk9822_init(dev_handle_t hdl, buffer_t *buffer) {
    my_sk9822[hdl].buffer = buffer->mem;
    my_sk9822[hdl].cnt = (sk9822_buffer.size>>2)-2;
    sk9822_reset(hdl);
    return hdl;
}


void sk9822_reset(dev_handle_t hdl) {
    sk9822_clear(hdl);
    sk9822_set_brighness(hdl, SK9822_ALL_LEDS, 0x00);
}


void sk9822_set_rgb(dev_handle_t hdl, led_nr_t led_nr, uint8_t rgb[3]){
    uint8_t low_lim = 0;
    uint8_t high_lim = sk9822_led_count;
    uint32_t value;
    uint32_t *ptr = (uint32_t*)sk9822_buffer.mem;

    if (led_nr < sk9822_led_count) {
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
    uint8_t high_lim = sk9822_led_count;
    uint32_t value;
    uint32_t *ptr = (uint32_t*)sk9822_buffer.mem;

    brightness &= SK9822_BRIGHTNESS_MASK_U8;
    if (led_nr < sk9822_led_count) {
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
    memset(sk9822_buffer.mem, 0, sk9822_buffer.size);
    uint32_t *ptr = (uint32_t*)sk9822_buffer.mem;
    ptr[sk9822_led_count+1] = 0xFFFFFFFF;
}

void sk9822_commit(dev_handle_t hdl) {
    //sk9822_write(4*SK9822_32BIT_BUFFERSIZE(sk9822_led_count));
}
