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

#ifndef UNIT_TEST
typedef struct cycle_s {
    volatile int8_t subSlot; // actual sub slot
    int8_t    actSlot;
    int8_t    lSlot;
    int8_t    sSlot;
    uint16_t  cycle;
    int8_t    press;
    set_slot_e role;
    system_state_e *sync_state;
    bool      init;
} cycle_t;
#endif


static idx2str_t cycle2str[] = {
    {.str = (char *)&"SLAVE ", .idx = SLAVE},        /*!< Device is slave */
    {.str = (char *)&"MASTER", .idx = MASTER},       /*!< Device is master */
};

idxa2str_t cyclea2str = {.cnt = ELCNT(cycle2str), .entry = (idx2str_t *)&cycle2str};

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
    cycle->role = SLAVE;
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
    cycle->role    = SLAVE;
    res = EM_OK;
    return res;
};

char *cycle_string(cycle_t *cycle){
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

int8_t cycle_act_sub_slot(cycle_t *cycle ){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return CYCLE_ACT_SUB_SLOT(cycle);

};

char *   cycle_role(cycle_t *cycle){
    return idxa2str(&cyclea2str, cycle->role);
}

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
    if (cycle_check_slot(slot)<0) return res;
    cycle->role = ss_type;
    // clang-format on
    if ((*cycle->sync_state == SYNCHRONIZE_DOING) || (*cycle->sync_state == SYNCHRONIZE_READY)){
        if (cycle_check_slot(slot) >= 0) {
            cycle_reset(cycle);
            *cycle->sync_state = SYNCHRONIZE_LOCKED;
            if (ss_type==MASTER){
                cycle->role = MASTER;
                cycle->subSlot = slot * CYCLE_SUB_SLOT_CNT-cycle->press;
            } else{
                cycle->role = SLAVE;
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

bool cycle_isOk(cycle_t *cycle, int8_t rxSlot) {
    bool res = false;
    bool inSlot = false;
    bool res_p = false;
    bool res_a = false;
    bool pre_slot = false;
    bool after_slot= false;
    int8_t last_slot;
    int8_t next_slot;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (cycle_check_slot(rxSlot)<0) return res;
    // clang-format on
    inSlot = (CYCLE_ACT_SLOT(cycle) == rxSlot);
    last_slot = (( CYCLE_ACT_SLOT(cycle) +  CYCLE_SLOT_CNT - 1) % CYCLE_SLOT_CNT);
    next_slot = (( CYCLE_ACT_SLOT(cycle) + 1) %  CYCLE_SLOT_CNT);
    pre_slot =   (rxSlot ==  last_slot);
    after_slot = (rxSlot ==  next_slot);
    bool  pre_sub_diff =   ((CYCLE_SUB_SLOT_CNT - CYCLE_ACT_SUB_SLOT(cycle)) < cycle->press);
    bool  after_sub_diff =  ( CYCLE_ACT_SUB_SLOT(cycle) < cycle->press);
    res_a = after_slot && after_sub_diff;
    res_p = pre_slot   && pre_sub_diff;
    if (pre_slot) {
        printf("Pre" NL);
    }
    if (after_slot) {
        printf("After" NL);
    }
    res = inSlot || res_p || res_a;
    if (res) {
        if (inSlot) {
            printf("%s: is in slot %d" NL, cycle_string(cycle), rxSlot);
        }
    }
    return res;
}

int8_t   cycle_press(cycle_t *cycle){
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    return cycle->press;

}

int8_t cycle_difference(cycle_t *cycle, int8_t rxSlot) {
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    // Signed sub-slot distance from the current position to rxSlot's window.
    // The window spans CYCLE_SUB_SLOT_CNT sub-slots with
    //   lower corner = rxSlot*CYCLE_SUB_SLOT_CNT
    //   upper corner = lower + CYCLE_SUB_SLOT_CNT - 1.
    // Inside the window the distance is 0; below the lower corner it is the
    // (negative) distance to that corner; above the upper corner it is the
    // (positive) distance past it. Distances are measured the shortest way
    // around the CYCLE_SUB_SLOT_CNT*CYCLE_SLOT_CNT ring, so rxSlot 0 and
    // rxSlot CYCLE_SLOT_CNT yield the same result.
    const int16_t total = CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT; // 128
    const int16_t half  = total / 2;                           // 64
    int16_t d = (cycle->subSlot - (int16_t)rxSlot * CYCLE_SUB_SLOT_CNT) % total;
    if (d < 0) {
        d += total; // normalise to [0, total)
    }
    if (d >= half) {
        d -= total; // fold to [-half, half): signed distance to the lower corner
    }
    if (d < 0) {
        return (int8_t)d; // below the lower corner
    }
    if (d < CYCLE_SUB_SLOT_CNT) {
        return 0; // inside the window
    }
    return (int8_t)(d - (CYCLE_SUB_SLOT_CNT - 1)); // above the upper corner
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
        stateled_toggle_pin(led_3);
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
                stateled_toggle_pin(led_5);
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
    printf("actSlot    = %x" NL, cycle_act_slot(cycle));
    printf("actSubSlot = %d" NL, cycle_act_sub_slot(cycle));
    printf("cycle      = %d" NL, cycle_cycle(cycle));
    printf("role       = %s" NL, cycle_role(cycle));
    return EM_OK;
}
