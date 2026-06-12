/*
 * cycle.c
 *
 *  Created on: 20.05.2026
 *      Author: badi
 */

#include "cycle.h"
#ifndef UNIT_TEST
#include "rb_system.h"
#include "stateled.h"
#endif

#define SLOT_PRINT_FMT "(c:%5d, %1X, %2d)" // length is 19
#define STRLEN 22
em_msg cycle_init(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    cycle->init         = true;
    cycle_reset(cycle);
    res = EM_OK;
    return res;
};


em_msg cycle_reset(cycle_t *cycle){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    cycle->subSlot      = 0;
    cycle->cycle        = 0;
    res = EM_OK;
    return res;
};

char * cycle_string(cycle_t *cycle){
    // clang-format off
    if (!cycle) return NULL;
    // clang-format on
    static char rStr[STRLEN];
    snprintf(rStr, STRLEN, SLOT_PRINT_FMT, cycle->cycle , ACT_SLOT(cycle), ACT_SUB_SLOT(cycle));
    return rStr;
}

int8_t cycle_check_slot(int8_t slot){
    if (((slot > 0) && (slot <= SLOT_CNT))&&(slot%2==1)){
        return slot;
    }
    return -1;
}

em_msg cycle_set_slot(cycle_t *cycle, int8_t slot){
     em_msg res = EM_ERR;
    // clang-format off
     if (!cycle) return res;
    // clang-format on
    if ( cycle_check_slot(slot) >= 0 ) {
        cycle_reset(cycle);
        cycle->subSlot = slot * SUB_SLOT_CNT;
        res = EM_OK;
    } else {
        res = EM_ERR;
        printf("Set invalid slot %d" NL, slot);
    }
    return res;
};

bool cycle_check(cycle_t *cycle, int8_t rxSlot, uint8_t ss){
    bool res = false;
   // clang-format off
    if (!cycle) return res;
    if (cycle_check_slot(rxSlot)<0) return res;
   // clang-format on
   res =  (cycle->sSlot==rxSlot);
   return res;
}

void cycle_increment(cycle_t *cycle, system_state_e *sync_state) {
    // clang-format off
    if (!cycle) return;
    if (!cycle->init) return;
    if (!sync_state) return;
    // clang-format on
    static uint8_t lastActSlot;
    static bool is_set = false;
    static uint8_t cycle_once = false;
    if (*sync_state == SYNCHRONIZE) {
        *sync_state = SYNCHRONIZE_READY;
        is_set = true;
        return;
    }
    if (is_set) {
        cycle->subSlot++;
        cycle->subSlot = (cycle->subSlot%(SUB_SLOT_CNT*SLOT_CNT));
        cycle->actSlot = ACT_SLOT(cycle);
        cycle->sSlot = ACT_SUB_SLOT(cycle);
#if OPTION_SHOW_TIMING == 1
        em_msg res = stateled_toggle_pin(led_3);
#endif
    }
    if ((*sync_state == SYNCHRONIZE_DOING) || (*sync_state == SYNCHRONIZE_READY) || (*sync_state == SYNCHRONIZE_ERROR)) {
        if (cycle->actSlot != lastActSlot) {
            cycle_once = false;
#if OPTION_SHOW_TIMING == 1
            stateled_set(cycle->actSlot);
            stateled_toggle_pin(led_4);
#endif
            lastActSlot = cycle->actSlot;
            if (((cycle->actSlot == 0) && (is_set) && (!cycle_once))) {
#if OPTION_SHOW_TIMING == 1
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
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    if ((title != NULL)) {
        printf("%s" NL, title);
    }
    printf("subSlot    = %d" NL, cycle->subSlot);
    printf("actSlot    = %x" NL, ACT_SLOT(cycle));
    printf("actSubSlot = %d" NL, ACT_SUB_SLOT(cycle));
    printf("cycle      = %d" NL, cycle->cycle);
    return EM_OK;
}
