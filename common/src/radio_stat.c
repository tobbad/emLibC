/*
 * radio_stat.c
 *
 *  Created on: 05.08.2026
 *      Author: tba
 */

#include "radio_stat.h"
#include "main.h"

radio_stat_t  rstat;

typedef struct radio_stat_s{
    uint8_t        activeSlots;
    int32_t        rssi[CYCLE_SLOT_CNT];
    int32_t        recv[CYCLE_SLOT_CNT];
    int32_t        recvd[CYCLE_SLOT_CNT];
    int32_t        recve[CYCLE_SLOT_CNT];
    int32_t        rack[CYCLE_SLOT_CNT];
    int32_t        sack[CYCLE_SLOT_CNT];
    bool           crc_err;
} radio_stat_t;

em_msg radio_stat_init(radio_stat_t *stat){
    // clang-format off
    if (!stat) return EM_ERR;
    // clang-format on
    radio_stat_reset(stat);
    return EM_OK;
};

em_msg radio_stat_reset(radio_stat_t *stat){
    // clang-format off
    if (!stat) return EM_ERR;
    // clang-format on
    memset(stat, 0, sizeof(radio_stat_t));
    return EM_OK;
}
em_msg radio_stat_set_rssi(radio_stat_t *stat,  uint8_t idx, int32_t dbm){
    // clang-format off
    if (!stat) return EM_ERR;
    // clang-format on
    stat->rssi[idx] = dbm;
    return EM_OK;
};

em_msg radio_stat_inc_recv(radio_stat_t *stat,  uint8_t idx){
    // clang-format off
    if (!stat) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    stat->recv[idx] = MIN(UINT32_MAX, stat->recv[idx] + 1);
    return EM_OK;
}
uint32_t radio_stat_get_recv(radio_stat_t *stat,  uint8_t idx){
    // clang-format off
     if (!stat) return EM_ERR;
     if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
     // clang-format on
     return stat->recv[idx];
 }

em_msg radio_stat_inc_recvd(radio_stat_t *stat,  uint8_t idx){
    // clang-format off
    if (!stat) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    stat->recvd[idx] = MIN(UINT32_MAX, stat->recvd[idx] + 1);
    return EM_OK;
};


uint32_t radio_stat_get_recd(radio_stat_t *stat,  uint8_t idx){
    // clang-format off
     if (!stat) return EM_ERR;
     if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
     // clang-format on
     return stat->recvd[idx];
 }

em_msg radio_stat_inc_rack(radio_stat_t *stat,  uint8_t idx){
    // clang-format off
    if (!stat) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    stat->rack[idx] = MIN(UINT32_MAX, stat->rack[idx] + 1);
    return EM_OK;
};

uint32_t radio_stat_get_sack(radio_stat_t *stat,  uint8_t idx){
    // clang-format off
    if (!stat) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    return stat->sack[idx];
}

em_msg radio_stat_inc_sack(radio_stat_t *stat,  uint8_t idx){
    // clang-format off
    if (!stat) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    stat->sack[idx] = MIN(UINT32_MAX, stat->sack[idx] + 1);
    return EM_OK;
};

uint32_t radio_stat_get_rack(radio_stat_t *stat,  uint8_t idx){
    // clang-format off
    if (!stat) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    return stat->rack[idx];
}

em_msg radio_stat_set_crc_err(radio_stat_t *stat){
    // clang-format off
    if (!stat) return EM_ERR;
    // clang-format on
    stat->crc_err= true;
    return EM_OK;
};

em_msg radio_stat_print(radio_stat_t *stat){
    // clang-format off
    if (!stat) return EM_ERR;
    // clang-format on
    printf(" slot   recv  reccd  sack  rack  crc_err  rssi"NL);
    for (uint8_t idx=0;idx<CYCLE_SLOT_CNT;idx++){
        printf("  %1d      %7ld     %7ld      %7ld      %7ld     %3s     %3ld"NL, idx, stat->recv[idx], stat->recvd[idx], stat->sack[idx], stat->rack[idx], stat->crc_err==0?"Yes":"No ", stat->rssi[idx]);
    }
    printf("Active slots = %d"NL, stat->activeSlots);
    return EM_OK;
}


system_state_e radio_stat_sync(radio_stat_t *stat){
    uint16_t sum=0;
    float usage=0;
    stat->activeSlots=0;
    for (uint8_t i = 0;i<CYCLE_SLOT_CNT;i++){
        uint8_t cnt = radio_stat_get_recv(&rstat, i);
        if (cnt>0) stat->activeSlots++;
            sum += cnt;
    }
    usage = sum/((float)stat->activeSlots);
    if (usage > ACTIVE_SLOT_USAGE) {
        return SYNCHRONIZE_LOCKED;
    } else{
        return SYNCHRONIZE_LOCKED;
    }
    return SYNCHRONIZE_ERROR;
}

em_msg radio_stat_print_rssi(radio_stat_t *stat, uint8_t min, uint8_t max){
    // clang-format off
    if (!stat) return EM_ERR;
    // clang-format on
    for (uint8_t ch=min;ch<=max;ch++){
        printf("Channel %2d = %ld"NL, ch, stat->rssi[ch]);
    }
    return EM_OK;
}

