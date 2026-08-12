/*
 * radio_state.h
 *
 *  Created on: 05.08.2026
 *      Author: tba
 */
#include "common.h"
#include "cycle.h"
#ifndef EMLIBC_COMMON_INC_RADIO_STATE_H_
#define EMLIBC_COMMON_INC_RADIO_STATE_H_

typedef struct radio_state_s radio_state_t;
#define ACTIVE_SLOT_USAGE 0

extern radio_state_t rstate;

em_msg radio_state_init(radio_state_t *state);
em_msg radio_state_reset(radio_state_t *state);
em_msg radio_state_set_rssi(radio_state_t *state, uint8_t idx, int32_t dbm);
em_msg radio_state_inc_recv(radio_state_t *state, uint8_t idx);
uint8_t radio_state_get_recv(radio_state_t *state, uint8_t idx);
em_msg radio_state_inc_recvd(radio_state_t *state, uint8_t idx);
em_msg radio_state_inc_recve(radio_state_t *state, uint8_t idx);
em_msg radio_state_inc_ack(radio_state_t *state, uint8_t idx);
em_msg radio_state_set_crc_err(radio_state_t *state);
system_state_e radio_state_sync(radio_state_t *state);
em_msg radio_state_print(radio_state_t *state);
em_msg radio_state_print_rssi(radio_state_t *state, uint8_t min, uint8_t max);

#endif /* EMLIBC_COMMON_INC_RADIO_STATE_H_ */
