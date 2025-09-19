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
        memset(mDev[hdl].buffer->mem, 0, mDev[hdl].buffer->size);
        return EM_OK;
    }
    return EM_ERR;
}

em_msg memory_device_write(dev_handle_t hdl, const uint8_t *data, int16_t count) {
    if ((NULL != mDev[hdl].buffer) && (mDev[hdl].buffer->used >= 1) && (NULL != data)) {
        uint16_t cnt = MIN(mDev[hdl].buffer->used, count);
        for (uint16_t i = 0; i < cnt; i++) {
            printf("set buf[%u] = %u\n", i, data[i]);
            mDev[hdl].buffer->mem[mDev[hdl].buffer->used++] = data[i];
        }
        return EM_OK;
    }
    return EM_ERR;
}

em_msg memory_device_read(dev_handle_t hdl, uint8_t *data, int16_t *count) {
    if ((NULL != mDev[hdl].buffer) && (mDev[hdl].buffer->used >= 1) && (NULL != data)) {
        uint16_t cnt = MIN(*count, mDev[hdl].buffer->used);
        for (uint16_t i = 0; i < cnt; i++) {
            printf("buf[%u] = %u\n", i, data[i]);
            data[i] = mDev[hdl].buffer->mem[i];
        }
        *count = cnt;
        return EM_OK;
    }
    return EM_ERR;
}

em_msg memory_device_print(dev_handle_t hdl, buffer_t *buf) {
    static const uint32_t BUF_SIZE = 310;
    char buffer[BUF_SIZE];
    if ((NULL != buf) && (NULL != buf->mem)) {
        if ((buf->used > 0) && (buf->used < BUF_SIZE)) {
            uint8_t offset = buf->pl - buf->mem;
            uint16_t full_cnt = offset + buf->used;
            uint16_t used_cnt = to_hex(buffer, BUF_SIZE, buf->mem, full_cnt, true);
            if (offset > 0) {
                printf("Buffer starts @ idx %d\n", offset);
            }
            if (used_cnt < BUF_SIZE) {
                printf("%s\n", buffer);
            } else {
                printf("%s", buffer);
                printf(".... .. (dropped used %d) \n", used_cnt);
            }
            return EM_OK;
        } else {
            printf("Transfer Size= %d(of = %d\n", buf->used, buf->size);
        }
    } else {
        if (NULL == buf) {
            printf("Given buffer is NULL\n");
        } else {
            printf("Given buffer memory is NULL\n");
        }
    }
    return EM_ERR;
}

device_t memory_device = {
    .user_data = NULL,
    .open = memory_device_open,
    .read = memory_device_read,
    .write = memory_device_write,
    .ioctrl = NULL,
    .close = NULL,
};
