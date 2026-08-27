/*
 * cycle->c
 *
 *  Created on: 20.05.2026
 *      Author: badi
 */

#include "cycle.h"
#include "assert.h"
#include "common.h"
#ifndef UNIT_TEST
#include "options.h"
#include "stateled.h"
#include "stm32l4xx_hal_tim.h"
#endif
#ifndef UNIT_TEST
typedef struct cycle_s {
    volatile uint8_t subSlot; // actual sub slot
    uint8_t           psubSlot;         // Pending subslot to be used on next cycle_increment
    int8_t           actSlot;
    int8_t           lSlot;
    int8_t           sSlot;
    uint16_t         cycle;
    int8_t           slot; // Configured slot of device
    bool             isSlave;
    int8_t           master;
    bool             isMaster;
    uint16_t         masterAge; // frame cycles since the network was last heard from
    int8_t           press;
    int8_t           postss;
    int8_t           postrx;
    dev_role_e       role;
    int8_t           ssCnt;      // Counter for subslot count between cycle_sscnt_start and cycle_sscnt_stop after cycle_sscnt_init
    uint32_t         timerCNT; // MCU cycle count when cycle count was set
    bool             doMeasure;
    bool             cntErrror;
    system_state_e   sync_state;
    bool             init;
    bool             set;  // is set when cycle was finished
    TIM_HandleTypeDef *timer;
} cycle_t;
#endif

static idx2str_t cycle2str[] = {
    {.str = (char *)&"SLAVE ", .idx = SLAVE},  /*!< Device is slave */
    {.str = (char *)&"MASTER", .idx = MASTER}, /*!< Device is master */
};
#ifndef KEEP_ALIVE_CYCLE_VALUE
#define KEEP_ALIVE_CYCLE_VALUE 8
#endif
idxa2str_t cyclea2str = {.cnt = ELCNT(cycle2str), .entry = (idx2str_t *)&cycle2str};

#define CYCLE_ACT_SUB_SLOT(_cycle) (((_cycle)->subSlot) & CYCLE_SUB_SLOT_MASK)

#define CYCLE_ACT_SLOT(_cycle) (((_cycle)->subSlot >> CYCLE_SUB_SLOT_SHIFT) & CYCLE_SLOT_MASK)

cycle_t cycle;

#define SLOT_PRINT_FMT "(c:%5d, %1x, %2d)" // length is 19
#define SLOT_PRINT_FMT_STR "(c:     ,  ,   )"
#define SLOT_PRINT_FMT_STR_LEN 16 + 2
em_msg cycle_init(cycle_t *cycle, int8_t my_slot, int8_t press, int8_t postss, uint8_t postrx, TIM_HandleTypeDef *htim) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!htim)  return res;
    if (cycle_check_slot(my_slot)<0)  return res;
    // clang-format on
    memset(cycle, 0, sizeof(cycle_t));
    cycle->press  = press;
    cycle->postss = postss;
    cycle->postrx = postrx;
    cycle->slot   = my_slot;
    cycle->master  = -1;
    cycle->isSlave = false;
    cycle->isMaster= false;
    cycle->timer  = htim;
    cycle->sync_state = SYNC_RESET;
    cycle->role = NOT_SET;
    cycle->subSlot = 0;
    cycle->psubSlot = 0;
    cycle->cntErrror = 0;
    cycle_sscnt_init(cycle);
    cycle->init  = true;
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
    cycle->subSlot = cycle->slot*CYCLE_SUB_SLOT_CNT-cycle->press;
    cycle->psubSlot = 0;
    cycle->sSlot = 0;
    cycle->actSlot = CYCLE_ACT_SLOT(cycle);
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


dev_role_e cycle_role(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return SS_CNT;
    if (!cycle->init) return SS_CNT;
    // clang-format on
    return cycle->role;
}

char *cycle_role_str(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return "NA ";
    if (!cycle->init) return "NA ";
    // clang-format on
    return idxa2str(&cyclea2str, cycle->role);
}

bool cycle_role_is_set(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return false;
    if (!cycle->init) return false;
    // clang-format on
    return cycle->role != NOT_SET;
}

