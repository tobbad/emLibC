/*
 * sk9822.c
 *
 *  Created on: Aug 9, 2020
 *      Author: badi
 */

#include "assert.h"
#include "device.h"
#include "sk9822.h"

#define SK9822_RGB_MASK 0x00FFFFFF
#define SK9822_BRIGHTNESS_MASK 0x1F000000
#define SK9822_DEFAULT_BITS 0xD0000000

#define SK9822_RED_SHIFT 0
#define SK9822_GREEN_SHIFT 8
#define SK9822_BLUE_SHIFT 16
#define SK9822_BIGTHNESS_SHIFT 24

static buffer_t sk9822_buffer;
static device_t sk9822_dev;
static uint8_t sk9822_led_count;



sk9822_handle_t sk9822_init(device_t *device, buffer_t *buffer) {
    sk9822_dev = *device;
    sk9822_buffer = *buffer;
    sk9822_led_count = (sk9822_buffer.size>>2)-2;
    *(uint32_t*)&(sk9822_buffer.mem[0]) = 0;
    *(uint32_t*)&(sk9822_buffer.mem[sk9822_buffer.size-4]) = 0;
    return 0;
}



void sk9822_set_rgb(handle_t hdl, led_nr_t led_nr, uint8_t rgb[3]){
    uint32_t value;
    uint32_t *ptr = (uint32_t*)sk9822_buffer.mem;
    assert(led_nr<sk9822_led_count);
    value = ptr[1+led_nr] & (~SK9822_RGB_MASK);
    value |= SK9822_DEFAULT_BITS;
    value |= rgb[SK9822_RED] << SK9822_RED_SHIFT;
    value |= rgb[SK9822_GREEN] << SK9822_GREEN_SHIFT;
    value |= rgb[SK9822_BLUE] << SK9822_BLUE_SHIFT;

    ptr[1+led_nr] = value;
}

void sk9822_set_brighness(handle_t hdl, led_nr_t led_nr, uint8_t brighness){
    uint32_t value;
    uint32_t *ptr = (uint32_t*)sk9822_buffer.mem;
    assert(led_nr<sk9822_led_count);

    value = ptr[1+led_nr] & (~SK9822_BRIGHTNESS_MASK);
    value |= SK9822_DEFAULT_BITS;
    value |= brighness << SK9822_BIGTHNESS_SHIFT;

    ptr[1+led_nr] = value;
}

void sk9822_commit(handle_t hdl) {
    sk9822_write(4*SK9822_32BIT_BUFFERSIZE(sk9822_led_count));
}


static void sk9822_write(uint32_t count) {
    device_write(&sk9822_dev, sk9822_buffer.mem, count);
}
