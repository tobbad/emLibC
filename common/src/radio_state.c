/*
 * radio_state.c
 *
 *  Created on: 05.08.2026
 *      Author: tba
 */

#include "radio_state.h"


radio_state_t  rstate;

typedef struct radio_state_s{
    system_state_e *sync_state;
    uint8_t        activeSlots;
    int32_t        rssi[CYCLE_SLOT_CNT];
    uint8_t        recv[CYCLE_SLOT_CNT];
    uint8_t        recvd[CYCLE_SLOT_CNT];
    uint8_t        recve[CYCLE_SLOT_CNT];
    uint8_t        ack[CYCLE_SLOT_CNT];
    bool           crc_err;
} radio_state_t;

em_msg radio_state_init(radio_state_t *state,  system_state_e *sync_state){
    // clang-format off
    if (!state) return EM_ERR;
    if (*sync_state>=SYNC_CNT) return EM_ERR;
    // clang-format on
    radio_state_reset(state);
    state->sync_state = sync_state;
    return EM_OK;
};

em_msg radio_state_reset(radio_state_t *state){
    // clang-format off
    if (!state) return EM_ERR;
    // clang-format on
    memset(state, 0, sizeof(radio_state_t));
    return EM_OK;
}
em_msg radio_state_set_rssi(radio_state_t *state,  uint8_t idx, int32_t dbm){
    // clang-format off
    if (!state) return EM_ERR;
    // clang-format on
    state->rssi[idx] = dbm;
    return EM_OK;
};
em_msg radio_state_inc_recv(radio_state_t *state,  uint8_t idx){
    // clang-format off
    if (!state) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    state->recv[idx] = MIN(255, state->recv[idx] + 1);
    return EM_OK;
}

em_msg radio_state_inc_recvd(radio_state_t *state,  uint8_t idx){
    // clang-format off
    if (!state) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    state->recvd[idx] = MIN(255, state->recvd[idx] + 1);
    return EM_OK;
};

em_msg radio_state_inc_recve(radio_state_t *state,  uint8_t idx){
    // clang-format off
    if (!state) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    state->recve[idx] = MIN(255, state->recve[idx] + 1);
    return EM_OK;
};

uint8_t radio_state_get_recv(radio_state_t *state,  uint8_t idx){
    // clang-format off
    if (!state) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    return state->recv[idx];
}

em_msg radio_state_inc_ack(radio_state_t *state,  uint8_t idx){
    // clang-format off
    if (!state) return EM_ERR;
    if (idx>=CYCLE_SLOT_CNT) return EM_ERR;
    // clang-format on
    state->ack[idx] = MIN(255, state->ack[idx] + 1);
    return EM_OK;
};

em_msg radio_state_set_crc_err(radio_state_t *state){
    // clang-format off
    if (!state) return EM_ERR;
    // clang-format on
    state->crc_err= true;
    return EM_OK;
};

em_msg radio_state_print(radio_state_t *state){
    // clang-format off
    if (!state) return EM_ERR;
    // clang-format on
    printf(" slot   recv  recvd  recve  crc_err  rssi"NL);
    for (uint8_t idx=0;idx<CYCLE_SLOT_CNT;idx++){
        printf(" %2d     %2d     %2d     %2d     %3s     %3ld"NL, idx, state->recv[idx], state->recvd[idx], state->recve[idx], state->crc_err==0?"Yes":"No ", state->rssi[idx]);
    }
    printf("Active slots = %d"NL, state->activeSlots);
    return EM_OK;
}


em_msg radio_state_set_sync_state(radio_state_t *state,  system_state_e sync_state){
    // clang-format off
    if (!state) return EM_ERR;
    // clang-format on
    *state->sync_state =sync_state;
    return EM_OK;
};

system_state_e radio_state_get_sync_state(radio_state_t *state){
    // clang-format off
    if (!state) return EM_ERR;
    // clang-format on
    return *state->sync_state;
};

system_state_e radio_state_sync(radio_state_t *state){
    uint16_t sum=0;
    float usage=0;
    state->activeSlots=0;
    for (uint8_t i = 0;i<CYCLE_SLOT_CNT;i++){
        uint8_t cnt = radio_state_get_recv(&rstate, i);
        if (cnt>0) state->activeSlots++;
            sum += cnt;
    }
    usage = sum/((float)state->activeSlots);
    if (usage > ACTIVE_SLOT_USAGE) {
        return SYNCHRONIZE_LOCKED;
    } else{
        return SYNCHRONIZE_LOCKED;
    }
    return SYNCHRONIZE_ERROR;
}

em_msg radio_state_print_rssi(radio_state_t *state, uint8_t min, uint8_t max){
    // clang-format off
    if (!state) return EM_ERR;
    // clang-format on
    for (uint8_t ch=min;ch<=max;ch++){
        printf("Channel %2d = %ld"NL, ch, state->rssi[ch]);
    }
}

