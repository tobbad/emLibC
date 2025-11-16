/*
 * buffer.c
 *
 *  Created on: 8.07.2025
 *      Author: badi
 */
#include "buffer.h"
#include "state.h"

char *state2Str[BSTATE_CNT] = {(char *)&"READY", (char *)&"USED"};

buffer_t *buffer_init(buffer_t* buffer) {
    if (buffer->mem != 0)  return NULL;
    buffer->mem = malloc(buffer->size);
    buffer_reset(buffer);
    return buffer;
}

buffer_t *buffer_new(uint16_t size) {
    if (size == 0) return NULL;
    buffer_t *buffer = (buffer_t *)malloc(sizeof(buffer_t));
    memset(buffer, 0, sizeof(buffer_t));
    buffer->size = size;
    buffer->used = 0;
    buffer->mem = malloc(buffer->size);
    buffer_reset(buffer);
    return buffer;
}

em_msg buffer_reset(buffer_t *buffer) {
    em_msg res = EM_ERR;
    if (buffer == NULL)  return res;
    memset(buffer->mem, 0, buffer->size);
    buffer->pl = buffer->mem;
    buffer->state = READY;
    buffer->used = 0;
    buffer->id = 0;
    res = EM_OK;
    return res;
}

em_msg buffer_set(buffer_t *buffer, uint8_t *data, const uint16_t size) {
    em_msg res = EM_ERR;
    uint16_t msize = MIN(size, buffer->size);
    buffer->size = msize;
    memcpy(buffer->mem, data, buffer->size);
    buffer->state = USED;
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
    buffer->state = READY;
    *size=msize;
    return res;
}

bool buffer_is_used(buffer_t *buffer) { return buffer->state == USED; }

void buffer_print(buffer_t *buffer, uint8_t nr) {
    printf("Info of buffer  (%2d): @%p" NL, nr, buffer);
    printf("Data                : @%p" NL, buffer->mem);
    printf("Char count          : %2d" NL, buffer->size);
    printf("Char used           : %d" NL, buffer->used);
    printf("Buffer state is     : %s" NL, state2Str[buffer->state]);
}
