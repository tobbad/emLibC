/*
 * _time.c
 *
 *  Created on: Jan 22, 2025
 *      Author: badi
 */
#include "_time.h"
#define MEAS_CNT 10
#define LINE_CHAR 10

typedef struct time_meas_s {
  int64_t tick_start;
  int64_t tick_stop;
  int64_t tick_duration;
  int64_t start;
  int64_t stop_su;
  int64_t stop_tx;
  uint64_t duration_su_us;
  uint64_t duration_tx_us;
  uint32_t baud;
  int8_t count;
  char line[10 + 2];
} time_meas_t;

typedef struct timem_s {
  time_meas_t time[MEAS_CNT];
  int8_t idx;
  uint32_t clk_Hz;
  uint8_t mode;
} timem_t;

timem_t _time;

void time_init() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  time_reset();
  _time.clk_Hz = HAL_RCC_GetPCLK1Freq();
}

void time_set_mode(print_e mode) { _time.mode = mode; }

void time_reset() { memset(&_time, 0, sizeof(timem_t)); }

void time_start(uint8_t count, uint8_t *ptr) {
  if (_time.idx >= 0) {
    _time.time[_time.idx].count = count;
    _time.time[_time.idx].tick_start = HAL_GetTick();
    memcpy((uint8_t *)_time.time[_time.idx].line, ptr, LINE_CHAR);
    _time.time[_time.idx].start = DWT->CYCCNT;
  }
}
void time_end_su() {
  if (_time.idx >= 0) {
    _time.time[_time.idx].stop_su = DWT->CYCCNT;
    _time.time[_time.idx].duration_su_us =
        125 * (_time.time[_time.idx].stop_su - _time.time[_time.idx].start) /
        10000;
  }
}

void time_end_tx() {
  if (_time.idx >= 0) {
    _time.time[_time.idx].stop_tx = DWT->CYCCNT;
    _time.time[_time.idx].duration_tx_us =
        125 * (_time.time[_time.idx].stop_tx - _time.time[_time.idx].start) /
        10000;
    _time.time[_time.idx].baud = 1000000 * 10 * _time.time[_time.idx].count /
                                 _time.time[_time.idx].duration_tx_us;
    _time.time[_time.idx].tick_stop = HAL_GetTick();
    _time.time[_time.idx].tick_duration =
        _time.time[_time.idx].tick_stop - _time.time[_time.idx].tick_start;
    _time.idx = (_time.idx + 1) % MEAS_CNT;
    if ((_time.idx == 0) && (_time.mode & ONE_SHOT)) {
      _time.idx = -1;
      // doLoop = false;
    }
  }
}

void time_print(char *titel) {
  if (titel != NULL)
    printf("%s" NL, titel);
  printf("data:[" NL);
  for (uint8_t i = 0; i < MEAS_CNT; i++) {
    printf("[ %llu, %u, %ld ]," NL, _time.time[_time.idx].duration_tx_us,
           _time.time[_time.idx].count, _time.time[_time.idx].baud);
  }
  printf("]" NL);
}
