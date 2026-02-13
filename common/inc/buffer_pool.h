/*
 * buffer_pool.h
 *
 *  Created on: Aug 1, 2025
 *      Author: badi
 */

#ifndef EMLIBC_COMMON_INC_BUFFER_POOL_H_
#define EMLIBC_COMMON_INC_BUFFER_POOL_H_

#include "common.h"
#include "buffer.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LINEAR, // Blocks when nothing is avalable
  RING,   // Ringbuffer
} bp_type_e;

typedef struct buffer_pool_s {
  uint8_t buffer_cnt;
  buffer_t **buffer; //Buffer array
  buffer_t **sbuffer;
  bp_type_e type;
  int8_t next;  // Is negativ if a linear buffer is used in ring the index of the next element
} buffer_pool_t;

buffer_pool_t *buffer_pool_new(uint8_t lcnt, uint8_t charCnt, bp_type_e type);
void  buffer_pool_free(buffer_pool_t *pool);
buffer_t *buffer_pool_get(buffer_pool_t *bp);
em_msg buffer_pool_return(buffer_pool_t *bp, buffer_t *buffer);
em_msg buffer_pool_print(buffer_pool_t *bp);

#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_BUFFER_POOL_H_ */
