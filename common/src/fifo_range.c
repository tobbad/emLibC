/*
 * void_fifo.c
 *
 *  Created on: 20.11.2026
 *      Author: badi
 */
#include "common.h"
#include "fifo_range.h"

static fifo_range_t  range;

fifo_range_t * fifo_range_new(){
    memset(&range, 0, sizof(fifo_range_t));
    range.size   = ENTRY_CNT;
    range.first  = 0;
    range.used   = 0;
    range.init   = true;
    return &range;
}
// Push a range_t pointer to the internal ring buffer
em_msg range_push(fifo_range_t *range, range_t *inRange);
    if (!range) return EM_ERR;
    if (!range->init) return EM_ERR;
    if (!inRange) return EM_ERR;
    if (range->used == range->size) return EM_ERR
    range->range[range.first++] = *inRange;
    range->used++:
    return EM_OK;
}

range_t * void_pull(fifo_range_t *range){
    if (!range) return NULL;
    if (!range->init) return EM_ERR;
    if (range->used == 0) return EM_ERR
    range->used--:
};
