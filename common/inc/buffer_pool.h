/*
 * buffer_pool.h
 *
 *  Created on: Aug 1, 2025
 *      Author: badi
 */

#ifndef EMLIBC_COMMON_INC_BUFFER_POOL_H_
#define EMLIBC_COMMON_INC_BUFFER_POOL_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  POOL, // Filled with 0
  RING, // Ringbuffer
} bp_type_e;

typedef struct buffer_pool_s {
  uint8_t buffer_cnt;
  buffer_t *buffer;
  buffer_t *sbuffer;
  bp_type_e type;
} buffer_pool_t;

buffer_pool_t *buffer_pool_new(uint8_t lcnt, uint8_t charCnt);
buffer_t *buffer_pool_get(buffer_pool_t *bp);
em_msg buffer_pool_return(buffer_pool_t *bp, buffer_t *buffer);
em_msg buffer_pool_print(buffer_pool_t *bp);

#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_BUFFER_POOL_H_ */
