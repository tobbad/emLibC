/*
 * ringbuffer.h
 *
 *  Created on: Apr 26, 2020
 *      Author: badi
 */

#ifndef EMLIBC_COMMON_INC_RINGBUFFER_H_
#define EMLIBC_COMMON_INC_RINGBUFFER_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"
#include "ringbuffer_config.h"
/*
 * wrIdx Points to the location for the next write
 * rdIdx Points to the location for the next read
 *
 * if rdIdx == wrIdx: Ringbuffer is empty
 * if wrIdx
 */
typedef struct rbuf_t_
{
	bool empty;
	uint16_t nxtWrIdx;
	uint16_t nxtRdIdx;
	uint8_t data[RBUF_BUFFERSIZE];
} rbuf_t;

extern const rbuf_t rbuf_clear;

#ifdef __cplusplus
}
#endif
#endif /* EMLIBC_COMMON_INC_RINGBUFFER_H_ */
