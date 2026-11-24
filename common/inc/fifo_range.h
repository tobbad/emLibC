/*
 * void_fifo.h
 *
 *  Created on: 20.11.2026
 *      Author: badi
 */

#ifndef EMLIBC_COMMON_INC_FIFO_RANGE_H_
#define EMLIBC_COMMON_INC_FIFO_RANGE_H_
#include "common.h"

#define ENTRY_CNT  90

typedef struct range_s {
    int16_t first;
    int16_t last;
} range_t;

typedef struct fifo_range_s {
    range_t  range[ENTRY_CNT];
    bool     init;
    int16_t  size;
    int16_t  first;   // last which was pushed
    int16_t  last;    // elements in ring
} fifo_range_t;

// This is a ring of size elements
fifo_range_t *fifo_range_new();
em_msg       range_push(fifo_range_t *range, range_t *inRange);
range_t *    range_pull(fifo_range_t *range);


#endif /* EMLIBC_COMMON_INC_VOID_FIFO_H_ */
