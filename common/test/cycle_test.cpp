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

// Latches a fresh cycle_t as SLAVE of `masterSlot` through the election path.
static void elect(cycle_t *c, int8_t masterSlot) {
    ASSERT_EQ(cycle_master_seen(c, masterSlot), EM_OK);
    ASSERT_EQ(c->role, SLAVE);
    ASSERT_EQ(c->master, masterSlot);
}

class CycleTest : public ::testing::Test {
  protected:
    cycle_t cycle;
    TIM_HandleTypeDef timerPtr;
    void SetUp() override {}
    void TearDown() {}
};

// ---------------------------------------------------------------------------
// cycle_init / cycle_reset
// ---------------------------------------------------------------------------
TEST_F(CycleTest, NullNullPtrReturnsError) {
    EXPECT_EQ(cycle_init(nullptr, TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, nullptr), EM_ERR);
}
TEST_F(CycleTest, NullValidPtrReturnsError) {
    EXPECT_EQ(cycle_init(nullptr, TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_ERR);
}
TEST_F(CycleTest, ValidNullPtrReturnsError) {
    EXPECT_EQ(cycle_init(&cycle, TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, nullptr), EM_ERR);
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
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
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
    EXPECT_EQ(cycle_get_state(&c), SYNCHRONIZE);

    ASSERT_EQ(cycle_set_slot(&c, my_slot, SLAVE), EM_OK);

    EXPECT_STREQ(cycle_role_str(&c), "SLAVE ");
    uint16_t exp = (my_slot * CYCLE_SUB_SLOT_CNT + CYCLE_MODULO + RX_SS) % CYCLE_MODULO;
    EXPECT_EQ(c.psubSlot, exp) ;
    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(c.slot, my_slot);
    EXPECT_EQ(c.master, -1);
    EXPECT_EQ(c.isMaster, false);
    EXPECT_EQ(c.isSlave,  true);
    EXPECT_EQ(c.subSlot, 0);
    EXPECT_EQ(c.slaveAge, 0);
    EXPECT_EQ(c.masterAge, 0);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE);

    cycle_increment(&c);

    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);
    EXPECT_EQ(c.subSlot,exp+1);
    EXPECT_EQ(c.psubSlot, 0) ;

    EXPECT_EQ(c.slot, my_slot);
    EXPECT_EQ(c.isMaster, false);
    EXPECT_EQ(c.isSlave, true);
    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(cycle_get_state(&c), SYNCHRONIZE_READY);

    ASSERT_EQ(cycle_set_slot(&c, (my_slot - 1), MASTER), EM_ERR);
}
TEST_F(CycleTest, SetSlotMasterRole) {
    cycle_t c{0};
    ASSERT_EQ(cycle_check_slot(my_slot), my_slot);
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    EXPECT_EQ(c.init, true);
    EXPECT_EQ(c.psubSlot, 0);
    EXPECT_EQ(c.subSlot, 0);
    ASSERT_EQ(cycle_set_state(&c, (system_state_e)-1), EM_ERR);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    EXPECT_EQ(cycle_get_state(&c), SYNCHRONIZE);
    EXPECT_EQ(c.psubSlot, 0);
    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_EQ(c.subSlot, 0);

    ASSERT_EQ(cycle_set_slot(&c, (my_slot), MASTER), EM_OK);
    EXPECT_STREQ(cycle_role_str(&c), "MASTER");
    EXPECT_EQ(c.role, MASTER);
    EXPECT_EQ(c.slot, (my_slot));
    EXPECT_EQ(c.master, (my_slot));

    ASSERT_EQ(cycle_set_slot(&c, (my_slot), SLAVE), EM_ERR);

    EXPECT_EQ(c.isMaster, true);
    EXPECT_EQ(c.isSlave, false);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE);
    EXPECT_EQ(c.subSlot, 0);
    EXPECT_NE(c.psubSlot, (my_slot)*CYCLE_SUB_SLOT_CNT + RX_SS);
    EXPECT_EQ(c.psubSlot, (my_slot)*CYCLE_SUB_SLOT_CNT + TX_SS);
    EXPECT_EQ(cycle_get_state(&c), SYNCHRONIZE);

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
    uint16_t master = 1;
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, master, SLAVE), EM_OK) << "slot=" << (int)master;
    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(c.psubSlot, master * CYCLE_SUB_SLOT_CNT + RX_SS) << "slot=" << (int)master;

    cycle_increment(&c);

    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);
    EXPECT_EQ(c.psubSlot, 0);

    EXPECT_EQ(c.subSlot, master * CYCLE_SUB_SLOT_CNT + RX_SS + 1);

    // Nothing kicks the watchdog in this loop, so the slave role ages out after
    // CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT frame cycles. The sub-slot keeps
    // free-running across that -- losing the role must not stall the timebase.
    bool dropped = false;
    for (int16_t tick = 0; tick < (CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT + 1) * CYCLE_MODULO; tick++) {
        EXPECT_EQ(c.subSlot, (master * CYCLE_SUB_SLOT_CNT + RX_SS + 1 + tick) % CYCLE_MODULO)
            << "c.subSlot= " << (c.subSlot) << " tick= " << tick;
        cycle_increment(&c);
        EXPECT_EQ(c.subSlot, (master * CYCLE_SUB_SLOT_CNT + RX_SS + 2 + tick) % CYCLE_MODULO)
            << "EXP= " << master * CYCLE_SUB_SLOT_CNT + RX_SS + 2 + tick << " tick= " << tick;
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
// cycle_reset_role() is the one "drop the role" primitive, and it releases the
// latch with the role: the claim is one-shot per election, not per cycle_init.
// Without this the network would die at the first timeout -- no device could
// ever be elected master a second time.
#if 0
TEST_F(CycleTest, ResetRoleReleasesMasterLatch) {
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, my_slot, MASTER), EM_OK);
    ASSERT_TRUE(c.isMaster);
    ASSERT_EQ(c.master, my_slot);

    cycle_reset_role(&c);
    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_FALSE(c.isMaster) << "the latch must go with the role, or re-election is impossible";
}
// ---------------------------------------------------------------------------
// Master watchdog and re-election.
//
// A role survives CYCLE_MASTER_LOOSE_CYCLE_CNT (CYCLE_MASTER_LOOSE *
// CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT == 24) frame cycles without proof that the network
// is still there. cycle_master_seen() supplies that proof from the RX path and
// elects a master when there is none; cycle_increment() ages the counter at the
// frame-cycle boundary and drops the role via cycle_reset_role() when it runs
// dry. Together: a master that goes quiet is torn down everywhere, and the next
// device heard becomes the new master.
// ---------------------------------------------------------------------------
// A master with nobody left to hear tears itself down -- exactly at the
// timeout, not before. Without this the slot stays occupied by a device the
// others can no longer reach.
TEST_F(CycleTest, MasterAgesOutWhenAlone) {
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, my_slot, MASTER), EM_OK);
    ASSERT_EQ(c.masterAge, 0);
    EXPECT_TRUE(c.isMaster);
    ASSERT_EQ(c.master, my_slot);
    ASSERT_EQ(c.slot, my_slot);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE);
    // masterAge is a *frame cycle* counter, so it moves once per CYCLE_MODULO
    // cycle_increment() ticks -- hence advance_frame_cycles() and not a bare
    // cycle_increment() loop here.
    for (uint16_t i = 1; i < CYCLE_MASTER_LOOSE_CYCLE_CNT; i++) {
        advance_frame_cycles(&c, 1);
        ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY) << "frame cycle " << i;
        ASSERT_EQ(c.role, MASTER) << "frame cycle " << i;
        ASSERT_EQ(c.masterAge, i) << "frame cycle " << i;
    }
    EXPECT_EQ(c.role, MASTER) << "the role has to hold right up to the timeout";
    EXPECT_EQ(c.masterAge, CYCLE_MASTER_LOOSE_CYCLE_CNT - 1);

    advance_frame_cycles(&c, 1);
    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_FALSE(c.isMaster);
    EXPECT_EQ(c.masterAge, 0);
}

