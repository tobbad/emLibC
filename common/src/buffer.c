/*
 * buffer.c
 *
 *  Created on: 8.07.2025
 *      Author: badi
 */
#include "buffer.h"
#include "state.h"

char *state2Str[BUFFER_CNT] = {(char *)&"BUFFER_READY", (char *)&"BUFFER_USED"};

buffer_t *buffer_new(uint16_t size) {
    buffer_t *buffer = NULL;
    // clang-format off
    if (size == 0) return buffer;
    // clang-format on
    buffer = (buffer_t *)calloc(1, sizeof(buffer_t));
    if (!buffer) return NULL ;
    buffer->mem = calloc(1, size);
    if (!buffer->mem){
        free(buffer);
        return NULL;
    }
    buffer->size = size;
    buffer_reset(buffer);
    return buffer;
}
buffer_t *buffer_free(buffer_t *buffer) {
    // clang-format off
    if (!buffer) return NULL;
    // clang-format on
    free(buffer->mem);
    free(buffer);
    buffer =NULL; 
    return buffer;
}

buffer_t *buffer_new_buffer_t(buffer_t *buffer) {
    // clang-format off
    if (buffer->size == 0) return NULL;
    // clang-format on
    size_t size = buffer->size;
    memset(buffer, 0, sizeof(buffer_t));
    buffer->mem = calloc(1, size);
    if (!buffer->mem) return NULL;
    buffer->size = size;
    buffer_reset(buffer);
    return buffer;
}
em_msg buffer_reset(buffer_t *buffer) {
    // clang-format off
    em_msg res = EM_ERR;
    if (!buffer || !buffer->mem)  return res;
   // clang-format on
    memset(buffer->mem, 0, buffer->size);
    buffer->pl = buffer->mem;
    buffer->state = BUFFER_READY;
    buffer->used = 0;
    buffer->id = 0;
    res = EM_OK;
    return res;
}

em_msg buffer_set(buffer_t *buffer, const uint8_t *data,  int16_t *size) {
    em_msg res = EM_ERR;
    if (!buffer || !buffer->mem || !data || !size)  return res;
    res = EM_OK;
    int16_t msize = MIN(*size, buffer->size);
    memcpy(buffer->mem, data, msize);
    buffer->used = msize;
    *size = msize;
    buffer->state = BUFFER_USED;
    return res;
}

em_msg buffer_get(buffer_t *buffer, uint8_t *data, int16_t *size) {
    em_msg res = EM_ERR;
    if (!buffer || !buffer->mem || !data || !size) {
        printf("Precondition not fullfilled"NL);
        return res;
    };
    if (buffer->state ==BUFFER_READY) return res;
    res = EM_OK;
    int16_t msize = MIN(*size, buffer->size);
    memcpy(data, buffer->mem, msize);
    buffer->state = BUFFER_READY;
    buffer->used = 0;
    *size = msize;
    return res;
}

bool buffer_is_used(buffer_t *buffer) { return buffer->state == BUFFER_USED; }

void buffer_print(buffer_t *buffer, char *title) {
    if (buffer == NULL) {
        printf("buffer is NULL" NL);
        return;
    }
    if (title!=NULL) printf("%s"NL, title);
    printf("Info of buffer        = 0x%p"NL, buffer);
    printf("id                    = %d" NL, buffer->id);
    printf("Data                  = 0x%p" NL, buffer->mem);
    print_buffer(buffer->mem, buffer->size, "Buffer content");
    printf("buffer size           = %1d" NL, buffer->size);
    printf("buffer used           = %d" NL, buffer->used);
    printf("buffei id             = %d" NL, buffer->id);
    printf("Buffer state is       = %s" NL, state2Str[buffer->state]);
}
