/*
 * buffer_pool.c
 *
 *  Created on: Aug 1, 2025
 *      Author: badi
 */
#include "common.h"
#include "buffer.h"
#include "buffer_pool.h"
buffer_pool_t* buffer_pool_new(uint8_t bcnt, uint8_t charCnt){
    buffer_pool_t *bp;
	bp = malloc(sizeof(buffer_pool_t));
	memset(bp, 0, sizeof(buffer_pool_t));
	bp->bufer_cnt = bcnt;
	bp->buffer = (buffer_t *)malloc(bp->bufer_cnt*sizeof(buffer_t*));
	bp->sbuffer = (buffer_t *)malloc(bp->bufer_cnt*sizeof(buffer_t*));
	memset(bp->sbuffer, 0, bp->bufer_cnt*sizeof(buffer_t*));
	if (bp->buffer!=NULL) return NULL;
	for (uint8_t i=0;i<bcnt;i++){
		bp->buffer[i] = *buffer_new(charCnt);
	}
    return bp;
}

buffer_t * buffer_pool_get(buffer_pool_t *bp){
	if (bp==NULL) return NULL;
	buffer_t * buffer=NULL;
	for (uint8_t i=0;i<bp->bufer_cnt;i++){
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

