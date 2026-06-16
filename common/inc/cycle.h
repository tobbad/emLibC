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

#include "common.h"
#include "system_definitions.h"

typedef enum {
    SLAVE,
    MASTER,
} set_slot_e;

#define CYCLE_SUB_SLOT_POW2 3
#define CYCLE_SUB_SLOT_CNT (1<<CYCLE_SUB_SLOT_POW2)
#define CYCLE_SUB_SLOT_MASK (CYCLE_SUB_SLOT_CNT-1)
#define CYCLE_SLOT_POW2 4
#define CYCLE_SLOT_CNT (1<<CYCLE_SLOT_POW2)
#define CYCLE_SLOT_MASK (CYCLE_SLOT_CNT-1)
#define CYCLE_SLOT_SHIFT (CYCLE_SLOT_POW2)

typedef struct cycle_s cycle_t;
extern cycle_t cycle;

em_msg   cycle_init(cycle_t *cycle, int8_t press);
em_msg   cycle_reset(cycle_t *cycle);
char    *cycle_string(cycle_t *cycle);
int8_t   cycle_sub_sub_slot(cycle_t *cycle );
int8_t   cycle_act_slot(cycle_t *cycle);
int8_t   cycle_act_sub_slot(cycle_t *cycle);
uint16_t cycle_cycle(cycle_t *cycle);
int8_t   cycle_check_slot(int8_t slot);
em_msg   cycle_set_slot(cycle_t *cycle, int8_t slot, set_slot_e ss_type);
bool     cycle_check(cycle_t *cycle, int8_t rxSlot, uint8_t ss);
int8_t   cycle_difference(cycle_t *cycle, int8_t rxSlot);
void     cycle_increment(cycle_t *cycle, system_state_e *sync_state);
em_msg   cycle_print(cycle_t *cycle, char *title);

#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_CYCLE_H_ */
