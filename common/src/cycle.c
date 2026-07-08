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
    volatile int8_t   subSlot; // actual sub slot
    int8_t            actSlot;
    int8_t            lSlot;
    int8_t            sSlot;
    uint16_t          cycle;
    int8_t            press;
    set_slot_e        role;
    int8_t            ssCnt;
    bool              doMeasure;
    bool              cntErrror;
    system_state_e    *sync_state;
    bool              init;
    TIM_HandleTypeDef *timer;
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

em_msg cycle_init(cycle_t *cycle, int8_t press , system_state_e *sync_state, TIM_HandleTypeDef *htim) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!sync_state) return res;
    // clang-format on
    cycle->press= press;
    cycle->timer= htim;
    cycle->sync_state= sync_state;
    cycle->init = true;
    cycle->role = SLAVE;
    cycle_sscnt_init(cycle);
    cycle_reset(cycle);
    res = EM_OK;
    return res;
};

size_t cycle_size(){
    return sizeof(cycle_t);
}

em_msg cycle_reset(cycle_t *cycle){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    cycle->subSlot  = 0;
    cycle->sSlot    = 0;
    cycle->actSlot  = 0;
    cycle->lSlot    = 0;
    cycle->cycle    = 0;
    cycle->ssCnt    = -1;
    cycle->doMeasure= false;
    res = EM_OK;
    return res;
};

em_msg cycle_reset_timer(cycle_t *cycle){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (!cycle->timer) return res;
    // clang-format on
    // Reset the counter directly: no update event is generated, so no UIF is
    // raised and there is no spurious cycle_increment to guard against.
    cycle->timer->Instance->CNT = 0;
    return EM_OK;
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

em_msg   cycle_set_slot(cycle_t *cycle, int8_t slot, int8_t add, set_slot_e ss_type){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (cycle_check_slot(slot)<0) return res;
    if (*cycle->sync_state == SYNCHRONIZE_LOCKED) return res;
    cycle->role = ss_type;
    // clang-format on
    if ((*cycle->sync_state == SYNCHRONIZE_DOING) || (*cycle->sync_state == SYNCHRONIZE_READY)){
#ifndef UNIT_TEST
         uint32_t primask = __get_PRIMASK();
         __disable_irq();            // block TIM1 update ISR for the whole update
#endif
        cycle_reset_timer(cycle);
        cycle_reset(cycle);
        *cycle->sync_state = SYNCHRONIZE_LOCKED;
        if (ss_type==MASTER){
            cycle->role = MASTER;
            cycle->subSlot = slot * CYCLE_SUB_SLOT_CNT-cycle->press;;        } else{
            cycle->role = SLAVE;
            cycle->subSlot = (slot * CYCLE_SUB_SLOT_CNT+CYCLE_MODULO+add)%CYCLE_MODULO;
        }
        cycle->actSlot = CYCLE_ACT_SLOT(cycle);
        cycle->sSlot   = CYCLE_ACT_SUB_SLOT(cycle);
        res = EM_OK;
#ifndef UNIT_TEST
         __set_PRIMASK(primask);     // restore (don't blindly __enable_irq())
#endif

     }

     return res;
}


system_state_e   cycle_state(cycle_t *cycle){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    return *cycle->sync_state;
}

bool cycle_isOk(cycle_t *cycle, int8_t rxSlot) {
    // clang-format off
    if (!cycle) return false;
    if (!cycle->init) return false;
    if (cycle_check_slot(rxSlot) < 0) return false;
    // clang-format on
    // OK when the distance to rxSlot's window exceeds the press tolerance.
    return cycle_difference(cycle, rxSlot) < cycle->press;
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
    // Positive sub-slot distance from the current position to rxSlot's window,
    // with
    //   lower = rxSlot*CYCLE_SUB_SLOT_CNT
    //   upper = (rxSlot+1)*CYCLE_SUB_SLOT_CNT.
    // Below the lower corner -> lower - subSlot
    // At/above the upper edge -> subSlot - lower
    // Inside [lower, upper)  -> 0  (8 sub-slots wide)
    // Always >= 0, no ring wrap-around.
    const int16_t lower = (int16_t)rxSlot * CYCLE_SUB_SLOT_CNT;
    const int16_t upper = ((int16_t)rxSlot + 1) * CYCLE_SUB_SLOT_CNT;
    if (cycle->subSlot <= lower) {
        return (int8_t)(lower - cycle->subSlot); // below the lower corner
    } else if (cycle->subSlot >= upper) {
        return (int8_t)(cycle->subSlot - upper); // above the upper corner
    }
    return 0; // inside the window
}
void     cycle_sscnt_init(cycle_t *cycle){
    if (!cycle) return;
    if (!cycle->init) return;
    cycle->ssCnt     = 0;
    cycle->doMeasure = false;
}
/*
 *  Measure how many subslot past between cycle_sscnt_start and
 *  cycle_sscnt_stop and return the value with cycle_sscnt_get.
 *  If an overflow occured on this int8_t value cycle_sscnt_get
 *  returns EM_ERR otherwise a value >=0
 */
void     cycle_sscnt_start(cycle_t *cycle){
    if (!cycle) return;
    if (!cycle->init) return;
    cycle->doMeasure = true;
}

void     cycle_sscnt_stop(cycle_t *cycle){
    if (!cycle) return;
    if (!cycle->init) return;
    cycle->doMeasure = false;
}

uint8_t  cycle_sscnt_get(cycle_t *cycle){
    if (!cycle) return -1;
    if (!cycle->init) return -1;
    if (!cycle->cntErrror){
        return cycle->ssCnt;
    }
    return EM_ERR;
};

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
    }
    if (is_set) {
        cycle->subSlot++;
        cycle->subSlot = (cycle->subSlot % (CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT));
        cycle->actSlot = CYCLE_ACT_SLOT(cycle);
        cycle->sSlot = CYCLE_ACT_SUB_SLOT(cycle);
#if OPTION_SHOW_TIMING == 1
        stateled_set(cycle->sSlot);
        stateled_toggle_pin(led_3);
#endif
    }
    if (cycle->doMeasure){
        int8_t lss =cycle->ssCnt;
        cycle->ssCnt++;
        if (cycle->ssCnt<lss){
            cycle->cntErrror = true;
        }
    }
    if ((*cycle->sync_state == SYNCHRONIZE_DOING) || (*cycle->sync_state == SYNCHRONIZE_READY)
            || (*cycle->sync_state== SYNCHRONIZE_ERROR) ||  (*cycle->sync_state == SYNCHRONIZE_LOCKED)) {
        if (cycle->actSlot != cycle->lSlot) {
            cycle_once = false;
#if OPTION_SHOW_TIMING == 1
            //stateled_set(cycle->actSlot);
            stateled_toggle_pin(led_4);
#endif
            cycle->lSlot = cycle->actSlot;
            if (((cycle->actSlot == 0) && (is_set) && (!cycle_once))) {
#if OPTION_SHOW_TIMING == 1
                stateled_toggle_pin(led_5);
#endif
                cycle_once = true;
                cycle->cycle += 1;
//                if (cycle->cycle%CYCLE_RESET_CNT==0){
//                    *cycle->sync_state = SYNCHRONIZE;
//                }
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
