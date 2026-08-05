/*
 * cycle->c
 *
 *  Created on: 20.05.2026
 *      Author: badi
 */

#include "cycle.h"
#include "assert.h"
#include "common.h"
#include "radio_state.h"
#ifndef UNIT_TEST
#include "options.h"
#include "stateled.h"
#include "stm32l4xx_hal_tim.h"
#endif
#include "app_x-cube-subg2.h"
#ifndef UNIT_TEST
typedef struct cycle_s {
    volatile uint8_t subSlot; // actual sub slot
    uint8_t psubSlot;         // Pending subslot to be used on next cycle_increment
    int8_t actSlot;
    int8_t lSlot;
    int8_t sSlot;
    uint16_t cycle;
    int8_t slot; // Configured slot of device
    int8_t press;
    int8_t postss;
    int8_t postrx;
    set_slot_e role;
    int8_t ssCnt;      // Counter for subslot count between cycle_sscnt_start and cycle_sscnt_stop after cycle_sscnt_init
    uint32_t timerCNT; // MCU cycle count when cycle count was set
    bool doMeasure;
    bool cntErrror;
    radio_state_t *rstate;
    bool init;
    TIM_HandleTypeDef *timer;
} cycle_t;
#endif

static idx2str_t cycle2str[] = {
    {.str = (char *)&"SLAVE ", .idx = SLAVE},  /*!< Device is slave */
    {.str = (char *)&"MASTER", .idx = MASTER}, /*!< Device is master */
};

idxa2str_t cyclea2str = {.cnt = ELCNT(cycle2str), .entry = (idx2str_t *)&cycle2str};

#define CYCLE_ACT_SUB_SLOT(_cycle) (((_cycle)->subSlot) & CYCLE_SUB_SLOT_MASK)

#define CYCLE_ACT_SLOT(_cycle) (((_cycle)->subSlot >> CYCLE_SUB_SLOT_SHIFT) & CYCLE_SLOT_MASK)

cycle_t cycle;

#define SLOT_PRINT_FMT "(c:%5d, %1x, %2d)" // length is 19
#define SLOT_PRINT_FMT_STR "(c:     ,  ,   )"
#define SLOT_PRINT_FMT_STR_LEN 16 + 2
em_msg cycle_init(cycle_t *cycle, int8_t my_slot, int8_t press, int8_t postss, uint8_t postrx, radio_state_t *rstate,
                  TIM_HandleTypeDef *htim) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!rstate) return res;
    // clang-format on
    cycle->press = press;
    cycle->postss = postss;
    cycle->postrx = postrx;
    cycle->slot = my_slot;
    cycle->timer = htim;
    cycle->rstate = rstate;
    cycle->init = true;
    cycle->role = NOT_SET;
    cycle->psubSlot = 0;
    cycle->cntErrror = 0;
    cycle_sscnt_init(cycle);
    cycle_reset(cycle);
    res = EM_OK;
    return res;
};

size_t cycle_size() { return sizeof(cycle_t); }

em_msg cycle_reset(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    cycle->subSlot = 0;
    cycle->sSlot = 0;
    cycle->actSlot = 0;
    cycle->lSlot = 0;
    cycle->cycle = 0;
    res = EM_OK;
    return res;
};
#if 1 == 0
em_msg cycle_timer_add(cycle_t *cycle, int8_t add) {
    em_msg res = EM_ERR;
    int32_t cnt;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (!cycle->timer) return res;
    // clang-format on
#ifndef UNIT_TEST
    __disable_irq();
    cnt = cycle->timer->Instance->CNT;
    // Reset the counter directly: no update event is generated, so no UIF is
    // raised and there is no spurious cycle_increment to guard against.
    uint32_t newTime = cycle->timer->Instance->ARR;
    if (add == 0)
        cTime = 1;
    if (add > 0) {
        if (cTime + add < preset) {
            cTime = (cTime - 1);
        } else {
            cTime += add;
        }
    } else {
        if (cTime + add < 0) {
            cTime = 0;
        } else {
            cTime += add;
        }
    }
    __HAL_TIM_SET_AUTORELOAD(cycle->timer, cTime);
    SET_BIT(cycle->timer->Instance->EGR, TIM_EGR_UG);
    // printf("add=%d  cTime=%lu"NL, add, cTime);
#endif
    __enable_irq();
    return EM_OK;
};
#endif

