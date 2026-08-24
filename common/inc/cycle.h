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
#endif
#include "common.h"

typedef enum {
    NOT_SET,
    SLAVE,
    MASTER,
    SS_CNT,
} dev_role_e;

#define CYCLE_SUB_SLOT_POW2  4
#define CYCLE_SUB_SLOT_CNT   (1<<CYCLE_SUB_SLOT_POW2)
#define CYCLE_SUB_SLOT_MASK  (CYCLE_SUB_SLOT_CNT-1)
#define CYCLE_SUB_SLOT_SHIFT CYCLE_SUB_SLOT_POW2

#define CYCLE_SLOT_POW2 4
#define CYCLE_SLOT_CNT (1<<CYCLE_SLOT_POW2)
#define CYCLE_SLOT_MASK (CYCLE_SLOT_CNT-1)
#define CYCLE_SLOT_SHIFT (CYCLE_SLOT_POW2)
#define CYCLE_MODULO (CYCLE_SUB_SLOT_CNT*CYCLE_SLOT_CNT)
extern idxa2str_t synca2str;
#define CYCLE_KEEP_ALIVE_CYCLE_CNT (uint16_t)8 // is set so that at least once in a KEEP_ALIVE_CYCLE_CNT Frame cycle a frame is sent
#define CYCLE_MASTER_LOOSE 3 // After CYCLE_MASTER_LOOSE*CYCLE_KEEP_ALIVE_CYCLE_CNT a MASTER loooses its slave role and all slave set their role to
                             // NOT_SET. Then when the first Packet is received the Device sending in this slot becomes the new MASTER.

#ifdef UNIT_TEST
typedef struct cycle_s {
    volatile uint8_t subSlot; // actual sub slot
    uint8_t          psubSlot;         // Pending subslot to be used on next cycle_increment
    int8_t           actSlot;
    int8_t           lSlot;
    int8_t           sSlot;
    uint16_t         cycle;
    int8_t           slot; // Configured slot of device
    int8_t           master;
    int8_t           press;
    int8_t           postss;
    int8_t           postrx;
    dev_role_e       role;
    int8_t           ssCnt;      // Counter for subslot count between cycle_sscnt_start and cycle_sscnt_stop after cycle_sscnt_init
    uint32_t         timerCNT; // MCU cycle count when cycle count was set
    bool             doMeasure;
    bool             cntErrror;
    system_state_e   sync_state;
    bool             init;
    bool             set;  // is set when cycle was finished
    TIM_HandleTypeDef *timer;
} cycle_t;
#else
typedef struct cycle_s cycle_t;
#endif
extern cycle_t cycle;


em_msg   cycle_reset(cycle_t *cycle);
em_msg   cycle_init(cycle_t *cycle, int8_t my_slot, int8_t press, int8_t postss, uint8_t postrx, TIM_HandleTypeDef *htim);
em_msg   cycle_timer_add(cycle_t *cycle, int8_t add);
size_t   cycle_size();
char    *cycle_string(cycle_t *cycle);
int8_t   cycle_act_slot(cycle_t *cycle);
dev_role_e cycle_role(cycle_t *cycle);
char *   cycle_role_str(cycle_t *cycle);
bool     cycle_role_is_set(cycle_t *cycle);
void     cycle_reset_role(cycle_t *cycle);
int8_t   cycle_act_sub_slot(cycle_t *cycle);
uint16_t cycle_cycle(cycle_t *cycle);
bool     cycle_doSend(cycle_t *cycle);
int8_t   cycle_check_slot(int8_t slot);
em_msg   cycle_set_slot(cycle_t *cycle, int8_t slot, dev_role_e ss_type);
int8_t   cycle_get_slot(cycle_t *cycle);
int8_t   cycle_get_master(cycle_t *cycle);
em_msg   cycle_set_state(cycle_t *cycle, system_state_e state);
system_state_e cycle_get_state(cycle_t *cycle);
int8_t   cycle_press(cycle_t *cycle);
int8_t   cycle_postss(cycle_t *cycle);
uint8_t  cycle_postrx(cycle_t *cycle);
uint8_t   cycle_difference(cycle_t *cycle, int8_t rxSlot);
void     cycle_increment(cycle_t *cycle);
bool     cycle_is_set(cycle_t *cycle);
void     cycle_sscnt_init(cycle_t *cycle);
void     cycle_sscnt_start(cycle_t *cycle);
void     cycle_sscnt_stop(cycle_t *cycle);
uint8_t  cycle_sscnt_get(cycle_t *cycle);
em_msg   cycle_print(cycle_t *cycle, char *title);

#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_CYCLE_H_ */
