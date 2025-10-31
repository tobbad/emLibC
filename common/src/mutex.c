/*
 * mutex.c
 *
 *  Created on: Jan 11, 2025
 *      Author: badi
 */
#include "mutex.h"
#include "cmsis_gcc.h"
#include "common.h"
bool ReadModify_write(int8_t *mem, int8_t add) {
    do {
        uint8_t val = __LDREXB((uint8_t *)mem);
        if (0 == __STREXB((val + add), (uint8_t *)mem)) {
            __DMB();
            return true;
        }
    } while (true);
};
