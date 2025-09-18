/*
 * pbuf.h
 *
 *  Created on: 26.10.2020
 *      Author: badi
 */

#ifndef COMMON_INC_PBUF_H_
#define COMMON_INC_PBUF_H_
#include "common.h"
#include <stdint.h>

#define BLOCK_SIZE 64

typedef struct refcnt_flags_t_ {
  uint8_t ref : 4;
  uint8_t flags : 4;
} refcnt_flags_t;

typedef struct pbuf_t_ {
  struct pbuf_t_ *next;
  uint32_t *payload;
  uint16_t total_length; /* Sum of all length pointed to by * next + len */
  uint16_t len;          /* Size data content in this pbuf */
  refcnt_flags_t rcfl;
  uint8_t reserved;
} pbuf_t;

elres_t pbuf_init(uint8_t *buffer, uint32_t size);
uint8_t *pbuf_alloc();

#endif /* COMMON_INC_PBUF_H_ */
