/*
 * mutex.c
 *
 *  Created on: Jan 11, 2025
 *      Author: badi
 */
#include "common.h"
#include "mutex.h"
#include "cmsis_gcc.h"
bool ReadModify_write(uint8_t *mem, int8_t add){
	do {
		uint8_t val = __LDREXB(mem);
		if (0 == __STREXB((val+add), mem)){
			__DMB();
			return true;
		}
	}while(true);
};
