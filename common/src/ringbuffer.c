/*
 * ringbuffer.c
 *
 *  Created on: Apr 26, 2020
 *      Author: badi
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"
#include "device.h"

#include <string.h>

#include "ringbuffer.h"

const rbuf_t rbuf_clear = {
    .empty = true,
    .nxtRdIdx = 0,
    .nxtWrIdx = 0,
    .buffer = NULL,
    .buffer_size = 0,
};

rbufline_t line_buffer;

static rbuf_t *get_rbuf(rbuf_hdl_t hdl) {
    if ((hdl < 0) || (hdl >= RBUF_REGISTERS)) {
        return NULL;
    }
    return &line_buffer.rbuf_reg[hdl];
}

void rbuf_init(void) {
    for (uint8_t i = 0; i < RBUF_REGISTERS; i++) {
        line_buffer.rbuf_reg[i] = rbuf_clear;
    }
    line_buffer.valid_cnt = RBUF_REGISTERS;
    line_buffer.empty = false;
    line_buffer.nxtLineRdIdx = 0;
    line_buffer.nxtLineWrIdx = 0;
    line_buffer.current = 0;
}

rbuf_hdl_t rbuf_register(uint8_t *buffer, uint16_t size) {
    if (NULL == buffer) {
        return -1;
    }
    if (0 == size) {
        return -1;
    }
    for (uint8_t i = 0; i < RBUF_REGISTERS; i++) {
        if (NULL == line_buffer.rbuf_reg[i].buffer) {
            line_buffer.rbuf_reg[i].buffer = buffer;
            line_buffer.rbuf_reg[i].buffer_size = size;
            line_buffer.valid_cnt++;
            return (rbuf_hdl_t)i;
        }
    }
    return EM_ERR;
}

rbuf_hdl_t rbuf_deregister(rbuf_hdl_t hdl) {
    rbuf_t *rbuf = get_rbuf(hdl);
    if (rbuf == NULL) {
        return hdl;
    }
    *rbuf = rbuf_clear;
    return -1;
}

/*
 * Count of char in line with handle hdl
 */
uint16_t rbuf_free(rbuf_hdl_t hdl) {
    uint16_t count = 0;
    rbuf_t *rbuf = get_rbuf(hdl);
    if (rbuf == NULL) {
        return count;
    }
    count = rbuf->buffer_size;
    if (!rbuf->empty) {
        if (rbuf->nxtRdIdx < rbuf->nxtWrIdx) {
            count = (rbuf->nxtRdIdx + rbuf->buffer_size) - rbuf->nxtWrIdx;
        } else if (rbuf->nxtRdIdx == rbuf->nxtWrIdx) {
            count = 0;
        } else if (rbuf->nxtRdIdx > rbuf->nxtWrIdx) {
            count = rbuf->nxtRdIdx - rbuf->nxtWrIdx;
        }
    }
    return count;
}

em_msg rbuf_write_byte(rbuf_hdl_t hdl, uint8_t byte) {
    em_msg res = EM_ERR;
    rbuf_t *rbuf = get_rbuf(hdl);
    if (rbuf == NULL) {
        return res;
    }
    if ((rbuf->empty) || (rbuf->nxtWrIdx != rbuf->nxtRdIdx)) {
        rbuf->buffer[rbuf->nxtWrIdx] = byte;
        rbuf->nxtWrIdx = (rbuf->nxtWrIdx + 1) % rbuf->buffer_size;
        /* ToDo protect by semaphore */
        rbuf->empty = false;
        res = EM_OK;
    }
    return res;
}

em_msg rbuf_read_byte(rbuf_hdl_t hdl, uint8_t *byte) {
    em_msg res = EM_ERR;
    rbuf_t *rbuf = get_rbuf(hdl);
    if ((rbuf == NULL) || (rbuf->buffer == NULL)) {
        return res;
    }
    if (!rbuf->empty) {
        *byte = rbuf->buffer[rbuf->nxtRdIdx];
        rbuf->nxtRdIdx = (rbuf->nxtRdIdx + 1) % rbuf->buffer_size;
        if (rbuf->nxtRdIdx == rbuf->nxtWrIdx) {
            rbuf->empty = true;
        }
        res = EM_OK;
    }

    return res;
}