// The normal case: a slave that keeps hearing its master never ages out, no
// matter how long the link stays up.
TEST_F(CycleTest, MasterSeenKeepsSlaveAlive) {
    const int8_t masterSlot = 1;
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    elect(&c, masterSlot);

    for (int i = 0; i < 4 * CYCLE_MASTER_LOOSE_CYCLE_CNT; i++) {
        advance_frame_cycles(&c, 1);
        ASSERT_EQ(cycle_master_seen(&c, masterSlot), EM_OK) << "frame cycle " << i;
        ASSERT_EQ(c.role, SLAVE) << "frame cycle " << i;
        ASSERT_EQ(c.masterAge, 0) << "frame cycle " << i;
    }
    EXPECT_EQ(c.master, masterSlot);
}

// A slave that stops hearing its master gives the role up after
// CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT frame cycles -- exactly then, not before --
// and falls back to SYNCHRONIZE so the next frame heard can elect a new master.
TEST_F(CycleTest, SlaveAgesOutWithoutMaster) {
    const int8_t masterSlot = 1;
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    elect(&c, masterSlot);
    ASSERT_EQ(c.slaveAge, 0);

    for (uint16_t i = 1; i < CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT; i++) {
        advance_frame_cycles(&c, 1);
        ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY) << "frame cycle " << i;
        ASSERT_EQ(c.role, SLAVE) << "frame cycle " << i;
        ASSERT_EQ(c.slaveAge, i) << "frame cycle " << i;
    }
    EXPECT_EQ(c.role, SLAVE) << "the role has to hold right up to the timeout";

    advance_frame_cycles(&c, 1);
    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_FALSE(c.isSlave);
    EXPECT_EQ(c.slaveAge, 0);
    EXPECT_EQ(c.masterAge, 0);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE);
}

