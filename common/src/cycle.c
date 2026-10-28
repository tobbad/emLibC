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

#if defined(STM32F303xC)
#include "stm32f4xx_hal_tim.h"
#elif defined(STM32F407xx) || defined(STM32F401xE)
#include "stm32f4xx_hal_tim.h"
#elif defined(STM32H755xx) || defined(STM32H743xx)
#include "stm32h7xx_hal_tim.h"
#elif defined(STM32L476xx) || defined(STM32L432xx) || defined(STM32L412xx) || defined(STM32L475xx)
#include "stm32l4xx_hal_tim.h"
#elif defined(STM32L053xx)
#include "stm32l0xx_hal_tim.h"
#elif defined(STM32L053xx)
#include "stm32l0xx_hal_tim.h"
#endif
#endif
#ifndef UNIT_TEST
typedef struct cycle_s {
    // clang-format off
    volatile uint8_t   subSlot; // actual sub slot
    volatile uint8_t   psubSlot;         //  Pendig difference subslot value
    volatile system_state_e sync_state;
    volatile system_state_e psync_state;
    volatile int8_t   actSlot;
    volatile int8_t   lSlot;
    volatile int8_t   sSlot;
    volatile uint16_t cycle;
    int8_t            slot; // Configured slot of device
    dev_handle_t      hdl;
    int8_t            master;
    bool              isSlave;
    bool              isMaster;
    uint16_t          masterAge; // frame cycles since the network was last heard from
    uint16_t          slaveAge;  // iterates over slave cycles till role is resetted
    int8_t            tx_ss;
    int8_t            rx_ss;
    int8_t            postrx;
    dev_role_e        role;
    int8_t            kaCnt;      // Set Keep alive counter
    int8_t            _kaCnt;     // Keep alive counter
    uint32_t          timerCNT; // MCU cycle count when cycle count was set
    uint16_t          ssCnt[RADIO_CNT];    // Counter for subslot count between cycle_sscnt_start and cycle_sscnt_stop after cycle_sscnt_init
    bool              doMeasure[RADIO_CNT];
    bool              cntErrror[RADIO_CNT];
    bool              init;
    uint8_t           idx;   // on cycle is idx= 0, 1 and 2
    bool              set; // is set when cycle was finished
    TIM_HandleTypeDef *timer;
    // clang-format on
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
#ifndef UNIT_TEST
// subSlot packs the position in the frame cycle: the upper nibble is the slot,
// the lower nibble the sub-slot within it (e.g. 223 = 0xDF -> slot 13, sub-slot 15).
// clang-format off
#define CYCLE_ACT_SUB_SLOT_N(ss)   (((ss) >> CYCLE_SUB_SLOT_SHIFT) & CYCLE_SUB_SLOT_MASK)
#define CYCLE_ACT_SUB_SLOT(_cycle) (CYCLE_ACT_SUB_SLOT_N((_cycle)->subSlot))
#define CYCLE_ACT_SLOT_N(ss)       (((ss) >> CYCLE_SLOT_SHIFT) & CYCLE_SLOT_MASK)
#define CYCLE_ACT_SLOT(_cycle)     (CYCLE_ACT_SLOT_N((_cycle)->subSlot))
// clang-format on
#endif

cycle_t cycle;
cycle_t mcycle;

#define SLOT_PRINT_FMT "(c:%5d, %1x, %1x)" // length is 19
#define SLOT_PRINT_FMT_STR_LEN 16 + 2

em_msg cycle_init(cycle_t *cycle, dev_handle_t hdl, int8_t tx_ss, int8_t rx_ss, uint8_t postrx, uint8_t kaCnt, TIM_HandleTypeDef *htim) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!htim)  return res;
    memset(cycle, 0, sizeof(cycle_t));
    cycle->tx_ss    = tx_ss;
    cycle->rx_ss    = rx_ss;
    cycle->postrx   = postrx;
    cycle->kaCnt    = kaCnt;
    cycle->timer    = htim;
    cycle->subSlot  = 0;
    cycle->idx      = 0;
    cycle->slot     = SLOT_NOT_SET;
    cycle->master   = SLOT_NOT_SET;
    cycle->hdl      = hdl;
    cycle->init     = true;
    cycle->sync_state = SYNC_RESET;
    cycle->psync_state = SYNC_RESET;
    // clang-format on
    cycle_sscnt_init(cycle, RADIO_RX);
    cycle_sscnt_init(cycle, RADIO_TX);
    cycle_reset_ka(cycle);
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
    cycle->subSlot  = 0;
    cycle->psubSlot = 0;
    cycle->sSlot    = 0;
    cycle->actSlot  = 0;
    cycle->lSlot    = SLOT_NOT_SET;
    cycle->cycle    = 0;
    cycle->isMaster = false;
    cycle->isSlave  = false;
    cycle->slaveAge = 0;
    cycle->masterAge= 0;
    // clang-format on
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
    if (!cycle) return DEFAULT_CHAR_NULL;
    if (!cycle->init) return DEFAULT_CHAR_NULL;
    // clang-format on
    static char rStr[SLOT_PRINT_FMT_STR_LEN];
    snprintf(rStr, SLOT_PRINT_FMT_STR_LEN, SLOT_PRINT_FMT, cycle->cycle, CYCLE_ACT_SLOT(cycle), CYCLE_ACT_SUB_SLOT(cycle));
    return rStr;
}

