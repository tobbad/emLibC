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
#define NOW (DWT->CYCCNT * _time.ccnt2ns)


typedef struct time_meas_s {
    uint32_t tick_start;
    uint32_t tick_stop;
    uint32_t tick_duration;
    int64_t start_ns;
    int64_t stop_su_ns;
    int64_t stop_tx_ns;
    int64_t duration_su_ns;
    int64_t duration_tx_ns;
    int64_t baud;
    uint8_t count;
    char line[TIME_MEAS_CHAR_PER_LINE + 1];
} time_meas_t;

typedef struct time_single_s {
    time_meas_t measurement[TIME_MEAS_CNT];
    int8_t idx;
    uint8_t max;
    int64_t init_ns;
    int64_t last_start_ns;
    int64_t first_ns;
    int64_t last_ns; // This is set, when maximal baudrate was achived
    float max_baud;
    int32_t max_cnt;
    uint8_t mode;
    uint8_t name[LINE_CHAR];
} time_single_t;

typedef struct timem_s {
    time_single_t time[TIME_DEV_CNT];
    uint8_t used;
    uint32_t clk_Hz;
    float ccnt2ns;
    int64_t init_ns;
    bool init;
    bool doLoop; // becomes true when one cyle of times are stored and oneShot is active
} timem_t;

timem_t _time;

em_msg time_check_hdl(time_handle_t hdl) {
    em_msg res = EM_ERR;
    // clang-format off
    if ((hdl >= 0) && (hdl < TIME_DEV_CNT)) return EM_OK;
    // clang-format on

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
    _time.ccnt2ns = 1000000000 / div;
    _time.init = true;
    _time.doLoop = true;
    _time.init_ns = NOW;
}

time_handle_t time_new(char *name) {
    // clang-format off
    if (!_time.init) return -1;
    // clang-format on
    uint8_t len =strlen((char*)name);
    len = MIN(len, TIME_MEAS_CHAR_PER_LINE);
    for (uint8_t hdl = 0; hdl < TIME_DEV_CNT; hdl++) {
        if ((_time.used & (1 << hdl)) == 0) {
            _time.used |= (1 << hdl);
            time_reset(hdl);
            _time.time[hdl].init_ns = NOW;
            _time.time[hdl].max = 0;
            memcpy(_time.time[hdl].name, name, len);
            return hdl;
        }
    }
    printf("*** No more timing handles ***"NL);
    return -1;
}

em_msg time_set_mode(time_handle_t hdl, mode_e mode) {
	em_msg res = EM_ERR;
    // clang-format off
    if (time_check_hdl(hdl) == EM_ERR) return res;
    if (!_time.init) return res;
    // clang-format on
    res = EM_OK;
    _time.time[hdl].mode = mode;
    return res;
}

mode_e time_get_mode(time_handle_t hdl) {
    // clang-format off
    if (time_check_hdl(hdl) == EM_ERR) return EM_ERR;
    if (!_time.init) return EM_ERR;
    // clang-format on
    return _time.time[hdl].mode;
}


int8_t time_get_max(time_handle_t hdl){
	em_msg res = EM_ERR;
    if (time_check_hdl(hdl) == EM_ERR) return res;
	return _time.time[hdl].max;
}

em_msg time_set_max(time_handle_t hdl, int8_t max){
	em_msg res = EM_ERR;
	if ((max<TIME_MEAS_CNT-1)&&(max>=0)) {
		_time.time[hdl].max = max;
	}
	return res;

}

void time_reset(time_handle_t hdl) {
    // clang-format off
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    // clang-format on
    int64_t init_ns = _time.time[hdl].init_ns;
    memset(&_time.time[hdl].measurement, 0, sizeof(time_single_t));
    _time.time[hdl].init_ns = init_ns;
    _time.time[hdl].first_ns = -1;
    _time.time[hdl].last_ns = -1;
}

void time_start(time_handle_t hdl, uint8_t count, uint8_t *ptr) {
    // clang-format off
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    // clang-format on
    uint8_t len =strlen((char*)ptr);
    len = MIN(len, TIME_MEAS_CHAR_PER_LINE);
    int64_t now_ns = NOW;
    if (_time.time[hdl].first_ns < 0) {
        _time.time[hdl].first_ns = now_ns;
    }
    if (_time.time[hdl].idx >= 0) {
        _time.time[hdl].last_start_ns = now_ns;
        _time.time[hdl].last_ns = now_ns;
        _time.time[hdl].measurement[_time.time[hdl].idx].count = count;
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_start = HAL_GetTick();
        memcpy((uint8_t *)_time.time[hdl].measurement[_time.time[hdl].idx].line, ptr, len);
        _time.time[hdl].measurement[_time.time[hdl].idx].line[len + 1] = 0;
        _time.time[hdl].measurement[_time.time[hdl].idx].start_ns = now_ns;
        _time.time[hdl].max_cnt += count;
    }
}
void time_stop_su(time_handle_t hdl) {
    // clang-format off
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    // clang-format on
    if (!_time.init)
        return;
    if (_time.time[hdl].idx >= 0) {
        int64_t now_ns = NOW;
        _time.time[hdl].measurement[_time.time[hdl].idx].stop_su_ns = now_ns;
        _time.time[hdl].measurement[_time.time[hdl].idx].duration_su_ns =
            (_time.time[hdl].measurement[_time.time[hdl].idx].stop_su_ns -
             _time.time[hdl].measurement[_time.time[hdl].idx].start_ns);
    }
}