em_msg rbuf_write_bytes(rbuf_hdl_t hdl, const uint8_t *bytes, int16_t count) {
    em_msg res = EM_ERR;
    rbuf_t *rbuf = get_rbuf(hdl);
    uint16_t free;
    if (rbuf == NULL) {
        return res;
    }
    free = rbuf_free(hdl);
    if (free >= count) {
        res = EM_OK;
        for (uint16_t i = 0; i < count && (res == EM_OK); i++) {
            res = rbuf_write_byte(hdl, bytes[i]);
        }
    }
    return res;
}

em_msg rbuf_read_bytes(rbuf_hdl_t hdl, uint8_t *bytes, int16_t *count) {
    em_msg res = EM_ERR;
    rbuf_t *rbuf = get_rbuf(hdl);
    uint16_t available;
    if ((rbuf == NULL) || (rbuf->buffer == NULL)) {
        return res;
    }
    available = rbuf->buffer_size - rbuf_free(hdl);
    if (!rbuf->empty) {
        *count = *count < available ? *count : available;
        res = EM_OK;
        for (uint16_t i = 0; i < *count && (res == EM_OK); i++) {
            res = rbuf_read_byte(hdl, &bytes[i]);
        }
    }
    return res;
}

em_msg rbuf_pull_line(rbuf_hdl_t hdl, uint8_t *bytes, int16_t *count) {
    em_msg res = EM_ERR;
    rbuf_t *rbuf = get_rbuf(hdl);
    if ((rbuf == NULL) || (rbuf->buffer == NULL)) {
        return res;
    }
    if ((line_buffer.empty) || (line_buffer.nxtLineWrIdx != line_buffer.nxtLineRdIdx)) {
        uint8_t toRead = MIN(*count, rbuf->buffer_size);
        *count = toRead;
        line_buffer.nxtLineWrIdx = (line_buffer.nxtLineWrIdx + 1) % line_buffer.valid_cnt;
        memcpy(bytes, rbuf->buffer, toRead);
        rbuf->dirty = false;
        res = EM_OK;
    }
    return res;
}

em_msg rbuf_push_line(rbuf_hdl_t hdl, const uint8_t *bytes, int16_t count) {
    em_msg res = EM_ERR;
    rbuf_t *rbuf = get_rbuf(hdl);
    if (rbuf == NULL) {
        return res;
    }
    if (!line_buffer.empty) {
        uint8_t toWrite = MIN(count, rbuf->buffer_size);
        line_buffer.nxtLineRdIdx = (line_buffer.nxtLineRdIdx + 1) % line_buffer.valid_cnt;
        memcpy(rbuf->buffer, bytes, toWrite);
        if (line_buffer.nxtLineRdIdx == line_buffer.nxtLineWrIdx) {
            line_buffer.empty = true;
        }
        rbuf->dirty = true;

        res = EM_OK;
    }
    return res;
}

em_msg rbuf_get_device(rbuf_hdl_t hdl, device_t *device, dev_func_t dev_type) {
    static rbuf_hdl_t my_hdl;
    em_msg res = EM_ERR;
    if (NULL == device) {
        if (DEV_NONE == dev_type) {
            return res;
        }
    } else {
        res = EM_OK;
        memset(device, 0, sizeof(device_t));
        if (dev_type & DEV_READ) {
            device->read = rbuf_read_bytes;
        }
        if (dev_type & DEV_LREAD) {
            device->read = rbuf_pull_line;
        }
        if (dev_type & DEV_WRITE) {
            device->write = rbuf_write_bytes;
        }
        if (dev_type & DEV_LWRITE) {
            device->write = rbuf_push_line;
        }
        device->user_data = (void *)&my_hdl;
    }
    return res;
}

#ifdef __cplusplus
}
#endif
