/*
 * buffer.h
 *
 *  Created on: Jul 8, 2025
 *      Author: badi
 */

#ifndef COMMON_INC_BUFFER_H_
#define COMMON_INC_BUFFER_H_
#include "common.h"

typedef struct buffer_s {
	int8_t   ready;
    uint16_t size;
    uint8_t* pl;   /* pointer to first byte used in buffer */
    uint8_t* mem;  /* Start of memory */
} buffer_t;
/*
 * Create a new buffer with given size field filled in, if doAlloc is true
 * the memory is claimed as well, The filled in struct
 * is afterwards returned
 */
buffer_t * buffer_init(buffer_t *buffer, uint16_t size, bool doAlloc);
buffer_t * buffer_new(uint16_t size);

#endif /* COMMON_INC_BUFFER_H_ */