char *cycle_text_char(cycle_t *cycle, const char *text) {
    // clang-format off
    if (!cycle) return DEFAULT_CHAR_NULL;
    if (!cycle->init) return DEFAULT_CHAR_NULL;
    if (!text) return DEFAULT_CHAR_NULL;
    // clang-format on
    uint8_t text_len = strlen(text);
    static char rbuf[TX_BUFFER_SIZE];
    char *cycle_str = cycle_string(cycle);
    memset(rbuf, ' ', TX_BUFFER_SIZE);
    text_len = MIN(text_len, CYCLE_POSITION);
    memcpy(rbuf, text, text_len);
    memcpy(&rbuf[CYCLE_POSITION], cycle_str, strlen(cycle_str));
    text_len = CYCLE_POSITION + strlen(cycle_str) + 1;
    rbuf[text_len] = 0;
    return rbuf;
}
em_msg cycle_text_print(cycle_t *cycle, const char *text) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (!text) return res;
    // clang-format on
    char *line = cycle_text_char(cycle, text);
    printf("%s" NL, line);
    return EM_OK;
};

em_msg cycle_text_print_s(cycle_t *cycle, const char *text, char *str) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (!text) return res;
    // clang-format on
    char buf[TX_BUFFER_SIZE];
    snprintf(buf, TX_BUFFER_SIZE, "%s", str);
    char *line = cycle_text_char(cycle, text);
    printf("%s%s" NL, line, buf);
    return EM_OK;
};

em_msg cycle_text_print_v(cycle_t *cycle, const char *text, void *data) {
    char uint32[] = "%d";
    char x32[] = "%x";
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (!text) return res;
    // clang-format on
    char buf[TX_BUFFER_SIZE];
    if (strstr(text, uint32) != NULL) {
        snprintf(buf, TX_BUFFER_SIZE, text, *(uint32_t *)data);
    } else if (strstr(text, x32) != NULL) {
        snprintf(buf, TX_BUFFER_SIZE, text, *(uint32_t *)data);
    }
    char *line = cycle_text_char(cycle, text);
    printf("%s%s" NL, line, buf);
    return EM_OK;
}

int8_t cycle_act_slot(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return CYCLE_ACT_SLOT(cycle);
}

int8_t cycle_act_sub_slot(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return CYCLE_ACT_SUB_SLOT(cycle);
}

em_msg cycle_update(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (cycle->slot != SLOT_NOT_SET) {
        cycle->sSlot   = CYCLE_ACT_SUB_SLOT(cycle);
        cycle->actSlot = CYCLE_ACT_SLOT(cycle);
        cycle->lSlot   = CYCLE_ACT_SLOT_N(cycle->subSlot - 1);
    }
    // clang-format on
    return EM_OK;
}

uint16_t cycle_cycle(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return -1;
    if (!cycle->init) return -1;
    // clang-format on

    return cycle->cycle;
}

