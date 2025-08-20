/*
 * buffer.c
 *
 *  Created on: 8.07.2025
 *      Author: badi
 */
#include "buffer.h"
#include "state.h"

buffer_t * buffer_init(buffer_t *buffer, uint16_t size){
	if (buffer!=NULL){
		buffer->size = size;
		buffer->mem = malloc(buffer->size);
		buffer->pl = buffer->mem;
		buffer_reset(buffer);
	}
	return buffer;
}

buffer_t * buffer_new(uint16_t size){
	if (size==0) return NULL;
	buffer_t *buffer = malloc(sizeof(buffer_t));
	memset(buffer , 0, sizeof(buffer_t));
	buffer_init(buffer, size);
	return buffer;
}

em_msg  buffer_reset(buffer_t *buffer){
	em_msg res = EM_ERR;
	if (buffer==NULL) return res;
	memset(buffer->mem, 0, buffer->size);
	buffer->state = ZERO;
	buffer->used =0;
	buffer->id = 0;
	res = EM_OK;
	return res;
}

em_msg  buffer_set(buffer_t *buffer, uint8_t* data, const uint8_t size){
	em_msg res = EM_ERR;
	uint8_t msize = MIN(size, buffer->size);
	buffer->size = msize;
	memcpy(buffer->mem, data, buffer->size);
	buffer->state = USED;
	res = EM_OK;
	return res;
}

em_msg  buffer_get(buffer_t *buffer, uint8_t* data, uint8_t *size){
	em_msg res = EM_OK;
	if (*size>buffer->size){
		*size = buffer->size;
		res = EM_ERR;
	}
	uint8_t msize = MIN(*size, buffer->size);
	memcpy(data,buffer->mem, msize);
	return res;

}

bool buffer_is_used(buffer_t *buffer){
	return buffer->state!=ZERO;
}