char *cycle_string(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return NULL;
    if (!cycle->init) return NULL;
    // clang-format on
    static char rStr[SLOT_PRINT_FMT_STR_LEN];
    snprintf(rStr, SLOT_PRINT_FMT_STR_LEN, SLOT_PRINT_FMT, cycle->cycle, CYCLE_ACT_SLOT(cycle), CYCLE_ACT_SUB_SLOT(cycle));
    return rStr;
}

int8_t cycle_act_slot(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return CYCLE_ACT_SLOT(cycle);
};

int8_t cycle_act_sub_slot(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return CYCLE_ACT_SUB_SLOT(cycle);
};

char *cycle_role(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return "NA ";
    if (!cycle->init) return "NA ";
    // clang-format on
    return idxa2str(&cyclea2str, cycle->role);
}

uint16_t cycle_cycle(cycle_t *cycle) {
    uint16_t res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return cycle->cycle;
}

bool cycle_doSend(cycle_t *cycle) {
    uint16_t res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    int8_t actSlot = CYCLE_ACT_SLOT(cycle);
    int8_t subSlot = CYCLE_ACT_SUB_SLOT(cycle);
    res = (actSlot == cycle->slot - 1) && ((CYCLE_SUB_SLOT_CNT - subSlot) < cycle->press);
#if OPTION_VERBOSE == 1
    if (res) {
        printf("Do send?                %s" NL, cycle_string(cycle));
    }
#endif
    return res;
};