void cycle_reset_role(cycle_t *cycle) {
    // The one "drop the role" primitive: it puts the device back into the
    // state it boots in, so the next frame heard re-elects a master
    // (cycle_master_seen). The master watchdog in cycle_increment() calls it
    // after CYCLE_MASTER_LOOSE_CYCLE_CNT silent frame cycles.
    //
    // A forced resync is otherwise silently swallowed: cycle_set_slot() only
    // recomputes psubSlot/timerCNT the *first* time it is called after role
    // goes back to NOT_SET, so without this every later resync trigger is a
    // no-op for timing purposes even though sync_state visibly changes.
    //
    // isMaster has to go with it. It is the one-shot guard against a master
    // re-latching onto its own timing (see cycle_set_slot); keeping it across
    // a drop would mean no device is ever elected master twice, and the
    // network dies at the first timeout.
    // clang-format off
    if (!cycle) return;
    if (!cycle->init) return;
    // clang-format on
    cycle->role      = NOT_SET;
    cycle->isMaster  = false;
    cycle->master    = -1;
    cycle->masterAge = 0;
}

system_state_e cycle_get_state(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return cycle->sync_state;
}

em_msg cycle_set_state(cycle_t *cycle, system_state_e state ) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (state >= SYNC_CNT) return res;
    // clang-format on
    cycle->sync_state = state;
    res = EM_OK;
    return res;
}

int8_t cycle_get_master(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return cycle->master;
}

bool cycle_doSend(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    int8_t actSlot = CYCLE_ACT_SLOT(cycle);
    int8_t subSlot = CYCLE_ACT_SUB_SLOT(cycle);
    res = (actSlot == cycle->slot - 1) && ((CYCLE_SUB_SLOT_CNT - subSlot) < cycle->press);
#if MOPTION_VERBOSE == 1
    res=1;
    if (res) {
        printf("Do send?                %s" NL, cycle_string(cycle));
    }
#endif
    return res;
};

int8_t cycle_check_slot(int8_t slot) {
    if (((slot > 0) && (slot <= CYCLE_SLOT_CNT)) && (slot % 2 == 1)) {
        return slot;
    }
    return -1;
}

em_msg cycle_set_slot(cycle_t *cycle, int8_t slot, dev_role_e ss_type) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (cycle_check_slot(slot)<0) return res;
    if ((ss_type < SLAVE) || (ss_type>MASTER)) return res;
    // MASTER is a one-shot bootstrap claim, for a device that has not heard
    // anyone yet. A master derives the cycle from its own TX slot, so latching
    // it a second time recomputes psubSlot and resets the timer counter
    // against that same self-reference -- shifting the phase every slave has
    // synced to, for no gain. cycle->isMaster records that the claim happened
    // and is released only by cycle_reset_role(), i.e. by the master watchdog
    // in cycle_increment() -- so the role is claimable once per election, and
    // a device can be elected again after a timeout has torn the old master
    // down. SLAVE claims are never latched: a slave re-syncs on every frame
    // it receives from its master.
    //
    // The guard sits in the MASTER branch below and keys off cycle->role, not
    // ss_type, so a latched master cannot demote onto a peer it finally hears
    // -- the SLAVE claim is rejected too (SlaveClaimRejectedAfterMasterLatch).
    // Demotion is the watchdog's job alone, which keeps one path for it
    // instead of racing an RX against a timeout.
    if (cycle->role == NOT_SET){
        cycle->role = ss_type;
    }
    if ((cycle->sync_state == SYNCHRONIZE) || (cycle->sync_state == SYNCHRONIZE_READY)  ||
        (cycle->sync_state == SYNCHRONIZE_DOING) || (cycle->sync_state == SYNCHRONIZE_ERROR) ||
        (cycle->sync_state == SYNCHRONIZE_LOCKED)) {
            if (cycle->role == MASTER) {
                if (cycle->isMaster) return EM_ERR;
                cycle->isMaster = true;
#ifndef UNIT_TEST
                cycle->timerCNT = cycle->timer->Instance->CNT;
#endif
                cycle->psubSlot = (slot * CYCLE_SUB_SLOT_CNT + CYCLE_MODULO - cycle_press(cycle)) % CYCLE_MODULO;
                cycle->master   = slot;
                cycle->isSlave = true;

                res = EM_OK;
            } else {
            	if (cycle->isSlave) return EM_ERR;
                cycle->psubSlot = (slot * CYCLE_SUB_SLOT_CNT + CYCLE_MODULO - cycle_press(cycle)) % CYCLE_MODULO;
                cycle->isSlave = true;
                res = EM_OK;
            }
            // A successful claim is proof the network is there: restart the
            // watchdog so the fresh role gets a full CYCLE_MASTER_LOOSE_CYCLE_CNT.
            cycle->masterAge = 0;
#ifndef UNIT_TEST
        __HAL_TIM_SET_COUNTER(cycle->timer, 0);
#endif
    }

    return res;
}

