/*
 * _time.c
 *
 *  Created on: Jan 22, 2025
 *      Author: badi
 */
#include "_time.h"
#include "serial.h"

#define MEAS_CNT 10
#define LINE_CHAR 10

typedef struct time_meas_s {
    uint32_t tick_start;
    uint32_t tick_stop;
    uint32_t tick_duration;
    int64_t start_us;
    int64_t stop_su_us;
    int64_t stop_tx_us;
    uint32_t duration_su_us;
    uint32_t duration_tx_us;
    uint32_t baud;
    int8_t count;
    char line[10 + 2];
} time_meas_t;

typedef struct time_single_s {
    time_meas_t measurement[MEAS_CNT];
    int8_t idx;
    uint8_t mode;
} time_single_t;

typedef struct timem_s {
    time_single_t time[TIME_DEV_CNT];
    uint8_t used;
    uint32_t clk_Hz;
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
    _time.clk_Hz = HAL_RCC_GetPCLK1Freq();
}

time_handle_t time_new() {
    time_handle_t res = EM_ERR;
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
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    _time.time[hdl].mode = mode;
}

void time_reset(time_handle_t hdl) {
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    memset(&_time.time[hdl].measurement, 0, sizeof(time_single_t));
}

void time_start(time_handle_t hdl, uint8_t count, uint8_t *ptr) {
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    if (_time.time[hdl].idx >= 0) {
        _time.time[hdl].measurement[_time.time[hdl].idx].count = count;
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_start = HAL_GetTick();
        memcpy((uint8_t *)_time.time[hdl].measurement[_time.time[hdl].idx].line, ptr, LINE_CHAR);
        _time.time[hdl].measurement[_time.time[hdl].idx].start_us = DWT->CYCCNT;
    }
}
void time_end_su(time_handle_t hdl) {
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    if (_time.time[hdl].idx >= 0) {
        _time.time[hdl].measurement[_time.time[hdl].idx].stop_su_us = DWT->CYCCNT;
        _time.time[hdl].measurement[_time.time[hdl].idx].duration_su_us =
            125 *
            (_time.time[hdl].measurement[_time.time[hdl].idx].stop_su_us -
             _time.time[hdl].measurement[_time.time[hdl].idx].start_us) /
            10000;
    }
}

void time_end_tx(time_handle_t hdl) {
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    if (_time.time[hdl].idx >= 0) {
        _time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_us = DWT->CYCCNT;
        _time.time[hdl].measurement[_time.time[hdl].idx].duration_tx_us =
            125 *
            (_time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_us -
             _time.time[hdl].measurement[_time.time[hdl].idx].start_us) /
            10000;
        assert(_time.time[hdl].measurement[_time.time[hdl].idx].duration_tx_us!=0);
        _time.time[hdl].measurement[_time.time[hdl].idx].baud =
            1000000 * 10 * _time.time[hdl].measurement[_time.time[hdl].idx].count /
            _time.time[hdl].measurement[_time.time[hdl].idx].duration_tx_us;
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_stop = HAL_GetTick();
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_duration =
            _time.time[hdl].measurement[_time.time[hdl].idx].tick_stop -
            _time.time[hdl].measurement[_time.time[hdl].idx].tick_start;
        _time.time[hdl].idx = (_time.time[hdl].idx + 1) % MEAS_CNT;
        if ((_time.time[hdl].idx == 0) && (_time.time[hdl].mode & ONE_SHOT)) {
            _time.time[hdl].idx = -1;
            // doLoop = false;
        }
    }
}

void time_print(time_handle_t hdl, char *titel) {
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    if (titel != NULL)
        printf("%s // Transfer time us, count baud" NL, titel);
    printf("data:[" NL);
    for (uint8_t i = 0; i < MEAS_CNT; i++) {
        uint32_t duration_us = _time.time[hdl].measurement[i].duration_tx_us;
        uint32_t count = _time.time[hdl].measurement[i].count;
        uint32_t baud  = _time.time[hdl].measurement[i].baud;
        printf("    [ %4lu, %4lu, %4lu ]," NL, duration_us, count, baud);
    }
    printf("]" NL);
}
