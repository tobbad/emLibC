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

#define BUFFER_CNT 20
typedef enum {
	POOL,  // Filled with 0
	RING,  // Ringbuffer
} bp_type_e;

typedef struct buffer_pool_s{
	uint8_t bufer_cnt;
	buffer_t *buffer[BUFFER_CNT];
	buffer_t *hbuffer[BUFFER_CNT]; // hidden buffers
	bp_type_e type;
	uint8_t next;
}buffer_pool_t;

buffer_pool_t* buffer_pool_new(uint8_t bcnt, bp_type_e type);
em_msg buffer_pool_add(buffer_pool_t *bp, buffer_t *buffer);
buffer_t * buffer_pool_get(buffer_pool_t *bp, uint8_t index);
em_msg buffer_pool_set(buffer_pool_t *bp, uint8_t index, buffer_t *buffer);
#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_BUFFER_POOL_H_ */
