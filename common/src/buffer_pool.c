/*
 * buffer_pool.c
 *
 *  Created on: Aug 1, 2025
 *      Author: badi
 */
#include <assert.h>
#include "common.h"
#include "buffer.h"
#include "buffer_pool.h"
buffer_pool_t* buffer_pool_new( uint8_t lcnt, uint8_t charCnt){
    buffer_pool_t *bp=NULL;
	bp = malloc(sizeof(buffer_pool_t));
	assert(bp!=NULL);
	memset(bp, 0, sizeof(buffer_pool_t));
	bp->buffer_cnt = lcnt;
	bp->buffer = (buffer_t *)malloc(bp->buffer_cnt*sizeof(buffer_t*));
	printf("Bufferarray= %p"NL, bp->buffer);
	memset(bp->buffer, 0, bp->buffer_cnt*sizeof(buffer_t*));
	if (bp->buffer==NULL) return NULL;
	for (uint8_t i=0;i<bp->buffer_cnt;i++){
		bp->buffer[i] = *buffer_new(charCnt);
	}
	printf("Buffer pool is %p"NL, bp);
	buffer_pool_print(bp);
    return bp;
}

buffer_t * buffer_pool_get(buffer_pool_t *bp){
	if (bp==NULL) return NULL;
	buffer_t * buffer=NULL;
	for (uint8_t i=0;i<bp->buffer_cnt;i++){
		if (!buffer_is_used(&bp->buffer[i])) {
			buffer = &bp->buffer[i];
			buffer->id= i;
			buffer->state = USED;
		}
	}
	return buffer;
}
em_msg buffer_pool_return(buffer_pool_t *bp, buffer_t * buffer){
    em_msg res = EM_ERR;
    if (bp==NULL) return res;
    if (buffer==NULL) return res;
    res = buffer_reset(&bp->buffer[buffer->id]);
    res = EM_OK;
    return res;
}
em_msg buffer_pool_print(buffer_pool_t *bp){
    em_msg res = EM_ERR;
    if (bp==NULL) {
        printf("Buffer pool is NULL"NL);
        return res;
    }
    printf("Buffer pool %p with %d buffers and type %d:"NL, bp, bp->buffer_cnt, bp->type);
    for (uint8_t i=0;i<bp->buffer_cnt;i++){
        buffer_print(&bp->buffer[i], i);
        return EM_OK;
    }
    res = EM_OK;
    return res;
}

