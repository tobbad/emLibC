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
	ZERO,   // Filled with 0
	EMPTY,  // Can be used
	READY,  // Data in it, can be used (means dirty)
	LOCKED, // Is currently used
	BSTATE_CNT
} state_e ;

typedef struct buffer_s {
	state_e  state;
    uint16_t size;
    uint8_t* pl;   /* pointer to first byte used in buffer */
    uint8_t* mem;  /* Start of memory */
    uint8_t used; // Count of bytes used in this buffer
    uint8_t user; // Count of user using this buffer
} buffer_t;
/*
 * Create a new buffer with given size field filled in, if doAlloc is true
 * the memory is claimed as well, The filled in struct
 * is afterwards returned
 */
buffer_t * buffer_init(buffer_t *buffer, uint16_t size);
buffer_t * buffer_new(uint16_t size);


#ifdef __cplusplus
}
#endif

#endif /* COMMON_INC_BUFFER_H_ */