// Decrement keep alive
em_msg cycle_dec_ka(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    cycle->_kaCnt--;
    cycle->_kaCnt = MAX(0, cycle->_kaCnt);
    return EM_OK;
};

bool cycle_is_ka(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return cycle->_kaCnt == 0;
};

em_msg cycle_reset_ka(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
	if (!cycle) return res;
	if (!cycle->init) return res;
    // clang-format on
    cycle->_kaCnt = cycle->kaCnt;
    return EM_OK;
}

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
    // Reset slave role
    if (!cycle) return;
    if (!cycle->init) return;
    if (cycle->isSlave) {
    // clang-format on
        cycle->role     = NOT_SET;
        cycle->isSlave  = false;
        cycle->slaveAge = 0;
     // clang-format on
   }
}

em_msg cycle_reset_subslot(cycle_t *cycle) {
    // clang-format off
    em_msg res = EM_ERR;
    // Reset slave role
    if (!cycle)
        return res;
    if (!cycle->init)
        return res;
    // clang-format on
    cycle->psubSlot = 0;
    cycle->subSlot = 0;
    cycle->sSlot = 0;
    cycle->actSlot = 0;
    cycle->lSlot = 0;
    return EM_OK;
};


bool cycle_sync_ok(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return false;
    if (!cycle->init) return false;
    // clang-format on
    res = (cycle->sync_state == SYNCHRONIZE) || (cycle->sync_state == SYNCHRONIZE_READY)  ||
          (cycle->sync_state == SYNCHRONIZE_ERROR) ||(cycle->sync_state == SYNCHRONIZE_LOCKED);
    return res;
}

system_state_e cycle_get_state(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return SYNC_NA;
    if (!cycle->init) return SYNC_NA;
    // clang-format on
#ifndef UNIT_TEST
    __disable_irq();
#endif
    system_state_e s = cycle->sync_state;
#ifndef UNIT_TEST
    __enable_irq();
#endif
    return s;
}


em_msg cycle_set_state(cycle_t *cycle, system_state_e state) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if ((state >= SYNC_CNT) ||(state<SYNC_RESET)) return res;
    // clang-format on
    cycle->psync_state = state;
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

em_msg cycle_doSend(cycle_t *cycle) {
    bool res = false;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    int8_t actSlot = CYCLE_ACT_SLOT(cycle);
    int8_t subSlot = CYCLE_ACT_SUB_SLOT(cycle);
    if (cycle->slot == SLOT_NOT_SET) {
        res = true;
    } else {
        res = (actSlot == cycle->slot - 1) && ((CYCLE_SUB_SLOT_CNT - subSlot) < abs(cycle->tx_ss));
    }
#if OPTION_VERBOSE == 1
    if (res) {
        cycle_text_print(cycle, "Ready to send");
    }
#endif
    return res ? EM_OK : EM_ERR;
};

int8_t cycle_check_slot(int8_t slot) {
    if (((slot > 0) && (slot <= CYCLE_SLOT_CNT)) && (slot % 2 == 1)) {
        return slot;
    }
    return -1;
}

em_msg cycle_set_idx(cycle_t *cycle, uint8_t idx) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (!(idx<CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT)) return res;
    // clang-format on
    cycle->idx = idx;
    return res;
}

em_msg cycle_idx_increment(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    cycle->idx++;
    return res;
}

em_msg cycle_monce_in_cycle(cycle_t *cycle){
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    if(cycle_role(cycle)== MASTER) {
        res = (cycle->idx == CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT-1)?EM_OK:EM_ERR;
    }
    return res;
}


em_msg cycle_set_my_slot(cycle_t *cycle, int8_t slot) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (cycle_check_slot(slot)<0) return res;
    // clang-format on
    if (cycle->slot == SLOT_NOT_SET) {
        cycle->slot = slot;
        res = EM_OK;
    }
    return res;
}

em_msg cycle_set_master(cycle_t *cycle, int8_t slot) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if (cycle_check_slot(slot)<0) return res;
    // clang-format on
    if (cycle->master == SLOT_NOT_SET) {
        cycle->master = slot;
        res = EM_OK;
    }
    return res;
}