int8_t cycle_handle_rx(cycle_t *cycle, AppliFrame_t *rxFrame) {
    int8_t rxSlot = AppliFrame_GetSlot(rxFrame);
    int8_t diff   = cycle_difference(cycle, rxSlot);
    bool   isAck  = AppliFrame_IsAck(rxFrame);

    em_msg res    = cycle_check_slot(rxSlot);
    if (res == EM_ERR) {
        printf("Invalid rxSlot = %d"NL, rxSlot);
        // radio_state_get_sync_state(&rstate) = SYNCHRONIZE_ERROR; Out of slot
        return rxSlot;
    }

    switch (radio_state_get_sync_state(&rstate)) {
        case SYNCHRONIZE_READY: {
            if (rxSlot == cycle_get_slot(cycle)) {
                radio_state_set_sync_state(&rstate,SYNCHRONIZE_ERROR);
                printf("rxSlot %d is mine %d  %s" NL, rxSlot, cycle_get_slot(cycle), cycle_string(cycle));
            } else {
                if (cycle_check_slot(rxSlot) >= 0) {
                    radio_state_inc_recv(&rstate, rxSlot);
                    cycle_sscnt_stop(cycle);
                    time_stop(rrxhdl, NULL);
                    if (cycle_set_slot(cycle, rxSlot, SLAVE) == EM_ERR) {
                        printf("Can not set cycle role to slave"NL);
                    }
                    printf("%s           %s SET ACT SLOT TO  %d" NL, idxa2str(&synca2str, radio_state_get_sync_state(&rstate)), cycle_string(cycle), rxSlot);
                } else {
                    printf("Invalid rxSlot %d %s" NL, rxSlot, cycle_string(cycle));
                }
            }
        }
        break;

        case SYNCHRONIZE_DOING: {
            cycle_sscnt_stop(cycle);
            time_stop(rrxhdl, NULL);
            //printf("SYNCHRONIZE_DOING       %s diff= %2d, rxSlot = %x"NL, cycle_string(cycle), diff, rxSlot);
            if (rxSlot == cycle_get_slot(cycle)){
                if (isAck){
                    radio_state_set_sync_state(&rstate, SYNCHRONIZE_ERROR);
                    printf("ACK in my Slot %x, rxSlot %x %s"NL, rxSlot, cycle_get_slot(cycle), cycle_string(cycle));
                } else {
                    printf("%s in rxSlot %x/my Slot %x %s"NL, AppliFrame_CmdStr(rxFrame), rxSlot, cycle_get_slot(cycle), cycle_string(cycle));
                }
            } else if (diff <= cycle_postrx(cycle)){
                radio_state_inc_recv(&rstate, rxSlot);
                //printf("%x++"NL, rxSlot);
                radio_state_set_sync_state(&rstate, radio_state_sync(&rstate));
#if OPTION_VERBOSE == 1
                printf("Receive slot (%d) matches %s  %s" NL, rxSlot, idxa2str(&synca2str, radio_state_get_sync_state(&rstate)), cycle_string(cycle));
#endif

            } else if (diff > RECEIVED_THRESHOLD){
                radio_state_inc_recve(&rstate, rxSlot);
                radio_state_set_sync_state(&rstate, radio_state_sync(&rstate));
#if OPTION_VERBOSE == 1
                printf("Out of slot frame received (%d) matches %s  %s" NL, rxSlot, idxa2str(&synca2str, radio_state_get_sync_state(&rstate)), cycle_string(cycle));
#endif
            } else {
                  radio_state_inc_recvd(&rstate, rxSlot);
                  radio_state_set_sync_state(&rstate, radio_state_sync(&rstate));

            }
        }
        break;

        case SYNCHRONIZE_LOCKED: {
            cycle_sscnt_stop(cycle);
            time_stop(rrxhdl, NULL);
            if ((rxSlot == cycle_get_slot(cycle)) && (!isAck)) {
                radio_state_set_sync_state(&rstate, SYNCHRONIZE_ERROR);
                printf("rxSlot %d is mine %d %s" NL, rxSlot, cycle_get_slot(cycle), cycle_string(cycle));
                // Someone send in my slot
            } else if (!(diff < cycle_postrx(cycle))){
                radio_state_inc_recv(&rstate, rxSlot);
            } else if ((diff > RECEIVED_THRESHOLD)) {
                radio_state_inc_recv(&rstate, rxSlot);
                printf("Slot missmatch rxSlot %X %s diff  = %d"NL, rxSlot, cycle_string(cycle), diff);
            }
        }

        break;
        default:
#if OPTION_VERBOSE == 1
            printf("Not covered state %s" NL, idxa2str(&synca2str, radio_state_get_sync_state(&rstate))); // do nothing
#endif
            ;
    }
    //printf("Handle %x"NL, rxSlot);
    return rxSlot;}



em_msg cycle_set_slot(cycle_t *cycle, int8_t slot, set_slot_e ss_type) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (cycle_check_slot(slot)<0) return res;
    system_state_e state = radio_state_get_sync_state(&rstate);
    if (state == SYNCHRONIZE_LOCKED) return EM_OK;
    if (state < SYNCHRONIZE_READY) return res;
    if ((ss_type < SLAVE) || (ss_type>MASTER)) return res;
    if (cycle->role != NOT_SET){
        return res;
    } else {
        cycle->role = ss_type;
    }
    if (ss_type != cycle->role) return res;
    // clang-format on
     if ((state == SYNCHRONIZE_DOING) || (state == SYNCHRONIZE_READY) ||
        (state == SYNCHRONIZE_ERROR) || (state == SYNCHRONIZE_LOCKED)) {
        if (ss_type == MASTER) {
#ifndef UNIT_TEST
            cycle->timerCNT = cycle->timer->Instance->CNT;
#endif
            cycle->psubSlot = (slot * CYCLE_SUB_SLOT_CNT + CYCLE_MODULO - cycle_press(cycle)) % CYCLE_MODULO;
            cycle->role = MASTER;
            res = EM_OK;
        } else {
            if (cycle_postss(cycle)>0){
                cycle->psubSlot = ((slot-1) * CYCLE_SUB_SLOT_CNT + CYCLE_MODULO + cycle_postss(cycle)) % CYCLE_MODULO;
            } else {
                cycle->psubSlot = ((slot-1) * CYCLE_SUB_SLOT_CNT + CYCLE_MODULO + (CYCLE_SUB_SLOT_CNT-cycle_postss(cycle))) % CYCLE_MODULO;

            }
            cycle->role = SLAVE;
            res = EM_OK;
        }
#ifndef UNIT_TEST
        __HAL_TIM_SET_COUNTER(cycle->timer, 0);
#endif
    }

    return res;
}

