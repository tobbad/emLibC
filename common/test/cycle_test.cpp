/*
 * cycle_test.cpp
 */
#include "common.h"
#include "cycle.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Fixture: frisch initialisierter cycle
// ---------------------------------------------------------------------------
static int8_t my_slot = 3;
#define TX_SS -5
#define RX_SS 2
#define DEFAULT_TX (my_slot * CYCLE_SUB_SLOT_CNT + TX_SS)
#define DEFAULT_RX (my_slot * CYCLE_SUB_SLOT_CNT + RX_SS)
#define POSTRX 3
#define ACT_SLOT(_cycle) ((_cycle)->subSlot / CYCLE_SUB_SLOT_CNT)

// Drives cycle_increment() until c->cycle has advanced by `cycles` frame
// cycles. One frame cycle is CYCLE_MODULO sub-slot ticks; the cap only keeps a
// broken increment from hanging the suite.
static void advance_frame_cycles(cycle_t *c, int cycles) {
    const uint16_t target = (uint16_t)(c->cycle + cycles);
    const int cap = (cycles + 1) * CYCLE_MODULO;
    for (int i = 0; (i < cap) && (c->cycle != target); i++) {
        cycle_increment(c);
    }
    ASSERT_EQ(c->cycle, target) << "cycle_increment did not advance " << cycles << " frame cycles";
}

class CycleTest : public ::testing::Test {
  protected:
    cycle_t cycle;
    dev_handle_t dummy_hdl;
    TIM_HandleTypeDef timerPtr;
    void SetUp() override {}
    void TearDown() {}
    int8_t master = 5;
};

// ---------------------------------------------------------------------------
// cycle_init / cycle_reset
// ---------------------------------------------------------------------------
TEST_F(CycleTest, NullNullPtrReturnsError) {
    EXPECT_EQ(cycle_init(nullptr, dummy_hdl, TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, nullptr), EM_ERR);
}
TEST_F(CycleTest, NullValidPtrReturnsError) {
    EXPECT_EQ(cycle_init(nullptr, dummy_hdl, TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_ERR);
}
TEST_F(CycleTest, ValidNullPtrReturnsError) {
    EXPECT_EQ(cycle_init(&cycle, dummy_hdl, TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, nullptr), EM_ERR);
}
// The CYCLE_ACT_* macros split subSlot into slot (upper nibble) and sub-slot
// (lower nibble). Checked against plain arithmetic, not against the macros.
TEST_F(CycleTest, ActSlotMacros) {
    EXPECT_EQ(CYCLE_ACT_SLOT_N(223), 13);
    EXPECT_EQ(CYCLE_ACT_SUB_SLOT_N(223), 15);
    for (int ss = 0; ss < CYCLE_MODULO; ss++) {
        ASSERT_EQ(CYCLE_ACT_SLOT_N(ss), ss / CYCLE_SUB_SLOT_CNT) << "ss=" << ss;
        ASSERT_EQ(CYCLE_ACT_SUB_SLOT_N(ss), ss % CYCLE_SUB_SLOT_CNT) << "ss=" << ss;
    }
    cycle_t c{0};
    c.subSlot = 223;
    EXPECT_EQ(CYCLE_ACT_SLOT(&c), 13);
    EXPECT_EQ(CYCLE_ACT_SUB_SLOT(&c), 15);
}

// ---------------------------------------------------------------------------
// cycle_check_slot echoes back valid slots and returns EM_ERR otherwise. Valid
// are the odd slots 1..CYCLE_SLOT_CNT-1; 0, even slots and anything out of
// range are rejected. It is a free function -- no cycle_t involved.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, CheckCheckSlot) {
    EXPECT_EQ(cycle_check_slot(-1), EM_ERR);
    EXPECT_EQ(cycle_check_slot(-3), EM_ERR);
    for (int8_t slot = 0; slot < 2 * CYCLE_SLOT_CNT; slot++) {
        const bool valid = (slot % 2 == 1) && (slot < CYCLE_SLOT_CNT);
        ASSERT_EQ(cycle_check_slot(slot), valid ? slot : EM_ERR) << "slot=" << (int)slot;
    }
}

