/*
 * radio_stat.h
 * Radio statistic
 *
 *  Created on: 05.08.2026
 *      Author: tba
 */
#include "common.h"
#include "cycle.h"
#ifndef EMLIBC_COMMON_INC_radio_stat_H_
#define EMLIBC_COMMON_INC_radio_stat_H_

typedef struct radio_stat_s radio_stat_t;
#define ACTIVE_SLOT_USAGE 0

extern radio_stat_t rstat;

em_msg   radio_stat_init(radio_stat_t *stat);
em_msg   radio_stat_reset(radio_stat_t *stat);
em_msg   radio_stat_set_rssi(radio_stat_t *stat, uint8_t idx, int32_t dbm);
em_msg   radio_stat_inc_recv(radio_stat_t *stat, uint8_t idx);
uint32_t radio_stat_get_recv(radio_stat_t *stat, uint8_t idx);
em_msg   radio_stat_inc_recvd(radio_stat_t *stat, uint8_t idx);
uint32_t radio_stat_get_recvd(radio_stat_t *stat, uint8_t idx);
em_msg   radio_stat_inc_rack(radio_stat_t *stat, uint8_t idx);
uint32_t radio_stat_get_rack(radio_stat_t *stat, uint8_t idx);
em_msg   radio_stat_inc_sack(radio_stat_t *stat, uint8_t idx);
uint32_t radio_stat_get_sack(radio_stat_t *stat, uint8_t idx);
em_msg   radio_stat_set_crc_err(radio_stat_t *stat);
system_state_e radio_stat_sync(radio_stat_t *stat);
em_msg   radio_stat_print(radio_stat_t *stat);
em_msg   radio_stat_print_rssi(radio_stat_t *stat, uint8_t min, uint8_t max);

#endif /* EMLIBC_COMMON_INC_radio_stat_H_ */
