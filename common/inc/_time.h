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
#define TIME_MEAS_CHAR_PER_LINE 20
#define TIME_MEAS_TIME_FIELD_LEN 12
#define TIME_DEV_CNT 6
typedef int8_t  time_handle_t;

typedef enum {
	ONESHOT = 1,
	MAX_HOLD = 2,
} mode_e;

void time_init();
time_handle_t time_new(char *name);
bool time_doLoop_get();
em_msg time_set_max(time_handle_t hdl, int8_t max);
int8_t time_get_max(time_handle_t hdl);
bool   time_doLoop_get();
em_msg time_set_mode(time_handle_t hdl, mode_e mode);
mode_e time_get_mode(time_handle_t hdl);
void time_reset(time_handle_t hdl);
void time_start(time_handle_t hdl, uint8_t count, uint8_t *ptr);
void time_stop_su(time_handle_t hdl);
void time_stop(time_handle_t hdl, uint8_t *ptr);
void time_auto(time_handle_t hdl, uint8_t count, uint8_t *ptr);
void time_print(time_handle_t hdl, char *titel, bool python, bool timing);

#endif /* COMMON_INC__TIME_H_ */