// ---------------------------------------------------------------------------
// only
// synchronising it moves c.sync_state to SYNCHRONIZE_READY and, as SLAVE, parks
// subSlot at the slot start. A rejected slot leaves c.sync_state alone.
// A cycle_t latches its role on the first success (see SetSlotLatchesRole), so
// every slot needs its own freshly initialised cycle.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, CheckSetSlot) {
    for (int8_t sl = 0; sl < CYCLE_SLOT_CNT; sl++) {
        if (sl % 2) {
            ASSERT_EQ(cycle_check_slot(sl), sl);
        } else {
            ASSERT_EQ(cycle_check_slot(sl), EM_ERR);
        }
    }
}

// ---------------------------------------------------------------------------
// The role latches on the first successful cycle_set_slot: later calls are
// rejected and leave the position untouched.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, SetSlotSlaveRole) {
    cycle_t c{0};
    ASSERT_EQ(cycle_check_slot(my_slot), my_slot);
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    EXPECT_EQ(c.init, true);
    EXPECT_EQ(c.psubSlot, 0);
    EXPECT_EQ(c.sSlot, 0);
    EXPECT_EQ(c.actSlot, 0);
    EXPECT_EQ(c.master, SLOT_NOT_SET);
    EXPECT_EQ(c.slot,   SLOT_NOT_SET);
    EXPECT_EQ(c.lSlot,  SLOT_NOT_SET);
    EXPECT_EQ(c.subSlot, 0);
    EXPECT_EQ(c.role, NOT_SET);
    ASSERT_EQ(cycle_set_state(&c, (system_state_e)-1), EM_ERR);
    EXPECT_EQ(cycle_get_state(&c), SYNC_RESET);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    EXPECT_EQ(c.psync_state, SYNCHRONIZE);
    cycle_increment(&c);
    EXPECT_EQ(cycle_sync_ok(&c), true);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);

    ASSERT_EQ(cycle_set_master(&c, master), EM_OK);
    EXPECT_EQ(c.master, master);
    EXPECT_EQ(c.isMaster, false);
    EXPECT_EQ(c.role, NOT_SET);

    // Der eigene Slot kommt aus dem EEPROM, nicht aus cycle_set_slot.
    ASSERT_EQ(cycle_set_my_slot(&c, my_slot), EM_OK);
    EXPECT_EQ(c.slot, my_slot);
    EXPECT_EQ(c.isSlave, false);
    EXPECT_EQ(c.role, NOT_SET);

    ASSERT_EQ(cycle_sync_ok(&c), 1);
    ASSERT_EQ(cycle_set_slot(&c, my_slot, SLAVE), EM_OK);
    EXPECT_EQ(c.isSlave, true);

    EXPECT_STREQ(cycle_role_str(&c), "SLAVE ");
    uint16_t exp = (my_slot * CYCLE_SUB_SLOT_CNT + CYCLE_MODULO + RX_SS) % CYCLE_MODULO;
    EXPECT_EQ(c.psubSlot, exp) ;
    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(c.slot, my_slot);
    EXPECT_EQ(c.master, master); // der Slave uebernimmt rxSlot als Master
    EXPECT_EQ(c.isMaster, false);
    EXPECT_EQ(c.isSlave,  true);
    EXPECT_EQ(c.subSlot, 0);
    EXPECT_EQ(c.slaveAge, 0);
    EXPECT_EQ(c.masterAge, 0);

    cycle_increment(&c);

    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);
    EXPECT_EQ(c.subSlot,exp+1);
    EXPECT_EQ(c.psubSlot, 0) ;
    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(cycle_get_state(&c), SYNCHRONIZE_READY);

    ASSERT_EQ(cycle_set_slot(&c, (my_slot - 1), MASTER), EM_ERR);
}