// RX path entry point: a frame arrived in sub-slot window rxSlot.
//
// It kicks the master watchdog, and elects a master when there is none. What
// counts as proof that the network is still there depends on the role:
//   NOT_SET -- nobody is master (boot, or the watchdog just fired). The first
//              sender heard wins the election: it becomes cycle->master and we
//              latch our own timing onto its slot as SLAVE.
//   SLAVE   -- only a frame from cycle->master counts. Accepting any frame
//              would keep a partitioned group alive with no master in it.
//   MASTER  -- any frame counts. A master is its own timing reference and
//              never hears itself, so requiring its own slot here would time
//              a perfectly healthy master out every CYCLE_MASTER_LOOSE_CYCLE_CNT
//              cycles.
// Returns EM_OK when the frame kicked the watchdog, EM_ERR when it was
// ignored (wrong slot for a slave) or the election could not be latched.
em_msg cycle_master_seen(cycle_t *cycle, int8_t rxSlot) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (cycle_check_slot(rxSlot)<0) return res;
    // clang-format on
    if (cycle->role == NOT_SET) {
        res = cycle_set_slot(cycle, rxSlot, SLAVE);
        if (res == EM_OK) {
            cycle->master = rxSlot;
        }
        return res;
    }
    if ((cycle->role == SLAVE) && (rxSlot != cycle->master)) {
        return res;
    }
    cycle->masterAge = 0;
    res = EM_OK;
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

uint8_t cycle_difference(cycle_t *cycle, int8_t rxSlot) {
    // clang-format off
    if (!cycle) return EM_ERR;
    if (!cycle->init) return EM_ERR;
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
    return ((above < below) ? above : below);
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
    if (cycle->sync_state == SYNCHRONIZE) {
        cycle->sync_state = SYNCHRONIZE_READY;
        is_set = true;
    }
    if (is_set) {
        if (cycle->psubSlot>0) {
            cycle->subSlot = cycle->psubSlot;
            cycle->psubSlot = 0;
        }
        if ((cycle->sync_state == SYNCHRONIZE_DOING) || (cycle->sync_state == SYNCHRONIZE_READY) ||
            (cycle->sync_state == SYNCHRONIZE_ERROR) || (cycle->sync_state == SYNCHRONIZE_LOCKED)) {
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
                cycle->set = true;
                // Master watchdog. Every device that holds a role ages here;
                // cycle_master_seen() resets the age on every frame that proves
                // the network is still there. Running dry means the master is
                // gone (or, for a master, that nobody is left listening), so the
                // role is dropped and the next frame heard elects a new master.
                if (cycle->role != NOT_SET) {
                    cycle->masterAge++;
                    if (cycle->masterAge >= CYCLE_MASTER_LOOSE_CYCLE_CNT) {
                        cycle_reset_role(cycle);
                    }
                }
                if (cycle->cycle%KEEP_ALIVE_CYCLE_VALUE==0){
                    cycle_set_state(cycle, SYNCHRONIZE);
                }
            }
        }
    }
}

bool     cycle_is_set(cycle_t *cycle){
    bool res = false;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    bool state = cycle->set;
    cycle->set = false;
    return state;
};

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
    printf("cycle      = %d" NL, cycle_get_state(cycle));
    printf("my role    = %s" NL, cycle_role_str(cycle));
    printf("master is  = %d" NL, cycle->master);
    return EM_OK;
}