// A device with no role does not age: nothing to lose, and the watchdog must
// not run away while it waits for the first frame.
TEST_F(CycleTest, RolelessCycleDoesNotAge) {
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(c.role, NOT_SET);

    advance_frame_cycles(&c, 2 * CYCLE_MASTER_LOOSE_CYCLE_CNT);
    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_EQ(c.masterAge, 0);
    EXPECT_EQ(c.slaveAge, 0);
}

// The point of the whole mechanism: once the old master has aged out, the first
// device heard becomes the new one and the cycle re-latches onto its slot.
TEST_F(CycleTest, FirstFrameAfterTimeoutElectsSender) {
    const int8_t oldMaster = 1;
    const int8_t newMaster = 5;
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    elect(&c, oldMaster);

    // As a SLAVE it is the slave budget that runs out, not the master one.
    advance_frame_cycles(&c, CYCLE_SLAVE_KEEP_ALIVE_CYCLE_CNT);
    ASSERT_EQ(c.role, NOT_SET) << "the old master has to be gone first";

    EXPECT_EQ(cycle_master_seen(&c, newMaster), EM_OK);
    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(c.master, newMaster);
    EXPECT_EQ(c.masterAge, 0);
    EXPECT_EQ(c.psubSlot, newMaster * CYCLE_SUB_SLOT_CNT + RX_SS) << "timing must re-latch onto the new master";
}

// Traffic from anyone else is not proof the master is alive: a slave in a
// partition that lost the master must still time out, however busy the channel.
TEST_F(CycleTest, SlaveIgnoresFramesFromOtherSlots) {
    const int8_t masterSlot = 1;
    const int8_t otherSlot = 5;
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    elect(&c, masterSlot);

    for (int i = 0; i < CYCLE_MASTER_LOOSE_CYCLE_CNT - 1; i++) {
        advance_frame_cycles(&c, 1);
        ASSERT_EQ(cycle_master_seen(&c, otherSlot), EM_ERR) << "frame cycle " << i;
        ASSERT_EQ(c.role, SLAVE) << "frame cycle " << i;
    }
    EXPECT_EQ(c.masterAge, CYCLE_MASTER_LOOSE_CYCLE_CNT - 1) << "the noise must not have kicked the watchdog";

    advance_frame_cycles(&c, 1);
    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_EQ(c.master, -1);
}

// A master hears everyone but itself, so for it any frame is proof the network
// is still there. Requiring its own slot would time out a healthy master every
// 24 cycles and churn the election forever.
TEST_F(CycleTest, MasterWatchdogAcceptsAnyFrame) {
    const int8_t otherSlot = 5;
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, my_slot, MASTER), EM_OK);
    ASSERT_NE(otherSlot, c.master);

    for (int i = 0; i < 2 * CYCLE_MASTER_LOOSE_CYCLE_CNT; i++) {
        advance_frame_cycles(&c, 1);
        ASSERT_EQ(cycle_master_seen(&c, otherSlot), EM_OK) << "frame cycle " << i;
        ASSERT_EQ(c.role, MASTER) << "frame cycle " << i;
    }
    EXPECT_TRUE(c.isMaster);
    EXPECT_EQ(c.master, my_slot) << "hearing a slave must not move the master slot";
}

