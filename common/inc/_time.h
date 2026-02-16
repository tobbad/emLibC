/*
 * _time.h
 *
 *  Created on: Jan 22, 2025
 *      Author: badi
 */

#ifndef COMMON_INC__TIME_H_
#define COMMON_INC__TIME_H_
#include "common.h"

#define TIME_MEAS_CNT 10
#define TIME_MEAS_CHAR_PER_LINE 15
#define TIME_DEV_CNT 5
typedef int8_t  time_handle_t;



void time_init();
time_handle_t time_new();
bool time_doLoop_get();
void time_set_mode(time_handle_t hdl, uint8_t mode);
void time_reset(time_handle_t hdl);
void time_start(time_handle_t hdl,uint8_t count, uint8_t *ptr);
void time_end_su(time_handle_t hdl);
void time_end_tx(time_handle_t hdl);
void time_auto(time_handle_t hdl, uint8_t count, uint8_t *ptr);
void time_print(time_handle_t hdl, char *titel, bool python);

#endif /* COMMON_INC__TIME_H_ */
