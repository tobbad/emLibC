/*
 * memory_device.c
 *
 *  Created on: 30.10.2020
 *      Author: badi
 */
#include <string.h>
#include "common.h"
#include "memory_device.h"

uint8_t rcv_buffer[RCV_BUF_SIZE];

buffer_t memory_device_buffer = {.size=RCV_BUF_SIZE, .used=0, .mem=rcv_buffer, .pl=rcv_buffer};

elres_t memory_device_reset(buffer_t *buf)
{
    printf("buffer_t Reset\n");
    buf->used = 0;
    memset(buf->mem, 0, buf->size);
    return EMLIB_OK;
}

elres_t memory_device_write(void *user_data, const uint8_t *data, uint16_t count)
{
    buffer_t *buf = (buffer_t *)user_data;
    if ((NULL != buf) && (buf->size-buf->used >= 1) && (NULL != data))
    {
        for (uint16_t i = 0;i<count; i++)
        {
            printf("buf[%d] = %d\n", buf->used, data[i]);
            buf->pl[buf->used++] = data[i];
        }
        return EMLIB_OK;
    }
    return EMLIB_ERROR;
}

elres_t memory_device_print(buffer_t *buf)
{
    static const uint32_t BUF_SIZE =310;
    char buffer[BUF_SIZE];
    if ( (NULL != buf) && (NULL != buf->mem)) {
        if ( (buf->used > 0) && (buf->used<BUF_SIZE)) {
            uint8_t offset = buf->pl-buf->mem;
            uint16_t full_cnt = offset+buf->used;
            uint16_t used_cnt = to_hex(buffer, BUF_SIZE, buf->mem, full_cnt, true);
            if (offset>0) {
                printf("Buffer starts @ idx %d\n", offset);
            }
            if (used_cnt<BUF_SIZE) {
                printf("%s\n", buffer);
            } else {
                printf("%s", buffer);
                printf(".... .. (dropped used %d) \n", used_cnt);
            }
            return EMLIB_OK;
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
    return EMLIB_ERROR;
}

device_t memory_device = {
    .user_data=&memory_device_buffer,
    .open=NULL,
    .read=NULL,
    .write=memory_device_write,
    .ioctrl=NULL,
    .close=NULL,
};
