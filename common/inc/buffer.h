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
  BUFFER_READY, // Buffer can be used
  BUFFER_USED, // Data in it, can be used (means dirty)
  BUFFER_CNT
} state_e;

extern char *state2str[];
typedef struct buffer_s {
  state_e state;
  uint16_t size;
  uint8_t *pl;  /* pointer to first byte used in buffer */
  uint8_t *mem; /* Start of memory */
  uint8_t used; // Count of bytes used in this buffer
  uint8_t id;   // id of buffer
} buffer_t;
/*
 * Create a new buffer with given size field filled in, if doAlloc is true
 * the memory is claimed as well, The filled in struct
 * is afterwards returned
 */
buffer_t *buffer_new(uint16_t size);
buffer_t *buffer_new_buffer_t(buffer_t *buffer);
em_msg buffer_reset(buffer_t *buffer);
em_msg buffer_set(buffer_t *buffer, uint8_t *data, const uint32_t size);
em_msg buffer_get(buffer_t *buffer, uint8_t *data, uint16_t *size);
bool buffer_is_used(buffer_t *buffer);
void buffer_print(buffer_t *buffer, uint8_t nr);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_INC_BUFFER_H_ */
