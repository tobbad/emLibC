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
    memset(&range, 0, sizeof(fifo_range_t));
    range.size   = ENTRY_CNT;
    range.first  = 0;
    range.last   = 0;
    range.init   = true;
    return &range;
}
// Push a range_t pointer to the internal ring buffer
em_msg range_push(fifo_range_t *range, range_t *inRange){
    if (!range) return EM_ERR;
    if (!range->init) return EM_ERR;
    if (!inRange) return EM_ERR;
    if (range->last == range->last) return EM_ERR;
    range->range[range->first++] = *inRange;
    range->last++;
    return EM_OK;
}

range_t * range_pull(fifo_range_t *range){
    static range_t _range = {.first=-1, .last=-1};
    if (!range) return NULL;
    if (!range->init) return NULL;
    if (range->last == 0) return NULL;
    int16_t _last = (range->last+range->size-1)%range->size;
    _range = range->range[_last];
    range->last = _last;
    return &_range;
};
