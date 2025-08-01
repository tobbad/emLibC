/*
 * buffer.c
 *
 *  Created on: 8.07.2025
 *      Author: badi
 */
#include "buffer.h"

buffer_t * buffer_init(buffer_t *buffer, uint16_t size){
	if (buffer!=NULL){
		buffer->size = size;
		buffer->mem = malloc(buffer->size);
		buffer->pl = buffer->mem;
		memset(buffer->mem, 0, buffer->size);
		buffer->state = ZERO;
		buffer->user =0;
		memset(buffer->mem, 0, buffer->size);
	}
	return buffer;
}

buffer_t * buffer_new(uint16_t size){
	buffer_t *buffer = malloc(sizeof(buffer_t));
	memset(buffer , 0, sizeof(buffer_t));
	buffer_init(buffer, size);
	return buffer;
}
