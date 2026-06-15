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

#define ACT_SUB_SLOT(_cycle) (((_cycle)->subSlot) & SUB_SLOT_MASK)

#define ACT_SLOT(_cycle) (((_cycle)->subSlot >> SLOT_SHIFT) & SLOT_MASK)

#define CYCLE_TO_LINEAR(cycle)

typedef struct cycle_s {
    volatile int8_t subSlot; // actual sub slot
    int8_t actSlot;
    int8_t sSlot;
    uint16_t cycle;
    bool init;
} cycle_t;

em_msg cycle_init(cycle_t *cycle);
em_msg cycle_reset(cycle_t *cycle);
char *cycle_string(cycle_t *cycle);
int8_t cycle_check_slot(int8_t slot);
em_msg cycle_set_slot(cycle_t *cycle, int8_t slot);
bool cycle_check(cycle_t *cycle, int8_t rxSlot, uint8_t ss);
int8_t cycle_difference(cycle_t *cycle, int8_t rxSlot);
void cycle_increment(cycle_t *cycle, system_state_e *sync_state);
em_msg cycle_print(cycle_t *cycle, char *title);

#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_CYCLE_H_ */