int8_t cycle_get_slot(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    return cycle->slot;
}



int8_t cycle_press(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    return cycle->press;
}

int8_t cycle_postss(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    return cycle->postss;
}

uint8_t   cycle_postrx(cycle_t *cycle){
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    return cycle->postrx;
};

int8_t cycle_difference(cycle_t *cycle, int8_t rxSlot) {
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    // Positive sub-slot distance from the current position to rxSlot's window,
    // with
    //   lower = rxSlot*CYCLE_SUB_SLOT_CNT
    //   upper = (rxSlot+1)*CYCLE_SUB_SLOT_CNT.
    // Inside [lower, upper) -> 0. Outside, the cycle is a ring of CYCLE_MODULO
    // sub-slots, so take the shorter way round:
    //   min((subSlot - upper) mod CYCLE_MODULO, (lower - subSlot) mod CYCLE_MODULO)
    // Always >= 0, never more than CYCLE_MODULO/2.
    const int16_t lower = (int16_t)rxSlot * CYCLE_SUB_SLOT_CNT;
    const int16_t upper = ((int16_t)rxSlot + 1) * CYCLE_SUB_SLOT_CNT;
    if ((cycle->subSlot >= lower) && (cycle->subSlot < upper)) {
        return 0; // inside the window
    }
    const int16_t above = ((cycle->subSlot - upper) + CYCLE_MODULO) % CYCLE_MODULO;
    const int16_t below = ((lower - cycle->subSlot) + CYCLE_MODULO) % CYCLE_MODULO;
    return (int8_t)((above < below) ? above : below);
}

void cycle_sscnt_init(cycle_t *cycle) {
    if (!cycle)
        return;
    if (!cycle->init)
        return;
    cycle->ssCnt = 0;
    cycle->doMeasure = false;
}
/*
 *  Measure how many subslot past between cycle_sscnt_start and
 *  cycle_sscnt_stop and return the value with cycle_sscnt_get.
 *  If an overflow occured on this int8_t value cycle_sscnt_get
 *  returns EM_ERR otherwise a value >=0
 */
void cycle_sscnt_start(cycle_t *cycle) {
    if (!cycle)
        return;
    if (!cycle->init)
        return;
    assert(cycle->doMeasure != true);
    cycle->doMeasure = true;
}

void cycle_sscnt_stop(cycle_t *cycle) {
    if (!cycle)
        return;
    if (!cycle->init)
        return;
    cycle->doMeasure = false;
}

uint8_t cycle_sscnt_get(cycle_t *cycle) {
    if (!cycle)
        return -1;
    if (!cycle->init)
        return -1;
    if (!cycle->cntErrror) {
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
    system_state_e state = radio_state_get_sync_state(&rstate);
    if (state == SYNCHRONIZE) {
        radio_state_set_sync_state(&rstate, SYNCHRONIZE_READY);
        is_set = true;
    }
    if (is_set) {
        if (cycle->psubSlot) {
            cycle->subSlot = cycle->psubSlot;
            cycle->psubSlot = 0;
        }
        cycle->subSlot++;
        cycle->subSlot = (cycle->subSlot % (CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT));
        cycle->actSlot = CYCLE_ACT_SLOT(cycle);
        cycle->sSlot = CYCLE_ACT_SUB_SLOT(cycle);
#if OPTION_SHOW_TIMING == 1
        // stateled_set(cycle->sSlot);
        stateled_toggle_pin(led_3);
#endif
    }
    if (cycle->doMeasure) {
        int8_t lss = cycle->ssCnt;
        cycle->ssCnt++;
        if (cycle->ssCnt < lss) {
            cycle->cntErrror = true;
        }
    }
    if ((state == SYNCHRONIZE_DOING) || (state == SYNCHRONIZE_READY) ||
        (state == SYNCHRONIZE_ERROR) || (state == SYNCHRONIZE_LOCKED)) {
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
