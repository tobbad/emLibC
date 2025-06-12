/*
 * _time.h
 *
 *  Created on: Jan 22, 2025
 *      Author: badi
 */

#ifndef COMMON_INC__TIME_H_
#define COMMON_INC__TIME_H_
#include "serial.h"

void time_init();
void time_set_mode(print_e mode);
void time_reset();
void time_start(uint8_t count, uint8_t *ptr);
void time_end_su();
void time_end_tx();
void time_print(char * titel);

#endif /* COMMON_INC__TIME_H_ */
