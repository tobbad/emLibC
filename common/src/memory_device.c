/*
 * memory_device.c
 *
 *  Created on: 30.10.2020
 *      Author: badi
 */
#include "memory_device.h"
#include "buffer.h"
#include "common.h"
#include <string.h>

typedef struct memory_device_t {
    buffer_t *buffer;
} memory_device_t;

static memory_device_t mDev[DEVICE_CNT];

em_msg memory_device_open(dev_handle_t hdl, void *user_data) {
    if (mDev[hdl].buffer != NULL) {
        mDev[hdl].buffer = (buffer_t *)user_data;
        mDev[hdl].buffer = buffer_new_buffer_t(mDev[hdl].buffer);
        return EM_OK;
    }
    return EM_ERR;
}

em_msg memory_device_write(dev_handle_t hdl, const uint8_t *data, int16_t count) {
    // clang-format off
    em_msg res = buffer_check(mDev[hdl].buffer, false);
    if (res == EM_ERR) return res;
    int16_t used = buffer_used(mDev[hdl].buffer);
    uint16_t cnt = MIN(used, count);
    for (uint16_t i = 0; i < cnt; i++) {
        printf("set buf[%u] = %u\n", i, data[i]);
        mDev[hdl].buffer->mem[i] = data[i];
    }
    return EM_OK;
    return EM_ERR;
}

em_msg memory_device_read(dev_handle_t hdl, uint8_t *data, int16_t *count) {
    // clang-format off
    em_msg res = buffer_check(mDev[hdl].buffer, false);
    if (res == EM_ERR) return res;
    int16_t used = buffer_used(mDev[hdl].buffer);
    uint16_t cnt = MIN(*count, used);
    for (uint16_t i = 0; i < cnt; i++) {
        printf("buf[%u] = %u\n", i, data[i]);
        data[i] = mDev[hdl].buffer->mem[i];
    }
    *count = cnt;
    return EM_ERR;
}

void memory_device_print(dev_handle_t hdl, buffer_t *buf) {
    buffer_print(buf, NULL);
}

device_t memory_device = {
    .user_data = NULL,
    .open = memory_device_open,
    .read = memory_device_read,
    .write = memory_device_write,
    .ioctrl = NULL,
    .close = NULL,
};