em_msg cycle_set_slot(cycle_t *cycle, int8_t rxSlot, dev_role_e ss_type) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    if ((ss_type < SLAVE) || (ss_type>MASTER)) return res;
    // MASTER is a one-shot bootstrap claim, for a device that has not heard
    // anyone yet. A master derives the cycle from its own TX slot, so latching
    // it a second time recomputes psubSlot and resets the timer counter
    // against that same self-reference -- shifting the phase every slave has
    // synced to, for no gain. cycle->isMaster records that the claim happened
    // and is released only by cycle_reset_role(), i.e. by the master watchdog
    // in cycle_increment() -- so the role is claimable once per election, and
    // a device can be elected again after a timeout has torn the old master
    // down.
    //
    // SLAVE: the first claim (cycle->isSlave still false) takes the lock and
    // makes rxSlot the master. Every later claim from the master sets
    // psubSlot again the same way -- the slave's timer runs on its own
    // crystal and drifts (~0.5 sub-slot per minute at 12 ppm), so it has to
    // follow its master on every slot-aligned frame. Callers pass no ACKs:
    // those go out off-slot.
    //
    // The guard sits in the MASTER branch below and keys off cycle->role, not
    // ss_type, so a latched master cannot demote onto a peer it finally hears
    // -- the SLAVE claim is rejected too (SlaveClaimRejectedAfterMasterLatch).
    // Demotion is the watchdog's job alone, which keeps one path for it
    // instead of racing an RX against a timeout.
    if (cycle->role == NOT_SET){
        cycle->role = ss_type;
    }
    if (cycle_sync_ok(cycle)) {
            if (cycle->role == MASTER) {
                if (cycle->isMaster) return EM_ERR;
#ifndef UNIT_TEST
                   cycle->timerCNT = cycle->timer->Instance->CNT;
#endif
                if (cycle->role  == MASTER){
                    //cycle->timerCNT  = 0;
                    cycle_reset(cycle);
                    cycle->master    = rxSlot;
                    cycle->psubSlot  = (cycle->master * CYCLE_SUB_SLOT_CNT + CYCLE_MODULO + cycle_tx_ss(cycle)) % CYCLE_MODULO;
                    cycle->masterAge = 0;
                    cycle->slaveAge  = 0;
                    cycle->cycle     = 0;
                    cycle->isSlave   = false;
                    cycle->isMaster  = true;
                    return EM_OK;
                }
                res = EM_ERR;
            } else if (cycle->role == SLAVE)  {
                if (!cycle->isSlave){ // lock once, then track the master's phase
                    cycle_reset(cycle);
                    if (cycle->master == SLOT_NOT_SET){
                    	cycle->master   = rxSlot;
                    }
                    cycle->psubSlot = (rxSlot* CYCLE_SUB_SLOT_CNT + CYCLE_MODULO +cycle_rx_ss(cycle)) % CYCLE_MODULO;
                    cycle->masterAge= 0;
                    cycle->slaveAge = 0;
                    cycle->isSlave  = true;
                    cycle->isMaster = false;
                    return EM_OK;
                }
                if (rxSlot != cycle->master) return EM_ERR;
                cycle->psubSlot = (rxSlot* CYCLE_SUB_SLOT_CNT + CYCLE_MODULO +cycle_rx_ss(cycle)) % CYCLE_MODULO;
                cycle->masterAge= 0;
                cycle->slaveAge = 0;
                return EM_OK;
            }
#ifndef UNIT_TEST
//        __HAL_TIM_SET_COUNTER(cycle->timer, 0);
#endif

    }
    return res;
}
int8_t cycle_get_slot(cycle_t *cycle) {
    em_msg res = EM_ERR;
    // clang-format off
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    return cycle->slot;
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
    // Both watchdogs are kicked: the frame is proof the network is there,
    // whichever role we currently hold.
    cycle->masterAge = 0;
    cycle->slaveAge = 0;
    res = EM_OK;
    return res;
}

int8_t cycle_tx_ss(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    return cycle->tx_ss;
}

int8_t cycle_rx_ss(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    return cycle->rx_ss;
}