// The other half of re-election: a device that timed out as MASTER may claim
// the role again. The isMaster latch is per election, not per cycle_init.
TEST_F(CycleTest, TimedOutMasterCanClaimAgain) {
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, my_slot, MASTER), EM_OK);

    advance_frame_cycles(&c, CYCLE_MASTER_LOOSE_CYCLE_CNT);
    ASSERT_EQ(c.role, NOT_SET);

    EXPECT_EQ(cycle_set_slot(&c, my_slot, MASTER), EM_OK);
    EXPECT_EQ(c.role, MASTER);
    EXPECT_TRUE(c.isMaster);
    EXPECT_EQ(c.master, my_slot);
    EXPECT_EQ(c.masterAge, 0) << "the fresh claim restarts the watchdog";
}
#endif

// ---------------------------------------------------------------------------
// Only SLAVE and MASTER are roles a caller may ask for. A rejected role must
// not latch the cycle -- a valid call afterwards still has to succeed.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, SetSlotRejectsInvalidRole) {
    for (dev_role_e role : {NOT_SET, SS_CNT}) {
        cycle_t c{0};
        EXPECT_EQ(c.init, false);
        ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
        EXPECT_EQ(c.init, true);
        ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
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
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    const int8_t claimed = c.subSlot;

    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_LOCKED), EM_OK);
    EXPECT_EQ(cycle_set_slot(&c, 5, SLAVE), EM_ERR);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_LOCKED);
    EXPECT_EQ(c.subSlot, claimed);
    EXPECT_STREQ(cycle_role_str(&c), "SLAVE "); // still the originally claimed role
}


TEST_F(CycleTest, ResetSlaveRole) {
    cycle_t c{0};
    int8_t slot = 1;
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(c.subSlot, 0 );
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
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
    ASSERT_EQ(c.slot, SLOT_NOT_SET);
    ASSERT_EQ(c.master, SLOT_NOT_SET);
    ASSERT_EQ(c.sync_state, SYNCHRONIZE);
    ASSERT_EQ(c.sSlot, 0);
    ASSERT_EQ(c.lSlot, SLOT_NOT_SET);
    ASSERT_EQ(c.actSlot , 0);
    ASSERT_EQ(c.cycle,     0);
    ASSERT_EQ(c.slaveAge , 0);
    ASSERT_EQ(c.masterAge, 0);
}

TEST_F(CycleTest, ResetMasterRole) {
    cycle_t c{0};
    int8_t slot = 1;
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(c.subSlot, 0 );

    ASSERT_EQ(cycle_set_slot(&c, slot, MASTER), EM_OK);
    ASSERT_EQ(c.psubSlot, slot * CYCLE_SUB_SLOT_CNT + TX_SS );
    ASSERT_EQ(c.cycle, 0);
    ASSERT_EQ(c.isMaster, true);
    ASSERT_EQ(c.isSlave , false);
    ASSERT_EQ(c.slaveAge , 0);
    ASSERT_EQ(c.slot     , slot);
    ASSERT_EQ(c.master   , slot);

    cycle_reset(&c);

    ASSERT_EQ(c.subSlot, 0 );
    ASSERT_EQ(c.psubSlot, 0);
    ASSERT_EQ(c.slot, SLOT_NOT_SET);
    ASSERT_EQ(c.master, SLOT_NOT_SET);
    ASSERT_EQ(c.sSlot, cycle_act_sub_slot(&c));
    ASSERT_EQ(c.lSlot, SLOT_NOT_SET);
    ASSERT_EQ(c.actSlot , cycle_act_slot(&c));
    ASSERT_EQ(c.cycle, 0);
    ASSERT_EQ(c.slaveAge, 0);
    ASSERT_EQ(c.masterAge, 0);
}

