/*
 * buffer.c
 *
 *  Created on: 8.07.2025
 *      Author: badi
 */
#include "buffer.h"
#include "state.h"
#define TALKING

char *state2Str[BUFFER_CNT] = {(char *)&"BUFFER_READY", (char *)&"BUFFER_USED"};
char *type2Str[BUFFER_CNT] = {(char *)&"LINEAR", (char *)&"RING"};
#ifndef TALKING
em_msg buffer_check(const buffer_t *buffer) {
    // clang-format off
    int16_t res = EM_ERR;
    if (!buffer || !buffer->mem) return res;
    if (buffer->size==0) return res;
    // clang-format on
    return EM_OK;
}
#else
em_msg buffer_check(const buffer_t *buffer, bool reduced) {
    int16_t res = EM_ERR;
    if (!buffer){
        printf("buffer is NULL"NL);
        return res;
    } 
    if (buffer->size==0){
        printf("buffer->size is 0"NL);
        return res;
    } 
    if (!reduced){
        if (!buffer->mem){
            printf("buffer->mem is NULL"NL);
            return res;
        }
    }
    return EM_OK;
}
#endif

buffer_t *buffer_new(uint16_t size, b_type_e type) {
    buffer_t *buffer = NULL;
    // clang-format off
    if (size == 0) return buffer;
    buffer = (buffer_t *)calloc(1, sizeof(buffer_t));
    if (!buffer) return NULL;
    // clang-format on
    buffer->mem = calloc(1, size);
    if (!buffer->mem) {
        free(buffer);
        return NULL;
    }
    buffer->type  = type;
    buffer->size  = size;
    buffer->first = 0;
    buffer_reset(buffer);
    return buffer;
}

em_msg buffer_free(buffer_t *buffer) {
    // clang-format off
    em_msg res = buffer_check(buffer, false);
    if (res == EM_ERR) return res;
    // clang-format off
    // printf("buffer_free(%p)"NL, buffer);
    if (!buffer) return res;
    if (buffer->state == BUFFER_USED) return EM_ERR;
    // clang-format on
    free(buffer->mem);
    free(buffer);
    return EM_OK;
}

buffer_t *buffer_new_buffer_t(buffer_t *buffer) {
    // clang-format off
    em_msg res = buffer_check(buffer, true);
    if (res == EM_ERR) return NULL;
    size_t size = buffer->size;
    b_type_e type = buffer->type;
    memset(buffer, 0, sizeof(buffer_t));
    buffer->mem = calloc(1, size);
    if (!buffer->mem) return NULL;
    // clang-format on
    buffer->type  = type;
    buffer->size  = size;
    buffer->first = 0;
    buffer_reset(buffer);
    return buffer;
}

int16_t buffer_transfer(buffer_t *from, buffer_t *to){
    // clang-format off
    em_msg res = buffer_check(from, false);
    if (res == EM_ERR) return res;
    res = buffer_check(to, false);
    if (res == EM_ERR) return res;
    // clang-format on
    if (from->type==LINEAR){
        if (to->type==LINEAR){
            memcpy(to->mem, from->mem, from->used);
            to->used = from->used;
            from->used = 0;
            from->state = BUFFER_READY;
        } else if (to->type==RING){
            buffer_set(to, from->mem, from->used);
            from->state = BUFFER_READY;
        }
    } else {
        if (to->type==LINEAR){
            memcpy(to->mem, from->mem, from->used);
            to->used = from->used;
            from->used = 0;
            from->state = BUFFER_READY;
        } else if (to->type==RING){
            buffer_set(to, from->mem, from->used);
            from->state = BUFFER_READY;
        }
    }
    return from->used;
}

em_msg buffer_reset(buffer_t *buffer) {
    // clang-format off
    em_msg res = buffer_check(buffer, false);
    if (res == EM_ERR) return res;
    // clang-format on
    memset(buffer->mem, 0, buffer->size);
    memset(buffer->lbl.str,0, CMD_LEN);
    buffer->pl = buffer->mem;
    buffer->state = BUFFER_READY;
    buffer->used = 0;
    buffer->first = 0;
    buffer->id = 0;
    res = EM_OK;
    return res;
}

em_msg buffer_clear(buffer_t *buffer){
    // clang-format off
    em_msg res = buffer_check(buffer, false);
    if (res == EM_ERR) return res;
    // clang-format on
    buffer->first = (buffer->first+buffer->used)%buffer->size;
    buffer->used = 0;
    buffer->state = BUFFER_READY;
    memset(buffer->lbl.str,0, CMD_LEN);
    return res;
};