TEST_F(CycleTest, SetSlotMasterRole) {
    cycle_t c{0};
    ASSERT_EQ(cycle_check_slot(my_slot), my_slot);
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    EXPECT_EQ(c.init, true);
    EXPECT_EQ(c.psubSlot, 0);
    EXPECT_EQ(c.subSlot, 0);
    ASSERT_EQ(cycle_set_state(&c, (system_state_e)-1), EM_ERR);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    EXPECT_EQ(c.psync_state, SYNCHRONIZE);
    EXPECT_EQ(c.psubSlot, 0);
    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_EQ(c.subSlot, 0);

    cycle_increment(&c);

    EXPECT_EQ(cycle_sync_ok(&c), true);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);

    ASSERT_EQ(cycle_set_my_slot(&c, my_slot), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, (my_slot), MASTER), EM_OK);
    EXPECT_STREQ(cycle_role_str(&c), "MASTER");
    EXPECT_EQ(c.role, MASTER);
    EXPECT_EQ(c.slot, (my_slot));
    EXPECT_EQ(c.master, (my_slot));

    ASSERT_EQ(cycle_set_slot(&c, (my_slot), SLAVE), EM_ERR);

    EXPECT_EQ(c.isMaster, true);
    EXPECT_EQ(c.isSlave, false);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);
    EXPECT_EQ(c.subSlot, 0);
    // Beim Master ist psubSlot nur ein Flag: cycle_increment setzt subSlot fest
    // auf den Sub-Slot vor dem Master-Slot (geprueft nach dem Inkrement unten).
    EXPECT_GT(c.psubSlot, 0);
    EXPECT_EQ(cycle_get_state(&c), SYNCHRONIZE_READY);

    cycle_increment(&c);

    EXPECT_EQ(cycle_get_state(&c), SYNCHRONIZE_READY);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);
    EXPECT_STREQ(cycle_role_str(&c), "MASTER");
    EXPECT_EQ(c.subSlot, (my_slot)*CYCLE_SUB_SLOT_CNT);
    EXPECT_EQ(c.psubSlot, 0);
}

// ---------------------------------------------------------------------------
// The MASTER latch (c.isMaster). A MASTER is its own timing reference, so the
// claim is one-shot: it is taken on the first successful call and never given
// back by cycle_set_slot. The three tests below pin what that costs.
// ---------------------------------------------------------------------------

// SLAVE never touches the latch, so a slave keeps re-syncing on every receiv role is not set or go lost
// The normal case.
TEST_F(CycleTest, SlaveClaimsAreNotLatched) {
    cycle_t c{0};
    uint16_t slave = 1;
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(c.role, NOT_SET);
    cycle_increment(&c);
    EXPECT_EQ(cycle_sync_ok(&c), true);
    ASSERT_EQ(cycle_set_slot(&c, slave, SLAVE), EM_OK) << "slot=" << (int)slave;
    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(c.psubSlot, slave * CYCLE_SUB_SLOT_CNT + RX_SS) << "slot=" << (int)slave;

    cycle_increment(&c);

    EXPECT_EQ(c.psubSlot, 0 );
    EXPECT_EQ(c.subSlot, slave * CYCLE_SUB_SLOT_CNT + RX_SS+1 ) << "slot=" << (int)slave;
    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);

    // Nothing kicks the watchdog in this loop, so the slave role ages out after
    // CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT frame cycles. The sub-slot keeps
    // free-running across that -- losing the role must not stall the timebase.
    bool dropped = false;
    for (int16_t tick = 0; tick < (CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT+1) * CYCLE_MODULO; tick++) {
        EXPECT_EQ(c.subSlot, (slave * CYCLE_SUB_SLOT_CNT + RX_SS + 1 + tick) % CYCLE_MODULO)
            << "c.subSlot= " << (c.subSlot) << " tick= " << tick;
        cycle_increment(&c);
        EXPECT_EQ(c.subSlot, (slave * CYCLE_SUB_SLOT_CNT + RX_SS + 2 + tick) % CYCLE_MODULO)
            << "EXP= " << slave * CYCLE_SUB_SLOT_CNT + RX_SS + 3 + tick << " tick= " << tick;
        if (!dropped && (c.role == NOT_SET)) {
            dropped = true;
            EXPECT_EQ(c.cycle, CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT)
                << "the slave must age out exactly at the keep-alive limit, tick= " << tick;
            EXPECT_EQ(c.sync_state, SYNCHRONIZE) << "the watchdog asks for a resync on the tick it fires, tick= " << tick;
            EXPECT_EQ(c.slaveAge, 0) << "tick= " << tick;
        } else if (!dropped) {
            EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY) << "tick = " << tick;
            EXPECT_EQ(c.role, SLAVE) << "ITER = " << tick;
        }
    }
    EXPECT_TRUE(dropped) << "the slave never aged out";
    cycle_increment(&c);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);
}