void time_stop(time_handle_t hdl, uint8_t *ptr) {
    // clang-format off
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    // clang-format on
    if (_time.time[hdl].idx >= 0) {
        uint8_t len =strlen((char*)ptr);
        len = MIN(len, TIME_MEAS_CHAR_PER_LINE);
        if (ptr!=NULL){
            memcpy((uint8_t *)_time.time[hdl].measurement[_time.time[hdl].idx].line, ptr, len);
            _time.time[hdl].measurement[_time.time[hdl].idx].line[len + 1] = 0;
        }
        int64_t now_ns = NOW;
        _time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_ns = now_ns;
        float max_cnt = _time.time[hdl].max_cnt;
        float baud = (_time.time[hdl].last_start_ns - _time.time[hdl].first_ns) / (max_cnt);
        if (baud > _time.time[hdl].max_baud) {
            _time.time[hdl].last_ns = _time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_ns;
            _time.time[hdl].max_baud = baud;
        }
        _time.time[hdl].measurement[_time.time[hdl].idx].duration_tx_ns =
            (_time.time[hdl].measurement[_time.time[hdl].idx].stop_tx_ns -
             _time.time[hdl].measurement[_time.time[hdl].idx].start_ns);
        _time.time[hdl].measurement[_time.time[hdl].idx].baud =
            _time.time[hdl].measurement[_time.time[hdl].idx].count * 8000000000 /
            _time.time[hdl].measurement[_time.time[hdl].idx].duration_tx_ns;
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_stop = HAL_GetTick();
        _time.time[hdl].measurement[_time.time[hdl].idx].tick_duration =
            _time.time[hdl].measurement[_time.time[hdl].idx].tick_stop -
            _time.time[hdl].measurement[_time.time[hdl].idx].tick_start;
        _time.time[hdl].idx = _time.time[hdl].max + (_time.time[hdl].idx + 1-_time.time[hdl].max) % (TIME_MEAS_CNT- _time.time[hdl].max);
        if ((_time.time[hdl].idx == 0) && (_time.time[hdl].mode & ONE_SHOT)) {
            _time.time[hdl].idx = -1;
            _time.doLoop = false;
        }
    }
}
void time_auto(time_handle_t hdl, uint8_t count, uint8_t *ptr) {
    // clang-format off
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    // clang-format on
    time_start(hdl, count, ptr);
    time_stop(hdl, NULL);
    _time.time[hdl].last_start_ns = NOW;
}

bool time_doLoop_get() { return _time.doLoop; };

void time_print(time_handle_t hdl, char *titel, bool python, bool timing) {
    // clang-format off
    if (time_check_hdl(hdl) == EM_ERR) return;
    if (!_time.init) return;
    // clang-format on
    print_e save;
    if (titel != NULL) {
        printf("%s" NL, titel);
    }
    printf("Handle %s"NL, _time.time[hdl].name);
    if (python) {
        save = serial_mode_get();
        serial_mode_set(RAW);
        printf("# duration_tick, duration_ns, count, baud " NL);
        printf("data = {\"timing\":[" NL);
    } else {
        if (timing){
            printf("// Start_text duration_tick, duration_ns, count baud " NL);
        } else {
            printf("// duration_tick, duration_ns, count baud " NL);
        }
    }
    int64_t duration_ns = 0;
    uint32_t duration_tick = 0;
    uint32_t count = 0;
    int64_t baud = 0;
    //uint32_t tick_start = 0;

    for (uint8_t i = 0; i < TIME_MEAS_CNT; i++) {
        duration_ns = _time.time[hdl].measurement[i].duration_tx_ns;
        duration_tick = _time.time[hdl].measurement[i].tick_duration;
        count = _time.time[hdl].measurement[i].count;
        baud = _time.time[hdl].measurement[i].baud;
        //tick_start = _time.time[hdl].measurement[_time.time[hdl].idx].tick_start;
        if (python) {
            printf("    [ %4ld, %9"PRId64", %3ld, %8"PRId64" ]," NL, duration_tick, duration_ns, count, baud);
        } else {
            if (timing){
                char *txt =  _time.time[hdl].measurement[i].line;
                printf("    [ %6s , %4ld, %9"PRId64", %3ld, %8"PRId64" ]," NL, txt , duration_tick, duration_ns, count, baud);
            } else {
                printf("    [ %4ld, %9"PRId64", %4ld, %8"PRId64" ]," NL , duration_tick, duration_ns, count, baud);
            }
        }
    }
    if (python) {
        uint32_t maxTxTime_ns = (_time.time[hdl].last_ns - _time.time[hdl].first_ns);
        printf("]," NL);
        printf("    \"max_cnt\"       : %ld, " NL, _time.time[hdl].max_cnt);
        printf("    \"maxTxTime_ns\" : %ld, " NL, maxTxTime_ns);
        printf("    \"max_baud\"      : %f" NL, _time.time[hdl].max_baud);
        printf("}" NL);
        serial_mode_set(save);
    } else {
        printf("]" NL);
    }
}