int16_t buffer_used(const buffer_t *buffer) {
    return buffer->used;
}

int16_t buffer_writeable(const buffer_t *buffer) {
    return buffer->size - buffer->used;
}

em_msg buffer_set(buffer_t *buffer, const uint8_t *data, int16_t size) {
    em_msg res = buffer_check(buffer, false);
    if (res == EM_ERR) return res;

    if (size <= 0) return EM_ERR;

    int16_t writable = buffer_writeable(buffer);
    if (size > writable) return EM_ERR;

    if (buffer->type == LINEAR) {
        memcpy(buffer->mem, data, size);
        buffer->first = 0;
        buffer->used  = size;
    } else {
        int16_t write_pos = (buffer->first + buffer->used) % buffer->size;
        int16_t space_to_end = buffer->size - write_pos;

        if (size <= space_to_end) {
            memcpy(&buffer->mem[write_pos], data, size);
            buffer->lbl.cmd=0;
            memcpy(buffer->lbl.str, data, MIN(CMD_LEN-1, size));
        } else {
            memcpy(&buffer->mem[write_pos], data, space_to_end);
            memcpy(&buffer->mem[0], data + space_to_end, size - space_to_end);
            buffer->lbl.cmd=0;
            memcpy(buffer->lbl.str, data, MIN(CMD_LEN-1, size));
        }

        buffer->used += size;
    }

    buffer->state = BUFFER_USED;
    return EM_OK;
}

em_msg buffer_get(buffer_t *buffer, uint8_t *data, int16_t *size) {
    em_msg res = buffer_check(buffer, false);
    if (res == EM_ERR) return res;

    if (*size <= 0) return EM_ERR;

    int16_t readable = buffer_used(buffer);
    if (readable == 0) {
        *size = 0;
        return EM_OK;
    }

    *size = MIN(*size, readable);

    if (buffer->type == LINEAR) {
        memcpy(data, buffer->mem, *size);
        buffer->used = 0;
    } else {
        int16_t data_to_end = buffer->size - buffer->first;

        if (*size <= data_to_end) {
            memcpy(data, &buffer->mem[buffer->first], *size);
            memcpy(buffer->lbl.str, data, MIN(CMD_LEN-1, *size));
        } else {
            memcpy(data, &buffer->mem[buffer->first], data_to_end);
            memcpy(buffer->lbl.str, data, MIN(CMD_LEN-1, data_to_end));
            memcpy(data + data_to_end, &buffer->mem[0], *size - data_to_end);
        }

        buffer->first = (buffer->first + *size) % buffer->size;
        buffer->used -= *size;
    }

    if (buffer->used == 0) {
        buffer->state = BUFFER_READY;
    }

    return EM_OK;
}


clabel_u *buffer_get_clabel(buffer_t *buffer) {
    // clang-format off
    em_msg res = buffer_check(buffer, false);
    if (res == EM_ERR) return NULL;
    return &buffer->lbl;
}

buffer_t * buffer_get_till_end(buffer_t *buffer) {
    static buffer_t _buffer;
    int16_t data_to_end = buffer->size - buffer->first;
    _buffer.size = buffer->size;
    _buffer.used = buffer->used;
    _buffer.mem  = &buffer->mem[buffer->first];
    return &_buffer;
}

bool buffer_is_used(buffer_t *buffer) { 
    return buffer->state == BUFFER_USED;
}

void buffer_print(const buffer_t *buffer, char *title) {
    // clang-format off
    em_msg res = buffer_check(buffer, false);
    if (res == EM_ERR) return;
    if (title != NULL) printf("%s" NL, title);
    // clang-format on
    printf("Info of buffer      = 0x%p" NL, buffer);
    printf("size                = %d" NL, buffer->size);
    printf("id                  = %d" NL, buffer->id);
    printf("Data                = 0x%p" NL, buffer->mem);
    printf("buffer first        = %1d" NL, buffer->first);
    printf("buffer used         = %d" NL, buffer_used(buffer));
    printf("buffer writable     = %d" NL, buffer_writeable(buffer));
    printf("buffei id           = %d" NL, buffer->id);
    printf("Buffer state is     = %s" NL, state2Str[buffer->state]);
    printf("Buffer type is      = %s" NL, type2Str[buffer->type]);
    print_buffer(buffer->mem, buffer->size, "Full content");
}
