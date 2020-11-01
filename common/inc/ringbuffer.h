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
#include "device.h"
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
	uint8_t *buffer;
    uint16_t buffer_size;
} rbuf_t;



typedef int8_t rbuf_hdl_t;

/* 
 * Reset all fields 
 * even sets the buffer to NULL without freeing it!!!
 */
extern const rbuf_t rbuf_clear;

void rbuf_init(void);
rbuf_hdl_t rbuf_register(rbuf_t *rbuf);
rbuf_hdl_t rbuf_deregister(rbuf_hdl_t hdl);

uint16_t rbuf_free(rbuf_hdl_t hdl);
elres_t rbuf_write_byte(rbuf_hdl_t hdl, uint8_t byte);
/*
 * Only succeeds if there are count bytes free in the buffer
 */
elres_t rbuf_write_bytes(rbuf_hdl_t hdl, const uint8_t* bytes, uint16_t count);

elres_t rbuf_read_byte(rbuf_hdl_t hdl, uint8_t *byte);
/*
 * Read at most count (input) bytes, really count of bytes is in the returned count value
 */
elres_t rbuf_read_bytes(rbuf_hdl_t hdl, uint8_t *bytes, uint16_t *count);

elres_t rbuf_get_device(rbuf_hdl_t hdl, device_t *device, dev_func_t dev_type);

#ifdef __cplusplus
}
#endif
#endif /* EMLIBC_COMMON_INC_RINGBUFFER_H_ */
