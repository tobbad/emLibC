/*
 * buffer.c
 *
 *  Created on: 8.07.2025
 *      Author: badi
 */
#include "buffer.h"
#include "state.h"

char *state2Str[BUFFER_CNT] = {(char *)&"BUFFER_READY", (char *)&"USED"};

buffer_t *buffer_new(uint16_t size) {
    buffer_t *buffer = NULL;
    // clang-format off
    if (size == 0) return buffer;
    // clang-format on
    buffer = (buffer_t *)malloc(sizeof(buffer_t));
    memset(buffer, 0, sizeof(buffer_t));
    buffer->mem = malloc(size);
    memset(buffer->mem, 0, size);
    buffer->size = size;
    buffer_reset(buffer);
    return buffer;
}

buffer_t *buffer_new_buffer_t(buffer_t *buffer) {
    // clang-format off
    if (buffer->size == 0) return NULL;
    // clang-format on
    size_t size = buffer->size;
    memset(buffer, 0, sizeof(buffer_t));
    buffer->mem = malloc(size);
    memset(buffer->mem, 0, size);
    buffer->size = size;
    buffer_reset(buffer);
    return buffer;
}
em_msg buffer_reset(buffer_t *buffer) {
    // clang-format off
    em_msg res = EM_ERR;
    if (buffer == NULL) return res;
    if (buffer->mem == NULL) return res;
    // clang-format on
    uint8_t *m = buffer->mem;
    memset(m, 0, buffer->size);
    buffer->pl = buffer->mem;
    buffer->state = BUFFER_READY;
    buffer->used = 0;
    buffer->id = 0;
    res = EM_OK;
    return res;
}

em_msg buffer_set(buffer_t *buffer, uint8_t *data, const uint32_t size) {
    em_msg res = EM_ERR;
    uint16_t msize = MIN(size, buffer->size);
    buffer->size = msize;
    memcpy(buffer->mem, data, buffer->size);
    buffer->state = BUFFER_USED;
    res = EM_OK;
    return res;
}

em_msg buffer_get(buffer_t *buffer, uint8_t *data, uint16_t *size) {
    em_msg res = EM_OK;
    if (*size > buffer->size) {
        *size = buffer->size;
        res = EM_ERR;
    }
    uint16_t msize = MIN(*size, buffer->size);
    memcpy(data, buffer->mem, msize);
    buffer->state = BUFFER_READY;
    *size = msize;
    return res;
}

bool buffer_is_used(buffer_t *buffer) { return buffer->state == BUFFER_USED; }

void buffer_print(buffer_t *buffer) {
    if (buffer == NULL) {
        printf("buffer is NULL" NL);
        return;
    }
    printf("Info of buffer        = 0x%p"NL, buffer);
    printf("id                    = %d" NL, buffer->id);
    printf("Data                  = 0x%p" NL, buffer->mem);
    print_buffer(buffer->mem, buffer->size, "Buffer content");
    printf("buffer size           = %1d" NL, buffer->size);
    printf("buffer used           = %d" NL, buffer->used);
    printf("buffei id             = %d" NL, buffer->id);
    printf("Buffer state is       = %s" NL, state2Str[buffer->state]);
}
