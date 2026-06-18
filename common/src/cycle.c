/*
 * cycle->c
 *
 *  Created on: 20.05.2026
 *      Author: badi
 */

#include "cycle.h"
#ifndef UNIT_TEST
#include "stateled.h"
#include "options.h"
#endif


typedef struct cycle_s {
    volatile int8_t subSlot; // actual sub slot
    int8_t    actSlot;
    int8_t    lSlot;
    int8_t    sSlot;
    uint16_t  cycle;
    int8_t    press;
    system_state_e *sync_state;
    bool      init;
} cycle_t;

#define CYCLE_ACT_SUB_SLOT(_cycle) (((_cycle)->subSlot) & CYCLE_SUB_SLOT_MASK)

#define CYCLE_ACT_SLOT(_cycle) (((_cycle)->subSlot >> CYCLE_SUB_SLOT_SHIFT) & CYCLE_SLOT_MASK)

cycle_t cycle;

#define SLOT_PRINT_FMT "(c:%5d, %1X, %2d)" // length is 19
#define STRLEN 22

em_msg cycle_init(cycle_t *cycle, int8_t press , system_state_e *sync_state) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!sync_state) return res;
    // clang-format on
    cycle->press= press;
    cycle->sync_state= sync_state;
    cycle->init = true;
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
    cycle->subSlot = 0;
    cycle->sSlot   = 0;
    cycle->actSlot = 0;
    cycle->lSlot   = 0;
    cycle->cycle   = 0;
    res = EM_OK;
    return res;
};

char *cycle_string(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return NULL;
    if (!cycle->init) return NULL;
    // clang-format on
    static char rStr[STRLEN];
    snprintf(rStr, STRLEN, SLOT_PRINT_FMT, cycle->cycle,  CYCLE_ACT_SLOT(cycle),  CYCLE_ACT_SUB_SLOT(cycle));
    return rStr;
}
int8_t cycle_act_slot(cycle_t *cycle ){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return CYCLE_ACT_SLOT(cycle);
};

int8_t cycle_sub_sub_slot(cycle_t *cycle ){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return CYCLE_ACT_SUB_SLOT(cycle);

};

uint16_t cycle_cycle(cycle_t *cycle){
    uint16_t res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return cycle->cycle;
}

int8_t cycle_check_slot(int8_t slot) {
    if (((slot > 0) && (slot <= CYCLE_SLOT_CNT)) && (slot % 2 == 1)) {
        return slot;
    }
    return -1;
}

em_msg   cycle_set_slot(cycle_t *cycle, int8_t slot, set_slot_e ss_type){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    if ((*cycle->sync_state == SYNCHRONIZE_DOING) || (*cycle->sync_state == SYNCHRONIZE_READY)){
        if (cycle_check_slot(slot) >= 0) {
            cycle_reset(cycle);
            *cycle->sync_state = SYNCHRONIZE_LOCKED;
            if (ss_type==MASTER){
                cycle->subSlot = slot * CYCLE_SUB_SLOT_CNT-cycle->press;
            } else{
                cycle->subSlot = slot * CYCLE_SUB_SLOT_CNT;
            }
            cycle->actSlot = CYCLE_ACT_SLOT(cycle);
            cycle->sSlot   = CYCLE_ACT_SUB_SLOT(cycle);
            res = EM_OK;
        } else {
            res = EM_ERR;
            printf("Set invalid slot %d" NL, slot);
        }
    }
    return res;
}

bool cycle_check(cycle_t *cycle, int8_t rxSlot) {
    bool inSlot = false;
    bool res = false;
    bool res_p = false;
    bool res_a = false;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (cycle_check_slot(rxSlot)<0) return res;
    // clang-format on
    inSlot = (CYCLE_ACT_SLOT(cycle) == rxSlot);
    res_a = (rxSlot == (( CYCLE_ACT_SLOT(cycle) + 1) %  CYCLE_SLOT_CNT)) && ( CYCLE_ACT_SUB_SLOT(cycle) < cycle->press);
    res_p = (rxSlot == (( CYCLE_ACT_SLOT(cycle) +  CYCLE_SLOT_CNT - 1) % CYCLE_SLOT_CNT)) && ((CYCLE_SUB_SLOT_CNT - CYCLE_ACT_SUB_SLOT(cycle)) < cycle->press);
    if (res_p) {
        printf("Pre" NL);
    }
    if (res_a) {
        printf("After" NL);
    }
    res = inSlot || res_p || res_a;
    if (res) {
        if (!inSlot) {
            printf("%s: out matches %d" NL, cycle_string(cycle), rxSlot);
        }
    }
    return res;
}

int8_t cycle_difference(cycle_t *cycle, int8_t rxSlot) {
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    const int16_t total = CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT; // 128 sub-slots per cycle
    const int16_t half = total / 2;                // 64
    // Signed sub-slot distance from the current position to the start of
    // rxSlot's region, taken the shortest way around the sub-slot ring.
    // Negative => rxSlot lies ahead of us, positive => rxSlot lies behind us.
    int16_t diff = (cycle->subSlot - (int16_t)rxSlot * CYCLE_SUB_SLOT_CNT) % total;
    if (diff < 0) diff += total;     // normalise to [0, total)
    if (diff >= half) diff -= total; // fold to [-half, half) => [-64, 63]
    return (int8_t)diff;
}

void cycle_increment(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return;
    if (!cycle->init) return;
    // clang-format on
    static bool is_set = false;
    static uint8_t cycle_once = false;
    if (*cycle->sync_state == SYNCHRONIZE) {
        *cycle->sync_state = SYNCHRONIZE_READY;
        is_set = true;
        return;
    }
    if (is_set) {
        cycle->subSlot++;
        cycle->subSlot = (cycle->subSlot % (CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT));
        cycle->actSlot = CYCLE_ACT_SLOT(cycle);
        cycle->sSlot = CYCLE_ACT_SUB_SLOT(cycle);
#if OPTION_SHOW_TIMING == 1
        em_msg res = stateled_toggle_pin(led_3);
#endif
    }
    if ((*cycle->sync_state == SYNCHRONIZE_DOING) || (*cycle->sync_state == SYNCHRONIZE_READY)
            || (*cycle->sync_state== SYNCHRONIZE_ERROR) ||  (*cycle->sync_state == SYNCHRONIZE_LOCKED)) {
        if (cycle->actSlot != cycle->lSlot) {
            cycle_once = false;
#if OPTION_SHOW_TIMING == 1
            stateled_set(cycle->actSlot);
            stateled_toggle_pin(led_4);
#endif
            cycle->lSlot = cycle->actSlot;
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
    printf("actSlot    = %x" NL, CYCLE_ACT_SLOT(cycle));
    printf("actSubSlot = %d" NL, CYCLE_ACT_SUB_SLOT(cycle));
    printf("cycle      = %d" NL, cycle->cycle);
    return EM_OK;
}