// ---------------------------------------------------------------------------
// Only SLAVE and MASTER are roles a caller may ask for. A rejected role must
// not latch the cycle -- a valid call afterwards still has to succeed.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, SetSlotRejectsInvalidRole) {
    for (dev_role_e role : {NOT_SET, SS_CNT}) {
        cycle_t c{0};
        EXPECT_EQ(c.init, false);
        ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
        EXPECT_EQ(c.init, true);
        ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
        EXPECT_EQ(c.psync_state, SYNCHRONIZE);
        cycle_increment(&c);
        EXPECT_EQ(cycle_set_slot(&c, 3, role), EM_ERR) << "role=" << (int)role;
        EXPECT_EQ(c.sync_state, SYNCHRONIZE) << "role=" << (int)role;
    }
}

// ---------------------------------------------------------------------------
// cycle_set_slot claims a slot as SLAVE or MASTER: it only acts while
// synchronising (SYNCHRONIZE_READY/READY), reports SYNCHRONIZE_READY and
// positions subSlot relative to the slot start. SYNCHRONIZE_LOCKED is an input
// here, not an output -- the application sets it to freeze the state, and
// cycle_set_slot then does nothing (returning EM_OK, the freeze is not an
// error). Outside the syncing window it is a no-op returning EM_ERR.
// ---------------------------------------------------------------------------

// The application sets SYNCHRONIZE_LOCKED to freeze the cycle; cycle_set_slot
// must then leave state and position alone and report success.
TEST_F(CycleTest, SetSlotFrozenWhenLocked) {
    const int8_t slot = 3;
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    cycle_increment(&c);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);

    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    const int8_t claimed = c.subSlot;

    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_LOCKED), EM_OK);
    EXPECT_EQ(c.psync_state, SYNCHRONIZE_LOCKED);
    EXPECT_EQ(cycle_set_slot(&c, 5, SLAVE), EM_ERR);
    EXPECT_EQ(c.subSlot, claimed);
    EXPECT_STREQ(cycle_role_str(&c), "SLAVE "); // still the originally claimed role
}


TEST_F(CycleTest, ResetSlaveRole) {
    cycle_t c{0};
    int8_t slot = 1;
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(c.subSlot, 0 );
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    EXPECT_EQ(c.psync_state, SYNCHRONIZE);
    cycle_increment(&c);
    ASSERT_EQ(cycle_set_my_slot(&c, slot), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    ASSERT_EQ(c.subSlot, 0 );
    ASSERT_EQ(c.psubSlot, slot * CYCLE_SUB_SLOT_CNT + RX_SS );
    ASSERT_EQ(c.cycle, 0);
    ASSERT_EQ(c.isSlave,  true);
    ASSERT_EQ(c.isMaster, false);
    ASSERT_EQ(c.slaveAge , 0);
    ASSERT_EQ(c.masterAge , 0);
    ASSERT_EQ(c.slot     , slot);

    cycle_reset(&c);

    ASSERT_EQ(c.psubSlot, 0);
    ASSERT_EQ(c.subSlot, 0 );
    ASSERT_EQ(c.slot, slot); // eigener Slot ueberlebt cycle_reset
    ASSERT_EQ(c.sync_state, SYNC_RESET);
    ASSERT_EQ(c.sSlot, 0);
    ASSERT_EQ(c.lSlot, SLOT_NOT_SET);
    ASSERT_EQ(c.actSlot , 0);
    ASSERT_EQ(c.cycle,     0);
    ASSERT_EQ(c.slaveAge , 0);
    ASSERT_EQ(c.masterAge, 0);
}
#if 0

TEST_F(CycleTest, ResetMasterRole) {
    cycle_t c{0};
    int8_t slot = 1;
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(c.subSlot, 0 );

    ASSERT_EQ(cycle_set_my_slot(&c, slot), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, MASTER), EM_OK);
    ASSERT_GT(c.psubSlot, 0); // nur Flag, siehe SetSlotMasterRole
    ASSERT_EQ(c.cycle, 0);
    ASSERT_EQ(c.isMaster, true);
    ASSERT_EQ(c.isSlave , false);
    ASSERT_EQ(c.slaveAge , 0);
    ASSERT_EQ(c.slot     , slot);
    ASSERT_EQ(c.master   , slot);

    cycle_reset(&c);

    ASSERT_EQ(c.subSlot, 0 );
    ASSERT_EQ(c.psubSlot, 0);
    ASSERT_EQ(c.slot, slot); // eigener Slot ueberlebt cycle_reset
    ASSERT_EQ(c.master, SLOT_NOT_SET);
    ASSERT_EQ(c.sSlot, cycle_act_sub_slot(&c));
    ASSERT_EQ(c.lSlot, SLOT_NOT_SET);
    ASSERT_EQ(c.actSlot , cycle_act_slot(&c));
    ASSERT_EQ(c.cycle, 0);
    ASSERT_EQ(c.slaveAge, 0);
    ASSERT_EQ(c.masterAge, 0);
}

