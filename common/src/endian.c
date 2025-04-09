/*
 * endian.c
 *
 *  Created on: Mar 6, 2025
 *      Author: TBA
 */
#include "common.h"
uint32_t swap(uint32_t val){
    uint32_t out=0;
    uint8_t * in =(uint8_t*)&val;
    out  = (in[0]<<24)|(in[1]<<16)|(in[2]<<8)|(in[0]);
    return out;
};

