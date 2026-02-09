/*
 * buffer.c
 *
 *  Created on: 8.07.2025
 *      Author: badi
 */
#include "buffer.h"
#include "state.h"

char *state2Str[BUFFER_CNT] = {(char *)&"BUFFER_EMPTY", (char *)&"BUFFER_USED"};

buffer_t *buffer_new(uint16_t size) {
    buffer_t *buffer = NULL;
    // clang-format off
    if (buffer->size == 0) return buffer;
    // clang-format on
    buffer = (buffer_t *)malloc(sizeof(buffer_t));
    memset(buffer, 0, sizeof(buffer_t));
    buffer->mem = malloc(size);
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
    buffer->state = BUFFER_EMPTY;
    buffer->used = 0;
    buffer->id = 0;
    res = EM_OK;
    return res;
}

em_msg buffer_set(buffer_t *buffer, uint8_t *data, const uint32_t size) {
    // clang-format off
    em_msg res = EM_ERR;
    if (buffer == NULL) return res;
    if (buffer->mem == NULL) return res;
    // clang-format on
    uint16_t msize = MIN(size, buffer->size-1);
    buffer->used = msize;
    memcpy(buffer->mem, data, msize);
    buffer->mem[msize+1] =0;
    buffer->state = BUFFER_USED;
    res = EM_OK;
    return res;
}

em_msg buffer_get(buffer_t *buffer, uint8_t *data, uint16_t *size) {
    // clang-format off
    em_msg res = EM_ERR;
    if (buffer == NULL) return res;
    if (buffer->mem == NULL) return res;
    // clang-format on
    uint16_t msize = MIN(*size-1, buffer->used);
    memcpy(data, buffer->mem, msize);
    buffer->state = BUFFER_EMPTY;
    buffer->used = 0;
    *size = msize;
    return res;
}

bool buffer_is_used(buffer_t *buffer) {
    // clang-format off
    em_msg res = EM_ERR;
    if (buffer == NULL) return res;
    if (buffer->mem == NULL) return res;
    // clang-format on

    return buffer->state == BUFFER_USED; }

void buffer_print(buffer_t *buffer, uint8_t nr) {
    // clang-format off
    em_msg res = EM_ERR;
    if (buffer == NULL) return;
    if (buffer->mem == NULL) return;
    // clang-format on
    printf("Info of buffer nr=%1d = 0x@%p" NL, nr, buffer);
    printf("Data                  = 0x0@%p" NL, buffer->mem);
    printf("buffer size           = %1d" NL, buffer->size);
    printf("buffer used           = %d" NL, buffer->used);
    printf("buffei id             = %d" NL, buffer->id);
    printf("Buffer state is       = %s" NL, state2Str[buffer->state]);
}