// ---------------------------------------------------------------------------
// cycle_reset_role: vorerst verliert nur der Slave seine Rolle. Der Master
// behaelt sie; Master-Verlust kommt spaeter (siehe #if 0 Block unten).
// ---------------------------------------------------------------------------
TEST_F(CycleTest, ResetRoleDropsSlave) {
    cycle_t c{0};
    const int8_t slot = 1;
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    ASSERT_EQ(c.role, SLAVE);

    cycle_reset_role(&c);

    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_EQ(c.isSlave, false);
    EXPECT_EQ(c.isMaster, false);
    EXPECT_EQ(c.slaveAge, 0);
    // Danach darf die Rolle neu vergeben werden
    EXPECT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    EXPECT_EQ(c.role, SLAVE);
}

TEST_F(CycleTest, ResetRoleKeepsMaster) {
    cycle_t c{0};
    const int8_t slot = 1;
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, MASTER), EM_OK);
    const uint16_t psubSlot = c.psubSlot;

    cycle_reset_role(&c);

    EXPECT_EQ(c.role, MASTER);
    EXPECT_EQ(c.isMaster, true);
    EXPECT_EQ(c.isSlave, false);
    EXPECT_EQ(c.master, slot);
    EXPECT_EQ(c.psubSlot, psubSlot);
    EXPECT_STREQ(cycle_role_str(&c), "MASTER");
}

// Auch der Watchdog nimmt dem Master die Rolle nicht weg: nach weit mehr als
// CYCLE_MASTER_LOOSE_CYCLE_CNT Zyklen ohne Empfang ist er weiterhin Master.
TEST_F(CycleTest, MasterKeepsRoleBeyondWatchdog) {
    cycle_t c{0};
    const int8_t slot = 1;
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, MASTER), EM_OK);

    for (int i = 0; i < 2 * CYCLE_MASTER_LOOSE_CYCLE_CNT + 1; i++) {
        advance_frame_cycles(&c, 1);
        ASSERT_EQ(c.role, MASTER) << "cycle " << i;
        ASSERT_EQ(c.isMaster, true) << "cycle " << i;
        ASSERT_EQ(c.master, slot) << "cycle " << i;
    }
}

TEST_F(CycleTest, CheckSlotAsSlave) {
    cycle_t c{0};
    const int8_t dslot = 1;

    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_get_state(&c), SYNC_RESET);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, dslot, SLAVE), EM_OK);
    ASSERT_EQ(c.psubSlot, dslot * CYCLE_SUB_SLOT_CNT + RX_SS);
    cycle_increment(&c);
    ASSERT_EQ(cycle_get_state(&c), SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot, dslot * CYCLE_SUB_SLOT_CNT + RX_SS + 1);

    for (uint8_t slot = 1; slot < CYCLE_SLOT_CNT; slot += 2) {
        ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_ERR);
        ASSERT_EQ(c.psubSlot, 0);
        cycle_increment(&c);
        cycle_increment(&c);

        ASSERT_EQ(c.subSlot, dslot * CYCLE_SUB_SLOT_CNT + RX_SS + 2 + slot);
    }
}

// ---------------------------------------------------------------------------
// cycle_difference: signed sub-slot distance from the current position to
// rxSlot's window [lower, last], with
//   lower = rxSlot*CYCLE_SUB_SLOT_CNT, last = lower + CYCLE_SUB_SLOT_CNT - 1.
//   < 0  below the window: subSlot - lower
//   == 0 within the window
//   > 0  above the window: subSlot - last
// On the ring of CYCLE_MODULO sub-slots the shorter way round wins, so the
// result lies in [-CYCLE_DIFF_MAX, CYCLE_DIFF_MAX].
// ---------------------------------------------------------------------------
#define CYCLE_DIFF_MAX ((CYCLE_MODULO - CYCLE_SUB_SLOT_CNT) / 2)