TEST_F(CycleTest, DISABLED_CheckCycleSlaveIncrement) {
    cycle_t c{0};
    uint32_t cycle;
    int8_t slot = 1;
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_READY), EM_ERR);

    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_slot(nullptr, 1, SLAVE), EM_ERR);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    // Elect through the RX path rather than cycle_set_slot(): only
    // cycle_master_seen() records c.master, and the keep-alive below needs it.
    elect(&c, slot);
    ASSERT_EQ(c.psubSlot, slot * CYCLE_SUB_SLOT_CNT + RX_SS);
    ASSERT_EQ(c.subSlot, 0);

    uint8_t i;
    for (i = 0; i < (my_slot - slot) * CYCLE_SUB_SLOT_CNT; i++) {
        cycle_increment(&c);
        ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT + RX_SS + 1 + i);
    }
    ASSERT_EQ(i, 2 * CYCLE_SUB_SLOT_CNT);
    ASSERT_EQ(c.actSlot, 3);
    ASSERT_EQ(c.subSlot, 3 * CYCLE_SUB_SLOT_CNT + RX_SS);
    ASSERT_EQ(c.subSlot, (slot * CYCLE_SUB_SLOT_CNT + RX_SS + i ))  << "NOP";
    ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY);
    ASSERT_EQ(c.psubSlot, 0);
    ASSERT_EQ(c.lSlot,  my_slot);

    cycle_increment(&c);
    ASSERT_EQ(c.subSlot, 3*slot * CYCLE_SUB_SLOT_CNT + RX_SS+1);
    cycle_reset_subslot(&c);
    ASSERT_EQ(c.subSlot, 0);
    ASSERT_EQ(c.masterAge, 0);
    for (cycle = 0; cycle <= UINT16_MAX; cycle++) {
        for (uint16_t ss = 0; ss <  CYCLE_MODULO; ss++) {
            ASSERT_EQ(c.subSlot, ss);
            ASSERT_EQ(c.actSlot, c.lSlot);
            ASSERT_EQ(c.cycle,   cycle)<< " (ss+1) = " << (uint16_t)(ss+1) << " actSlot = "<<(uint32_t)cycle_act_slot(&c) << " cycle = " << cycle;
            cycle_increment(&c);
            ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY)  << " (ss+1) = " << (uint16_t)(ss+1) << " actSlot = "<< (uint32_t)cycle_act_slot(&c) << " cycle = " << cycle;
            ASSERT_EQ(c.subSlot, (ss+1)%CYCLE_MODULO)   << " (ss+1) = " << (uint16_t)(ss+1) << " actSlot = "<< (uint32_t)cycle_act_slot(&c) << " cycle = " << cycle;
            // Expected values are computed independently of the CYCLE_ACT_* macros.
            ASSERT_EQ(c.actSlot, ((ss+1)%CYCLE_MODULO) / CYCLE_SUB_SLOT_CNT)<< " (ss+1) = " << (uint16_t)(ss+1) << " actSlot = "<< (uint32_t)cycle_act_slot(&c)  << " cycle = " << cycle;
            ASSERT_EQ(c.sSlot, (ss+1) % CYCLE_SUB_SLOT_CNT)<< " (ss+1) = " << (uint16_t)(ss+1) << " sSlot = "<< (uint32_t)c.sSlot  << " cycle = " << cycle;
        }
        // The wrap at the end of the inner loop aged the role by one frame
        // cycle; a frame from the master resets it. Without that kick the
        // watchdog would drop the role (see SlaveAgesOutWithoutMaster).
        // Only the slave age advances, once per CYCLE_MODULO; masterAge stays 0.
        ASSERT_EQ(c.slaveAge, 1) << " cycle = " << cycle;
        ASSERT_EQ(c.masterAge, 0) << " cycle = " << cycle;
        ASSERT_EQ(cycle_master_seen(&c, slot), EM_OK) << " cycle = " << cycle;
        ASSERT_EQ(c.slaveAge, 0) << " cycle = " << cycle;
        ASSERT_EQ(c.masterAge, 0) << " cycle = " << cycle;
        ASSERT_EQ(c.role, SLAVE) << " cycle = " << cycle;
    }
    // The last wrap already drove cycle past 65535 and overflowed it back to 0,
    // and left subSlot at 0. One more tick only advances the sub-slot.
    ASSERT_EQ(c.cycle, 0);
    ASSERT_EQ(c.subSlot, 0);
    cycle_increment(&c);

    ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot, 1);
    ASSERT_EQ(c.actSlot, 0);
    ASSERT_EQ(c.sSlot, 1);
    ASSERT_EQ(c.cycle, 0);
}

