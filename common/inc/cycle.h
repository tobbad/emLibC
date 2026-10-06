/*
 * cycle.h
 *
 *  Created on: 20.05.2026
 *      Author: badi
 */
#ifndef EMLIBC_COMMON_INC_CYCLE_H_
#define EMLIBC_COMMON_INC_CYCLE_H_
#ifdef __cplusplus
extern "C" {
#endif
#ifndef UNIT_TEST
#include "hal_port.h"
#else
typedef struct __TIM_HandleTypeDef {} TIM_HandleTypeDef;
// subSlot packs the position in the frame cycle: the upper nibble is the slot,
// the lower nibble the sub-slot within it (e.g. 223 = 0xDF -> slot 13, sub-slot 15).
#define CYCLE_ACT_SUB_SLOT_N(ss)   (((ss) >> CYCLE_SUB_SLOT_SHIFT) & CYCLE_SUB_SLOT_MASK)
#define CYCLE_ACT_SUB_SLOT(_cycle) (CYCLE_ACT_SUB_SLOT_N((_cycle)->subSlot))
#define CYCLE_ACT_SLOT_N(ss)       (((ss) >> CYCLE_SLOT_SHIFT) & CYCLE_SLOT_MASK)
#define CYCLE_ACT_SLOT(_cycle)     (CYCLE_ACT_SLOT_N((_cycle)->subSlot))

#endif
#include "common.h"

typedef enum {
    NOT_SET,
    SLAVE,
    MASTER,
    SS_CNT,
} dev_role_e;

#define SLOT_NOT_SET ((int8_t)-1)
#define CYCLE_SUB_SLOT_POW2 4
#define CYCLE_SUB_SLOT_CNT (1 << CYCLE_SUB_SLOT_POW2)
#define CYCLE_SUB_SLOT_MASK (CYCLE_SUB_SLOT_CNT - 1)
#define CYCLE_SUB_SLOT_SHIFT 0

#define CYCLE_SLOT_POW2 4
#define CYCLE_SLOT_CNT (1 << CYCLE_SLOT_POW2)
#define CYCLE_SLOT_MASK (CYCLE_SLOT_CNT - 1)
#define CYCLE_SLOT_SHIFT (CYCLE_SLOT_POW2)
#define CYCLE_MODULO (CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT)
// Half the ring: cycle_difference() folds its result into
// [-CYCLE_MODULO_HALF, CYCLE_MODULO_HALF).
#define CYCLE_MODULO_HALF (CYCLE_MODULO / 2)
// Out-of-band result of cycle_difference() for a NULL or uninitialised cycle.
// Outside the folded range, so it cannot collide with a valid distance.
#define CYCLE_DIFF_INVALID INT16_MIN
extern idxa2str_t synca2str;
// is set so that at least once in a KEEP_ALIVE_CYCLE_CNT Frame cycle a frame is sent
#define CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT (uint8_t)16
// is set so that at least once in a KEEP_ALIVE_CYCLE_CNT Frame cycle a frame is sent
#define CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT  (uint8_t)3
#define CYCLE_MASTER_LOOSE                                                                                                  \
    3 // After CYCLE_MASTER_LOOSE*CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT a MASTER loooses its master role and all slave set their
      // role to NOT_SET. Then when the first Packet is received the Device sending in this slot becomes the new MASTER.
// Frame cycles a role survives without proof that the network is still there.
// Kicked by cycle_master_seen(), counted down in cycle_increment().
#define CYCLE_MASTER_LOOSE_CYCLE_CNT (uint16_t)(CYCLE_MASTER_LOOSE * CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT)

#ifdef UNIT_TEST
typedef struct cycle_s {
    volatile uint8_t subSlot; // actual sub slot
    uint8_t psubSlot;         //  Pendig difference subslot value
    int8_t actSlot;
    int8_t lSlot;
    int8_t sSlot;
    uint16_t cycle;
    int8_t slot; // Configured slot of device
    int8_t master;
    bool isSlave;
    bool isMaster;
    uint16_t masterAge; // frame cycles since the network was last heard from
    uint16_t slaveAge;  // iterates over slave cycles till role is resetted
    int8_t tx_ss;
    int8_t rx_ss;
    int8_t postrx;
    dev_role_e role;
    uint16_t ssCnt;    // Counter for subslot count between cycle_sscnt_start and cycle_sscnt_stop after cycle_sscnt_init
    int8_t kaCnt;      // Set Keep alive counter
    int8_t _kaCnt;     // Keep alive counter
    uint32_t timerCNT; // MCU cycle count when cycle count was set
    bool doMeasure;
    bool cntErrror;
    system_state_e sync_state;
    bool init;
    bool set; // is set when cycle was finished
    TIM_HandleTypeDef *timer;
} cycle_t;
#else
typedef struct cycle_s cycle_t;
#endif
extern cycle_t cycle;

em_msg cycle_reset(cycle_t *cycle);
em_msg cycle_init(cycle_t *cycle, int8_t master_ss, int8_t slave_ss, uint8_t postrx, uint8_t kaCnt, TIM_HandleTypeDef *htim);
em_msg cycle_timer_add(cycle_t *cycle, int8_t add);
size_t cycle_size();
void cycle_reset_role(cycle_t *cycle);
em_msg cycle_reset_subslot(cycle_t *cycle);
char *cycle_string(cycle_t *cycle);
char *cycle_text_char(cycle_t *cycle, const char *text);
em_msg cycle_text_print(cycle_t *cycle, const char *text);
em_msg cycle_text_print_s(cycle_t *cycle, const char *text, char *str);
em_msg cycle_text_print_v(cycle_t *cycle, const char *text, void *str);
int8_t cycle_act_slot(cycle_t *cycle);
int8_t cycle_act_sub_slot(cycle_t *cycle);
uint16_t cycle_cycle(cycle_t *cycle);
em_msg cycle_update(cycle_t *cycle);
dev_role_e cycle_role(cycle_t *cycle);
char *cycle_role_str(cycle_t *cycle);
bool cycle_role_is_set(cycle_t *cycle);
em_msg cycle_dec_ka(cycle_t *cycle);
em_msg cycle_reset_ka(cycle_t *cycle);
bool cycle_is_ka(cycle_t *cycle);
em_msg cycle_doSend(cycle_t *cycle);
int8_t cycle_check_slot(int8_t slot);
em_msg cycle_set_my_slot(cycle_t *cycle, int8_t slot);
em_msg cycle_set_slot(cycle_t *cycle, int8_t slot, dev_role_e ss_type);
int8_t cycle_get_slot(cycle_t *cycle);
int8_t cycle_get_master(cycle_t *cycle);
em_msg cycle_master_seen(cycle_t *cycle, int8_t rxSlot);
em_msg cycle_set_state(cycle_t *cycle, system_state_e state);
system_state_e cycle_get_state(cycle_t *cycle);
int8_t cycle_tx_ss(cycle_t *cycle);
int8_t cycle_rx_ss(cycle_t *cycle);
uint8_t cycle_postrx(cycle_t *cycle);
int16_t cycle_difference(cycle_t *cycle, int8_t rxSlot);
void cycle_increment(cycle_t *cycle);
bool cycle_ask_set(cycle_t *cycle);
void cycle_sscnt_init(cycle_t *cycle);
void cycle_sscnt_start(cycle_t *cycle);
void cycle_sscnt_stop(cycle_t *cycle);
uint16_t cycle_sscnt_get(cycle_t *cycle);
em_msg cycle_print(cycle_t *cycle, char *title);

#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_CYCLE_H_ */
