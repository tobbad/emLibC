/*
 * cycle.h
 *
 *  Created on: 20.05.2026
 *      Author: badi
 */

#ifndef EMLIBC_COMMON_INC_CYCLE_H_
#define EMLIBC_COMMON_INC_CYCLE_H_
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SUB_SLOT_POW2 3
#define SUB_SLOT_CNT         (1 << SUB_SLOT_POW2)
#define SUB_SLOT_MASK        (SUB_SLOT_CNT - 1)
#define SYSTEM_SLOT_POW2      4
#define SYSTEM_SLOT_CNT      (1 << SYSTEM_SLOT_POW2)
#define SYSTEM_SLOT_MASK     (SYSTEM_SLOT_CNT - 1)
#define SYSTEM_SLOT_SHIFT    (SUB_SLOT_POW2)
#define ACT_SUB_SLOT(_cycle) \
    (((_cycle)->subSlot) & SUB_SLOT_MASK)

#define ACT_SLOT(_cycle) \
    (((_cycle)->subSlot >> SYSTEM_SLOT_SHIFT) & SYSTEM_SLOT_MASK)

typedef struct cycle_s {
    volatile int8_t subSlot; // actual sub slot
    uint16_t cycle;
    bool init;
} cycle_t;

em_msg cycle_init(cycle_t *cycle);
int8_t cycle_check_slot(int8_t slot);
em_msg cycle_set_slot(cycle_t *cycle, int8_t slot);
void cycle_increment(cycle_t *cycle, system_state_e *sync_state);
em_msg cycle_print(cycle_t *cycle, char *title);

#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_CYCLE_H_ */
