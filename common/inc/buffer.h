/*
 * buffer.h
 *
 *  Created on: Jul 8, 2025
 *      Author: badi
 */

#ifndef COMMON_INC_BUFFER_H_
#define COMMON_INC_BUFFER_H_
#include "common.h"
#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BUFFER_READY, // Buffer can be used
    BUFFER_USED,  // Data in it, can be used (means dirty)
    BUFFER_CNT
} state_e;

typedef enum {
    LINEAR, // Blocks when nothing is avalable
    RING,   // Ringbuffer
} b_type_e;

typedef struct buffer_s {
    state_e state;
    uint8_t *pl;   /* pointer to first byte used in buffer */
    uint8_t *mem;  /* Start of memory */
    clabel_u lbl;  /* first 3 chars of input string */
    int16_t first; // First byte used
    int16_t used;  // How much is used
    int16_t size;  // Size of buffer
    uint8_t id;    // id of buffer
    b_type_e type; // LINEAR oder RING
} buffer_t;
/*
 * Create a new buffer with given size field filled in, if doAlloc is true
 * the memory is claimed as well, The filled in struct
 * is afterwards returned
 */
em_msg buffer_check(const buffer_t *buffer, bool reduced);
em_msg buffer_free(buffer_t *buffer);
int16_t buffer_writeable(const buffer_t *buffer);
int16_t buffer_used(const buffer_t *buffer);
buffer_t *buffer_new(uint16_t size, b_type_e type);
buffer_t *buffer_new_buffer_t(buffer_t *buffer);
int16_t buffer_transfer(buffer_t *from, buffer_t *to);
em_msg buffer_reset(buffer_t *buffer);
em_msg buffer_clear(buffer_t *buffer);
em_msg buffer_set(buffer_t *buffer, const uint8_t *data, int16_t size);
em_msg buffer_get(buffer_t *buffer, uint8_t *data, int16_t *size);
clabel_u *buffer_get_clabel(buffer_t *buffer);
buffer_t * buffer_get_till_end(buffer_t *buffer);
bool buffer_is_used(buffer_t *buffer);
void buffer_print(const buffer_t *buffer, char *title);
#ifdef __cplusplus
}
#endif

#endif /* COMMON_INC_BUFFER_H_ */
