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
#include "system_definitions.h"

typedef enum {
    NOT_SET,
    SLAVE,
    MASTER,
    SS_CNT,
} set_slot_e;

#define CYCLE_SUB_SLOT_POW2 3
#define CYCLE_SUB_SLOT_CNT   (1<<CYCLE_SUB_SLOT_POW2)
#define CYCLE_SUB_SLOT_MASK  (CYCLE_SUB_SLOT_CNT-1)
#define CYCLE_SUB_SLOT_SHIFT CYCLE_SUB_SLOT_POW2

#define CYCLE_SLOT_POW2 4
#define CYCLE_SLOT_CNT (1<<CYCLE_SLOT_POW2)
#define CYCLE_SLOT_MASK (CYCLE_SLOT_CNT-1)
#define CYCLE_SLOT_SHIFT (CYCLE_SLOT_POW2)
#define CYCLE_MODULO (CYCLE_SUB_SLOT_CNT*CYCLE_SLOT_CNT)
extern idxa2str_t synca2str;
#ifdef UNIT_TEST
typedef struct cycle_s {
    volatile int8_t   subSlot; // actual sub slot
    int8_t            actSlot;
    int8_t            lSlot;
    int8_t            sSlot;
    uint16_t          cycle;
    int8_t            slot;
    int8_t            press;
    int8_t            postss;
    set_slot_e        role;
    int8_t            ssCnt;
    bool              doMeasure;
    bool              cntErrror;
    system_state_e    *sync_state;
    bool              init;
    TIM_HandleTypeDef *timer;
} cycle_t;
#else
typedef struct cycle_s cycle_t;
#endif
extern cycle_t cycle;

em_msg   cycle_init(cycle_t *cycle, int8_t my_slot, int8_t press, int8_t postss, system_state_e *sync_state, TIM_HandleTypeDef *htim);
em_msg   cycle_reset(cycle_t *cycle);
em_msg   cycle_timer_add(cycle_t *cycle, int8_t add);
size_t   cycle_size();
char    *cycle_string(cycle_t *cycle);
int8_t   cycle_act_slot(cycle_t *cycle);
char *   cycle_role(cycle_t *cycle);
int8_t   cycle_act_sub_slot(cycle_t *cycle);
uint16_t cycle_cycle(cycle_t *cycle);
bool     cycle_doSend(cycle_t *cycle);
int8_t   cycle_check_slot(int8_t slot);
em_msg   cycle_set_slot(cycle_t *cycle, int8_t slot, set_slot_e ss_type);
system_state_e   cycle_state(cycle_t *cycle);
bool     cycle_isOk(cycle_t *cycle, int8_t rxSlot);
int8_t   cycle_press(cycle_t *cycle);
int8_t   cycle_postss(cycle_t *cycle);
int8_t   cycle_difference(cycle_t *cycle, int8_t rxSlot);
void     cycle_increment(cycle_t *cycle);
void     cycle_sscnt_init(cycle_t *cycle);
void     cycle_sscnt_start(cycle_t *cycle);
void     cycle_sscnt_stop(cycle_t *cycle);
uint8_t  cycle_sscnt_get(cycle_t *cycle);
em_msg   cycle_print(cycle_t *cycle, char *title);

#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_CYCLE_H_ */
