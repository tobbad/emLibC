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
typedef int8_t rbuf_hdl_t;

/*
 * wrIdx Points to the location for the next write
 * rdIdx Points to the location for the next read
 *
 * if rdIdx == wrIdx: Ringbuffer is empty
 * if wrIdx
 */
typedef struct rbuf_t_ {
  bool empty;
  uint16_t nxtWrIdx;
  uint16_t nxtRdIdx;
  uint8_t *buffer;
  uint16_t buffer_size;
  bool dirty;
} rbuf_t;

typedef struct rbufm_s {
  bool empty;
  uint16_t nxtWrIdx;
  uint16_t nxtRdIdx;
  uint8_t *buffer;
  uint16_t buffer_size;
} rbufm_t;

typedef struct rbufline_s {
  rbuf_t rbuf_reg[RBUF_REGISTERS];
  bool empty;
  rbuf_hdl_t current;
  uint16_t nxtLineWrIdx;
  uint16_t nxtLineRdIdx;
  int16_t valid_cnt;
} rbufline_t;
/*
 * Reset all fields
 * even sets the buffer to NULL without freeing it!!!
 */
extern const rbuf_t rbuf_clear;

void rbuf_init(void);

rbuf_hdl_t rbuf_register(uint8_t *buffer, uint16_t cnt);
rbuf_hdl_t rbuf_deregister(rbuf_hdl_t hdl);

uint16_t rbuf_free(rbuf_hdl_t hdl);
em_msg rbuf_write_byte(rbuf_hdl_t hdl, uint8_t byte);
/*
 * Only succeeds if there are count bytes free in the buffer
 */
em_msg rbuf_read_byte(rbuf_hdl_t hdl, uint8_t *byte);
/*
 * Read at most count (input) bytes, really count of bytes is in the returned
 * count value
 */
em_msg rbuf_write_bytes(rbuf_hdl_t hdl, const uint8_t *bytes, int16_t count);
em_msg rbuf_read_bytes(rbuf_hdl_t hdl, uint8_t *bytes, int16_t *count);
em_msg rbuf_get_device(rbuf_hdl_t hdl, device_t *device, dev_func_t dev_type);

em_msg rbuf_pull_line(rbuf_hdl_t hdl, uint8_t *bytes, int16_t *count);
em_msg rbuf_push_line(rbuf_hdl_t hdl, const uint8_t *bytes, int16_t count);

#ifdef __cplusplus
}
#endif
#endif /* EMLIBC_COMMON_INC_RINGBUFFER_H_ */
