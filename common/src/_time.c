/*
 * _time.c
 *
 *  Created on: Jan 22, 2025
 *      Author: badi
 */
#include "_time.h"
#include "serial.h"
#include <inttypes.h>
#define LINE_CHAR 10

typedef struct time_meas_s {
    uint32_t tick_start;
    uint32_t tick_stop;
    uint32_t tick_duration;
    int64_t start_cyccnt;
    int64_t stop_su_cyccnt;
    int64_t stop_tx_cyccnt;
    uint32_t duration_su_cyccnt;
    uint32_t duration_tx_cyccnt;
    uint32_t baud;
    int8_t count;
    char line[TIME_MEAS_CHAR_PER_LINE + 2];
} time_meas_t;

typedef struct time_single_s {
    time_meas_t measurement[TIME_MEAS_CNT];
    int8_t idx;
    int64_t last_start_cyccnt;
    uint8_t mode;
} time_single_t;

typedef struct timem_s {
    time_single_t time[TIME_DEV_CNT];
    uint8_t used;
    uint32_t clk_Hz;
    float cyccnt2us;
    bool init;
    bool doLoop;// becomes true when one cyle of times are stored and oneShot is active

} timem_t;

timem_t _time;

em_msg time_check_hdl(time_handle_t hdl) {
    em_msg res = EM_ERR;
    if ((hdl >= 0) | (hdl < TIME_DEV_CNT))
        return EM_OK;
    return res;
}

void time_init() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    memset(&_time, 0, sizeof(timem_t));
    for (uint8_t hdl = 0; hdl < TIME_DEV_CNT; hdl++) {
        time_reset(hdl);
    }
    _time.clk_Hz = HAL_RCC_GetHCLKFreq();
    float div = 10000000.0/_time.clk_Hz;
    _time.cyccnt2us = div;
    _time.init = true;
    _time.doLoop = true;
}

time_handle_t time_new() {
    time_handle_t res = EM_ERR;
    if (!_time.init) return res;
    for (uint8_t hdl = 0; hdl < TIME_DEV_CNT; hdl++) {
        if ((_time.used & (1 << hdl)) == 0) {
            _time.used |= (1 << hdl);
            time_reset(hdl);
            return hdl;
        }
    }
    return res;
}

void time_set_mode(time_handle_t hdl, uint8_t mode) {
    if (time_check_hdl(hdl) == EM_ERR)  return;
    _time.time[hdl].mode = mode;
}

void time_reset(time_handle_t hdl) {
    if (time_check_hdl(hdl) == EM_ERR) return;
    memset(&_time.time[hdl].measurement, 0, sizeof(time_single_t));
}

void time_start(time_handle_t hdl, uint8_t count, uint8_t *ptr) {
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    _time.time[hdl].last_start_cyccnt=DWT->CYCCNT;
    if (_time.time[hdl].idx >= 0) {
        _time.time[hdl].measurement[_time.time[hdl].idx].count = count;
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_start = HAL_GetTick();
        memcpy((uint8_t *)_time.time[hdl].measurement[_time.time[hdl].idx].line, ptr, TIME_MEAS_CHAR_PER_LINE);
        _time.time[hdl].measurement[_time.time[hdl].idx].line[TIME_MEAS_CHAR_PER_LINE+1]=0;
        _time.time[hdl].measurement[_time.time[hdl].idx].start_cyccnt = _time.time[hdl].last_start_cyccnt;
    }
}
void time_end_su(time_handle_t hdl) {
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    if (!_time.init) return;
    _time.time[hdl].last_start_cyccnt=DWT->CYCCNT;
    if (_time.time[hdl].idx >= 0) {
        _time.time[hdl].measurement[_time.time[hdl].idx].stop_su_cyccnt = _time.time[hdl].last_start_cyccnt;
        _time.time[hdl].measurement[_time.time[hdl].idx].duration_su_cyccnt =
            (_time.time[hdl].measurement[_time.time[hdl].idx].stop_su_cyccnt -
             _time.time[hdl].measurement[_time.time[hdl].idx].start_cyccnt);
    }
}

void time_end_tx(time_handle_t hdl) {
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    if (!_time.init) return;
    _time.time[hdl].last_start_cyccnt=DWT->CYCCNT;
    if (_time.time[hdl].idx >= 0) {
        _time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_cyccnt = _time.time[hdl].last_start_cyccnt;
        _time.time[hdl].measurement[_time.time[hdl].idx].duration_tx_cyccnt =
            (_time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_cyccnt -
             _time.time[hdl].measurement[_time.time[hdl].idx].start_cyccnt);
        _time.time[hdl].measurement[_time.time[hdl].idx].baud =
            1000000 * 10 * _time.time[hdl].measurement[_time.time[hdl].idx].count /
            _time.time[hdl].measurement[_time.time[hdl].idx].duration_tx_cyccnt;
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_stop = HAL_GetTick();
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_duration =
            _time.time[hdl].measurement[_time.time[hdl].idx].tick_stop -
            _time.time[hdl].measurement[_time.time[hdl].idx].tick_start;
        _time.time[hdl].idx = (_time.time[hdl].idx + 1) % TIME_MEAS_CNT;
        if ((_time.time[hdl].idx == 0) && (_time.time[hdl].mode & ONE_SHOT)) {
            _time.time[hdl].idx = -1;
            _time.doLoop = false;
        }
    }
}
void time_auto(time_handle_t hdl, uint8_t count, uint8_t *ptr) {
    if (time_check_hdl(hdl) == EM_ERR)  return;
    if (!_time.init) return;
    time_start(hdl, count, ptr);
    time_end_tx(hdl);
   _time.time[hdl].last_start_cyccnt=DWT->CYCCNT;
}

bool time_doLoop_get(){
    return _time.doLoop;
};

void time_print(time_handle_t hdl, char *titel, bool python) {
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    print_e save = serial_mode_get();
    if (titel != NULL){
        printf("%s", titel);
    }
    if (python){
        save = serial_mode_get();
        serial_mode_set(RAW);
        printf("# start_us duration_us count tick " NL);
        printf("data = dict(\"timing\":[" NL);
    } else {
        printf("%s // Transfer time us, count baud " NL, titel);
    }

    for (uint8_t i = 0; i < TIME_MEAS_CNT; i++) {
        uint64_t start_us    = _time.time[hdl].measurement[i].start_cyccnt*_time.cyccnt2us;
        uint32_t duration_us = _time.time[hdl].measurement[i].duration_tx_cyccnt*_time.cyccnt2us;
        uint32_t count = _time.time[hdl].measurement[i].count;
        uint32_t baud  = _time.time[hdl].measurement[i].baud;
        uint32_t tick_start  = _time.time[hdl].measurement[_time.time[hdl].idx].tick_start;
        if (python){
            printf("    [ %" PRId64 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 " ]," NL, start_us, duration_us, count, tick_start);
        }else{
            printf("    [ %" PRIu32 ", %" PRIu32 ", %" PRIu32 " ]," NL, duration_us, baud, count);
        }
    }
    if (python){
        printf("])" NL);
        serial_mode_set(save);
    } else {
        printf("]" NL);
    }
}
