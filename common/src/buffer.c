/*
 * buffer.c
 *
 *  Created on: 8.07.2025
 *      Author: badi
 */
#include "buffer.h"
buffer_t * buffer_init(buffer_t *buffer, uint16_t size, bool doAlloc){
	memset(buffer, 0, sizeof(buffer_t));
	buffer->size = size;
	if (doAlloc){
		buffer->mem = malloc(size);
		buffer->pl = buffer->mem;
		memset(buffer->mem, 0, size);
	}

}
