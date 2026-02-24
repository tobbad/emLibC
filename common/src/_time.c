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
    int64_t start_ns;
    int64_t stop_su_ns;
    int64_t stop_tx_ns;
    uint32_t duration_su_ns;
    uint32_t duration_tx_ns;
    int64_t baud;
    uint8_t count;
    char line[TIME_MEAS_CHAR_PER_LINE + 2];
} time_meas_t;

typedef struct time_single_s {
    time_meas_t measurement[TIME_MEAS_CNT];
    int8_t idx;
    int64_t last_start_ns;
    int64_t first_ns;
    int64_t last_ns; // This is set, when maximal baudrate was achived
    float max_baud;
    int32_t max_cnt;
    uint8_t  mode;
} time_single_t;

typedef struct timem_s {
    time_single_t time[TIME_DEV_CNT];
    uint8_t used;
    uint32_t clk_Hz;
    float ccnt2ns;
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
    float div = _time.clk_Hz;
    _time.ccnt2ns = 1000000000/div;
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
    _time.time[hdl].first_ns =-1;
    _time.time[hdl].last_ns =-1;
    _time.time[hdl].max_cnt     =0;
    _time.time[hdl].max_baud    =0;
}

void time_start(time_handle_t hdl, uint8_t count, uint8_t *ptr) {
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    int64_t now_ns = DWT->CYCCNT*_time.ccnt2ns;
    if (_time.time[hdl].first_ns<0){
        _time.time[hdl].first_ns = now_ns;
    }
    if (_time.time[hdl].idx >= 0) {
        _time.time[hdl].last_start_ns = now_ns;
        _time.time[hdl].measurement[_time.time[hdl].idx].count += count;
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_start = HAL_GetTick();
        memcpy((uint8_t *)_time.time[hdl].measurement[_time.time[hdl].idx].line, ptr, TIME_MEAS_CHAR_PER_LINE);
        _time.time[hdl].measurement[_time.time[hdl].idx].line[TIME_MEAS_CHAR_PER_LINE+1]=0;
        _time.time[hdl].measurement[_time.time[hdl].idx].start_ns = now_ns;
        _time.time[hdl].max_cnt     += count;

    }
}
void time_end_su(time_handle_t hdl) {
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    if (_time.time[hdl].idx >= 0) {
        int64_t now_ns = DWT->CYCCNT*_time.ccnt2ns;
        _time.time[hdl].measurement[_time.time[hdl].idx].stop_su_ns = now_ns;
        _time.time[hdl].measurement[_time.time[hdl].idx].duration_su_ns =
            (_time.time[hdl].measurement[_time.time[hdl].idx].stop_su_ns -
             _time.time[hdl].measurement[_time.time[hdl].idx].start_ns);
    }
}

void time_end_tx(time_handle_t hdl) {
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    if (_time.time[hdl].idx >= 0) {
        int64_t now_ns = DWT->CYCCNT*_time.ccnt2ns;
        _time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_ns = now_ns;
        float max_cnt =_time.time[hdl].max_cnt;
        float baud = (_time.time[hdl].last_start_ns - _time.time[hdl].first_ns)/(max_cnt);
        if (baud > _time.time[hdl].max_baud){
            _time.time[hdl].last_ns =  _time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_ns;
            _time.time[hdl].max_baud = baud;
         }
        _time.time[hdl].measurement[_time.time[hdl].idx].duration_tx_ns =
            (_time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_ns -
             _time.time[hdl].measurement[_time.time[hdl].idx].start_ns);
        _time.time[hdl].measurement[_time.time[hdl].idx].baud =
        _time.time[hdl].measurement[_time.time[hdl].idx].count*8000000000/
            _time.time[hdl].measurement[_time.time[hdl].idx].duration_tx_ns;
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
   _time.time[hdl].last_start_ns=DWT->CYCCNT*_time.ccnt2ns;
}

bool time_doLoop_get(){
    return _time.doLoop;
};

void time_print(time_handle_t hdl, char *titel, bool python) {
    if (time_check_hdl(hdl) == EM_ERR)
        return;
    print_e save = serial_mode_get();
    if (titel != NULL){
        printf("%s"NL, titel);
    }
    if (python){
        save = serial_mode_get();
        serial_mode_set(RAW);
        printf("# tick_start, duration_ns, duration_tick, count, baud " NL);
        printf("data = {\"timing\":[" NL);
    } else {
        printf("// duration_tick, count baud " NL);
    }
    uint64_t start_ns;
    uint32_t duration_ns;
    uint32_t duration_tick;
    uint32_t count;
    uint32_t baud;
    uint32_t tick_start;

    for (uint8_t i = 0; i < TIME_MEAS_CNT; i++) {
        start_ns    = _time.time[hdl].measurement[i].start_ns;
        duration_ns = _time.time[hdl].measurement[i].duration_tx_ns;
        duration_tick = _time.time[hdl].measurement[i].tick_duration;
        count = _time.time[hdl].measurement[i].count;
        baud  = _time.time[hdl].measurement[i].baud;
        tick_start  = _time.time[hdl].measurement[_time.time[hdl].idx].tick_start;
        if (python){
            printf("    [ %4ld, %8ld, %1ld, %3ld, %6ld ]," NL, tick_start, duration_ns, duration_tick, count, baud);
        }else{
            printf("    [ %3ld, %6ld, %3ld ]," NL, duration_tick, baud, count);
        }
    }
    if (python){
        uint64_t maxTxTime_ns    = (_time.time[hdl].last_ns -_time.time[hdl].first_ns );
        printf("]," NL);
        printf("    \"maxcnt\"       : %ld, "NL, _time.time[hdl].max_cnt);
        printf("    \"maxTxTime_ns\" : %ld, "NL,  maxTxTime_ns);
        printf("    \"maxbaud\"      : %f" NL,  _time.time[hdl].max_baud);
        printf("}" NL);
        serial_mode_set(save);
    } else {
        printf("]" NL);
    }
}
