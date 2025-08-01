/*
 * buffer_pool.h
 *
 *  Created on: Aug 1, 2025
 *      Author: badi
 */

#ifndef EMLIBC_COMMON_INC_BUFFER_POOL_H_
#define EMLIBC_COMMON_INC_BUFFER_POOL_H_

#include "common.h"
#define BUFFER_CNT 20
typedef bp_type_e {
	POOL,  // Filled with 0
	LIFO,  // Can be used
	FIFO,  // Data in it, can be used (means dirty)
};

typedef buffer_pool_s{
	uint8_t bufer_cnt;
	buffer_t *buffer;
	bp_type_e type;
}buffer_pool_t;

buffer_pool_t* buffer_pool_new(uint8_t cnt, bp_type_e type){
	buffer_pool_t * bp;
	bp = malloc(sizeof(buffer_pool_t));
	bp->type= type;
	return bp;
}

em_msg buffer_pool_add(buffer_pool_t bp, buffer_t *buffer)

#endif /* EMLIBC_COMMON_INC_BUFFER_POOL_H_ */