TEST_F(CycleTest, DISABLED_CheckCycleMasterIncrement) {
    cycle_t c{0};
    uint32_t cycle;
    int8_t slot = 1;
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_READY), EM_ERR);

    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_slot(nullptr, 1, MASTER), EM_ERR);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, MASTER), EM_OK);
    ASSERT_EQ(c.psubSlot, slot * CYCLE_SUB_SLOT_CNT + TX_SS);
    ASSERT_EQ(c.subSlot, 0);
    ASSERT_EQ(cycle_get_state(&c), SYNCHRONIZE);
    cycle_increment(&c);
    ASSERT_EQ(cycle_get_state(&c), SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT );
    ASSERT_EQ(c.psubSlot, 0);

    uint8_t i;
    for (i = 0; i < (my_slot - slot) * CYCLE_SUB_SLOT_CNT; i++) {
        cycle_increment(&c);
        ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT  + 1 + i);
    }
    ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT  + i  ) << "NOP" << i;

    ASSERT_EQ(c.psubSlot, 0);

    cycle_reset_role(&c);
    ASSERT_EQ(c.isSlave, false);
    ASSERT_EQ(c.role, MASTER);

    cycle_reset(&c);

    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT  + i);
    ASSERT_EQ(c.psubSlot, 0);
    ASSERT_EQ(c.sSlot, cycle_act_sub_slot(&c));
    ASSERT_EQ(c.lSlot, SLOT_NOT_SET);
    ASSERT_EQ(c.actSlot , 0 );
    ASSERT_EQ(c.master, SLOT_NOT_SET);
    ASSERT_EQ(c.cycle,   0);
    ASSERT_EQ(c.slaveAge, 0);
    ASSERT_EQ(c.masterAge, 0);

    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_READY), EM_OK);
    ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY);
    // cycle_reset() leaves lSlot at SLOT_NOT_SET; cycle_reset_subslot() brings
    // subSlot, actSlot and lSlot back in line, else the first tick counts as a wrap.
    cycle_reset_subslot(&c);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_READY), EM_OK);
    ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY);
    for (cycle = 0; cycle <= UINT16_MAX; cycle++) {
        for (uint16_t ss = 0; ss <  CYCLE_MODULO; ss++) {
            ASSERT_EQ(c.subSlot, ss);
            ASSERT_EQ(c.cycle,   cycle)<< " (ss+1) = " << (uint16_t)(ss+1) << " actSlot = "<<(uint32_t)cycle_act_slot(&c) << " cycle = " << cycle;
            cycle_increment(&c);
            ASSERT_EQ(c.actSlot, c.lSlot);
            ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY)  << " (ss+1) = " << (uint16_t)(ss+1) << " actSlot = "<< (uint32_t)cycle_act_slot(&c) << " cycle = " << cycle;
            ASSERT_EQ(c.subSlot, (ss+1)%CYCLE_MODULO)   << " (ss+1) = " << (uint16_t)(ss+1) << " actSlot = "<< (uint32_t)cycle_act_slot(&c) << " cycle = " << cycle;
            // Expected values are computed independently of the CYCLE_ACT_* macros.
            ASSERT_EQ(c.actSlot, ((ss+1)%CYCLE_MODULO) / CYCLE_SUB_SLOT_CNT)<< " (ss+1) = " << (uint16_t)(ss+1) << " actSlot = "<< (uint32_t)cycle_act_slot(&c)  << " cycle = " << cycle;
            ASSERT_EQ(c.sSlot, (ss+1) % CYCLE_SUB_SLOT_CNT)<< " (ss+1) = " << (uint16_t)(ss+1) << " sSlot = "<< (uint32_t)c.sSlot  << " cycle = " << cycle;
        }
        // The wrap at the end of the inner loop aged the role by one frame
        // cycle; any valid frame resets it. Without that kick the watchdog
        // would drop the role after CYCLE_MASTER_LOOSE_CYCLE_CNT cycles.
        ASSERT_EQ(c.masterAge, 1) << " cycle = " << cycle;
        ASSERT_EQ(c.slaveAge, 0) << " cycle = " << cycle;
        ASSERT_EQ(cycle_master_seen(&c, slot), EM_OK) << " cycle = " << cycle;
        ASSERT_EQ(c.masterAge, 0) << " cycle = " << cycle;
        ASSERT_EQ(c.role, MASTER) << " cycle = " << cycle;
    }
    // The last wrap already drove cycle past 65535 and overflowed it back to 0,
    // and left subSlot at 0. One more tick only advances the sub-slot.
    ASSERT_EQ(c.cycle, 0);
    ASSERT_EQ(c.subSlot, 0);
    cycle_increment(&c);

    ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot, 1);
    ASSERT_EQ(c.actSlot, 0);
    ASSERT_EQ(c.sSlot, 1);
    ASSERT_EQ(c.cycle, 0);
}

TEST_F(CycleTest, CheckSlotAsSlave) {
    cycle_t c{0};
    const int8_t dslot = 1;

    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
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
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
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
        ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
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
#if 0

// rxSlot is masked, so values outside 0..CYCLE_SLOT_CNT-1 alias onto a real
// slot instead of walking off the ring.
TEST_F(CycleTest, CycleDifferenceMasksRxSlot) {
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
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
    ASSERT_EQ(cycle_init(&c,  TX_SS, RX_SS, POSTRX, CYCLE_MASTER_KEEP_ALIVE_CYCLE_CNT, &timerPtr), EM_OK);
}
#endif