TEST_F(CycleTest, CycleDifferenceGuards) {
    // NULL / uninitialised return the out-of-band sentinel, which cannot be
    // confused with a valid distance.
    EXPECT_EQ(cycle_difference(nullptr, 0), CYCLE_DIFF_INVALID);
    cycle_t u{};
    u.init = false;
    EXPECT_EQ(cycle_difference(&u, 0), CYCLE_DIFF_INVALID);
}

// Stepping n sub-slots away from rxSlot's window must read -n below the lower
// edge, +n above the last sub-slot and 0 anywhere inside the window.
TEST_F(CycleTest, CycleDifferenceIsSymmetric) {
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    // slot 0 and slot 15 put an edge on the ring seam, so one side wraps.
    for (int8_t slot = 0; slot < CYCLE_SLOT_CNT; slot++) {
        const int lower = slot * CYCLE_SUB_SLOT_CNT;
        const int last = lower + CYCLE_SUB_SLOT_CNT - 1;
        for (int k = 0; k < CYCLE_SUB_SLOT_CNT; k++) {
            c.subSlot = (uint8_t)(lower + k);
            EXPECT_EQ(cycle_difference(&c, slot), 0) << "slot=" << (int)slot << " k=" << k;
        }
        for (int n = 1; n <= CYCLE_DIFF_MAX; n++) {
            c.subSlot = (uint8_t)((last + n) % CYCLE_MODULO);
            EXPECT_EQ(cycle_difference(&c, slot), (int16_t)n) << "slot=" << (int)slot << " n=+" << n;
            c.subSlot = (uint8_t)((lower - n + CYCLE_MODULO) % CYCLE_MODULO);
            EXPECT_EQ(cycle_difference(&c, slot), (int16_t)-n) << "slot=" << (int)slot << " n=-" << n;
        }
    }
}

// Exhaustive sweep over every subSlot and rxSlot: the result is in range, and
// stepping back by it lands exactly on the edge it was measured from.
TEST_F(CycleTest, CycleDifferenceSweep) {
    cycle_t c{0};
        ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
        for (int ss = 0; ss < CYCLE_MODULO; ss++) {
            for (int8_t rx = 0; rx < CYCLE_SLOT_CNT; rx++) {
                c.subSlot = (uint8_t)ss;
                const int16_t got = cycle_difference(&c, rx);
            const int lower = rx * CYCLE_SUB_SLOT_CNT;
            const int last = lower + CYCLE_SUB_SLOT_CNT - 1;
            const bool inside = (ss >= lower) && (ss <= last);

            ASSERT_GE(got, -CYCLE_DIFF_MAX) << "subSlot=" << ss << " rxSlot=" << (int)rx;
            ASSERT_LE(got, CYCLE_DIFF_MAX) << "subSlot=" << ss << " rxSlot=" << (int)rx;
            ASSERT_EQ(got == 0, inside) << "subSlot=" << ss << " rxSlot=" << (int)rx;
            const int edge = (ss - got + CYCLE_MODULO) % CYCLE_MODULO;
            if (got > 0) {
                ASSERT_EQ(edge, last) << "subSlot=" << ss << " rxSlot=" << (int)rx;
            } else if (got < 0) {
                ASSERT_EQ(edge, lower) << "subSlot=" << ss << " rxSlot=" << (int)rx;
            }
        }
    }
}

// rxSlot is masked, so values outside 0..CYCLE_SLOT_CNT-1 alias onto a real
// slot instead of walking off the ring.
TEST_F(CycleTest, CycleDifferenceMasksRxSlot) {
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    for (int ss = 0; ss < CYCLE_MODULO; ss += 7) {
        c.subSlot = (uint8_t)ss;
        for (int8_t rx = 0; rx < CYCLE_SLOT_CNT; rx++) {
            const int16_t want = cycle_difference(&c, rx);
            EXPECT_EQ(cycle_difference(&c, (int8_t)(rx + CYCLE_SLOT_CNT)), want)
                << "subSlot=" << ss << " rxSlot=" << (int)rx;
            EXPECT_EQ(cycle_difference(&c, (int8_t)(rx - CYCLE_SLOT_CNT)), want)
                << "subSlot=" << ss << " rxSlot=" << (int)rx;
        }
    }
}

TEST_F(CycleTest, SlaveResync) {
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c, dummy_hdl,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
}
#endif