uint8_t cycle_postrx(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return 0;
    if (!cycle->init) return 0;
    // clang-format on
    return cycle->postrx;
};

int16_t cycle_difference(cycle_t *cycle, int8_t rxSlot) {
    // clang-format off
    if (!cycle) return CYCLE_DIFF_INVALID;
    if (!cycle->init) return CYCLE_DIFF_INVALID;
    // clang-format on
    // Signed sub-slot distance from the current position to rxSlot's window
    //   lower = rxSlot*CYCLE_SUB_SLOT_CNT          (first sub-slot of rxSlot)
    //   last  = lower + CYCLE_SUB_SLOT_CNT - 1     (last sub-slot of rxSlot)
    //   < 0  below the window: subSlot - lower
    //   == 0 within the window [lower, last]
    //   > 0  above the window: subSlot - last
    // The cycle is a ring of CYCLE_MODULO sub-slots, so outside the window the
    // shorter way round wins. above + below == CYCLE_MODULO - CYCLE_SUB_SLOT_CNT + 1
    // is odd, so there is never a tie, and the result lies in
    // [-(CYCLE_MODULO - CYCLE_SUB_SLOT_CNT) / 2, (CYCLE_MODULO - CYCLE_SUB_SLOT_CNT) / 2].
    // rxSlot is masked to a valid slot, so no caller can push lower off the ring.
    const int16_t lower = (int16_t)(rxSlot & CYCLE_SLOT_MASK) * CYCLE_SUB_SLOT_CNT;
    const int16_t last = lower + CYCLE_SUB_SLOT_CNT - 1;
    if ((cycle->subSlot >= lower) && (cycle->subSlot <= last)) {
        return 0; // inside the window
    }
    const int16_t above = ((cycle->subSlot - last) + CYCLE_MODULO) % CYCLE_MODULO;
    const int16_t below = ((lower - cycle->subSlot) + CYCLE_MODULO) % CYCLE_MODULO;
    return ((above < below) ? above : (int16_t)-below);
}

void cycle_sscnt_init(cycle_t *cycle, cycle_measure_e dir) {
    if (!cycle)
        return;
    if (!cycle->init)
        return;
    cycle->cntErrror[dir] = 0;
    cycle->ssCnt[dir] = 0;
    cycle->doMeasure[dir] = false;
}
/*
 *  Measure how many subslot past between cycle_sscnt_start and
 *  cycle_sscnt_stop and return the value with cycle_sscnt_get.
 *  If an overflow occured on this int8_t value cycle_sscnt_get
 *  returns EM_ERR otherwise a value >=0
 */
void cycle_sscnt_start(cycle_t *cycle, cycle_measure_e dir) {
    if (!cycle)
        return;
    if (!cycle->init)
        return;
    if (dir >= RADIO_CNT)
        return;
    cycle->doMeasure[dir] = true;
}

void cycle_sscnt_stop(cycle_t *cycle, cycle_measure_e dir) {
    if (!cycle)
        return;
    if (!cycle->init)
        return;
    cycle->doMeasure[dir] = false;
}

uint16_t cycle_sscnt_get(cycle_t *cycle, cycle_measure_e dir) {
    if (!cycle)
        return -1;
    if (!cycle->init)
        return -1;
    if (!cycle->cntErrror[dir]) {
        return cycle->ssCnt[dir];
    }
    return EM_ERR;
};

