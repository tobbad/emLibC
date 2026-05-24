/*
 * cycle.c
 *
 *  Created on: 20.05.2026
 *      Author: badi
 */

#include "cycle.h"
#include "common.h"
#ifndef UNIT_TEST
#include "stateled.h"
#endif

em_msg cycle_init(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (cycle->init) return EM_OK;
    cycle->subSlot      = 0;
    cycle->cycle        = 0;
    cycle->init         = true;
    res = EM_OK;
    return res;
};

int8_t cycle_check_slot(int8_t slot){
    if (((slot > 0) && (slot <= SYSTEM_SLOT_CNT))&&(slot%2==1)){
        return slot;
    }
    return -1;
}

em_msg cycle_set_slot(cycle_t *cycle, int8_t slot){
     em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    // clang-format on
    if ((cycle_check_slot(slot) > 0)) {
        cycle_init(cycle);
        cycle->subSlot = slot * SUB_SLOT_CNT;
        res = EM_OK;
    } else {
        res = EM_ERR;
        printf("Set invalid slot %d" NL, slot);
    }
    return res;
};

void cycle_increment(cycle_t *cycle, system_state_e *sync_state) {
    // clang-format off
    if (!cycle) return;
    // clang-format on
    static uint8_t lastActSubSlot;
    static bool is_set = false;
    static uint8_t cycle_once = false;
    uint8_t actSubSlot;
    uint8_t actSlot;
    if (*sync_state == SYNCHRONIZE) {
        *sync_state = SYNCHRONIZE_READY;
        is_set = true;
        return;
    }
    if (is_set) {
        cycle->subSlot++;
#if OPTION_SHOW_TIMING == 1
        em_msg res = stateled_toggle_pin(led_3);
#endif
        actSubSlot = ACT_SUB_SLOT(cycle->subSlot);
    }
    if ((*sync_state == SYNCHRONIZE_DOING) || (*sync_state == SYNCHRONIZE_READY) || (*sync_state == SYNCHRONIZE_ERROR)) {
        actSlot = ACT_SLOT(cycle->subSlot);

        if (actSlot != lastActSubSlot) {
            cycle_once = false;
#if OPTION_SHOW_TIMING == 1
            stateled_set(actSlot);
            stateled_toggle_pin(led_4);
#endif
            lastActSubSlot = actSlot;
            // stateled_set(rb_system.actSlot);
            if (((actSlot == 0) && (is_set) && (!cycle_once))) {
#if OPTION_SHOW_TIMING == 1
                rb_system.subSlot = 0;
                em_msg res = stateled_toggle_pin(led_5);
#endif
                cycle_once = true;
                cycle->cycle += 1;
            }
        }
    }
}

em_msg cycle_print(cycle_t *cycle, char *title) {
    em_msg res = EM_ERR;
    if (!cycle)
        return res;
    if ((title != NULL)) {
        printf("%s" NL, title);
    }
    printf("subSlot    = %d" NL, cycle->subSlot);
    printf("actSlot    = %d" NL, ACT_SLOT(cycle->subSlot));
    printf("actSubSlot = %d" NL, ACT_SUB_SLOT(cycle->subSlot));
    printf("cycle      = %d" NL, cycle->cycle);
    return EM_OK;
}
