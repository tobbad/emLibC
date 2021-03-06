/*
 * sk9822.c
 *
 *  Created on: Aug 9, 2020
 *      Author: badi
 */

#include "assert.h"
#include "common.h"
#include "device.h"
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

static void sk9822_write(uint32_t count);


sk9822_handle_t sk9822_init(device_t *device, buffer_t *buffer) {
    sk9822_handle_t hdl = 0;
    sk9822_dev = *device;
    sk9822_buffer = *buffer;
    sk9822_led_count = (sk9822_buffer.size>>2)-2;
    sk9822_reset(hdl);
    return hdl;
}


void sk9822_reset(sk9822_handle_t hdl) {
    sk9822_clear(hdl);
    sk9822_set_brighness(hdl, SK9822_ALL_LEDS, 0x00);
    return 0;
}


void sk9822_set_rgb(sk9822_handle_t hdl, led_nr_t led_nr, uint8_t rgb[3]){
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

void sk9822_set_brighness(sk9822_handle_t hdl, led_nr_t led_nr, uint8_t brightness){

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


void sk9822_clear(sk9822_handle_t hdl){
    memset(sk9822_buffer.mem, 0, sk9822_buffer.size);
    uint32_t *ptr = (uint32_t*)sk9822_buffer.mem;
    ptr[sk9822_led_count+1] = 0xFFFFFFFF;
}

void sk9822_commit(sk9822_handle_t hdl) {
    sk9822_write(4*SK9822_32BIT_BUFFERSIZE(sk9822_led_count));
}


static void sk9822_write(uint32_t count) {
    device_write(&sk9822_dev, sk9822_buffer.mem, count);
}