// Role watchdog, called exactly once per frame cycle from cycle_increment().
//
// Every device that holds a role ages here; cycle_master_seen() resets the age
// on every frame that proves the network is still there. Running dry means the
// master is gone (or, for a master, that nobody is left listening), so the role
// is dropped and the next frame heard elects a new master.
//
// The two roles get different budgets: a slave must notice a dead master fast
// (CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT), while a master may sit quiet much longer
// before it concludes it is alone (CYCLE_MASTER_LOOSE_CYCLE_CNT).
// (plain `static`, not the STATIC macro: common.h only defines STATIC in its
// UNIT_TEST branch, so STATIC does not compile in the firmware build.)
static void cycle_age_role(cycle_t *cycle) {
    uint16_t *age = NULL;
    uint16_t limit = 0;
    switch (cycle_role(cycle)) {
    case MASTER:
        age = &cycle->masterAge;
        limit = CYCLE_MASTER_LOOSE_CYCLE_CNT;
        break;
    case SLAVE:
        age = &cycle->slaveAge;
        limit = CYCLE_SLAVE_LOOSE_CYCLE_CNT;
        break;
    default:
        // No role to lose -- nothing ages until the next election.
        return;
    }
    // Only the age of the held role counts, once per frame cycle.
    (*age)++;
    if (*age < limit) {
        return;
    }
    // Clear both ages, not just the one that ran dry: the role is gone, so the
    // next election has to start from a clean watchdog either way.
    if (cycle->role == MASTER) {
        cycle->masterAge = 0;
    }
    cycle_reset_role(cycle);
    cycle_set_state(cycle, SYNCHRONIZE);
}

void cycle_increment(cycle_t *cycle) {
    // clang-format off
    if (!cycle) return;
    if (!cycle->init) return;
    // clang-format on
	if (cycle->psync_state > SYNC_RESET){
		cycle->sync_state = cycle->psync_state;
		cycle->psync_state = SYNC_RESET;
	}
    if (cycle->sync_state == SYNCHRONIZE) {
        cycle->sync_state = SYNCHRONIZE_READY;
    }
    if (cycle_sync_ok(cycle)) {
			if (cycle->psubSlot > 0) {
				if (cycle->role == MASTER)
					cycle->subSlot = (cycle->slot * CYCLE_SUB_SLOT_CNT)-1;
				else if (cycle->role == SLAVE) {
					cycle->subSlot = cycle->psubSlot;
				}
				else {
					assert(1);
				}
				cycle->psubSlot = 0;
#ifndef UNIT_TEST
				stateled_dtoggle_pin(cycle->hdl, cycle_upd);
#endif
            }
			cycle->subSlot++;
			uint16_t s = cycle->subSlot;
			s =   s% (CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT);
			cycle->subSlot = s;
			cycle->actSlot = CYCLE_ACT_SLOT(cycle);
			cycle->sSlot   = CYCLE_ACT_SUB_SLOT(cycle);
#if OPTION_SHOW_TIMING == 1
           //stateled_set(cycle->hdl, cycle->sSlot);
	       stateled_toggle_pin(cycle->hdl, ss_toggle);
#endif
            if (cycle->doMeasure[RADIO_RX]) {
                int8_t lss = cycle->ssCnt[RADIO_RX];
                cycle->ssCnt[RADIO_RX]++;
                if (cycle->ssCnt[RADIO_RX] < lss) {
                    cycle->cntErrror[RADIO_RX] = true;
                }
            }
            if (cycle->doMeasure[RADIO_TX]) {
                int8_t lss = cycle->ssCnt[RADIO_TX];
                cycle->ssCnt[RADIO_TX]++;
                if (cycle->ssCnt[RADIO_TX] < lss) {
                    cycle->cntErrror[RADIO_RX]  = true;
                }
            }
	       if (cycle->actSlot != cycle->lSlot) {
#if OPTION_SHOW_TIMING == 1
	           stateled_set(cycle->hdl, cycle->actSlot);
	    	   stateled_toggle_pin(cycle->hdl, slot_toggle);
#endif
	    	 cycle->lSlot = cycle->actSlot;
             if ((cycle->actSlot == 0) && (cycle_sync_ok(cycle))){
                 cycle->idx = 0;
#if OPTION_SHOW_TIMING == 1
                stateled_toggle_pin(cycle->hdl, cycle_toggle);
#endif
                cycle->cycle += 1;
                cycle->set = true;
                cycle_age_role(cycle);
              }
	      }
     }
}

bool cycle_ask_set(cycle_t *cycle) {
    bool res = false;
    // clang-format offIch
    if (!cycle) return res;
    if (!cycle->init) return res;
    // clang-format on
    bool state = cycle->set;
    if (state) {
        cycle->set = false;
        return state;
    }
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
    printf("master age = %d" NL, cycle->masterAge);
    return EM_OK;
}
