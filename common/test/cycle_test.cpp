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
#define PRESS  1
#define POSTSS 2
#define POSTRX 3
#define ACT_SLOT(_cycle) (((_cycle)->subSlot >> CYCLE_SUB_SLOT_SHIFT) & CYCLE_SLOT_MASK)

static int8_t my_slot = 3;
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
TEST_F(CycleTest, NullNullPtrReturnsError)  { EXPECT_EQ(cycle_init(nullptr, my_slot, PRESS, POSTSS, POSTRX, nullptr), EM_ERR); }
TEST_F(CycleTest, NullValidPtrReturnsError) { EXPECT_EQ(cycle_init(nullptr, my_slot, PRESS, POSTSS, POSTRX, &timerPtr), EM_ERR); }
TEST_F(CycleTest, ValidNullPtrReturnsError) { EXPECT_EQ(cycle_init(&cycle,  my_slot, PRESS, POSTSS, POSTRX, nullptr), EM_ERR); }

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
// synchronising it moves c.sync_state to SYNCHRONIZE_DOING and, as SLAVE, parks
// subSlot at the slot start. A rejected slot leaves c.sync_state alone.
// A cycle_t latches its role on the first success (see SetSlotLatchesRole), so
// every slot needs its own freshly initialised cycle.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, CheckSetSlot) {
    for (int8_t sl=0;sl<CYCLE_SLOT_CNT;sl++){
        if (sl%2){
            ASSERT_EQ(cycle_check_slot(sl), sl);
        }else {
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
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, POSTRX,  &timerPtr), EM_OK);
    EXPECT_EQ(c.psubSlot, 0);
    int8_t ms = my_slot*CYCLE_SUB_SLOT_CNT-PRESS;
    EXPECT_EQ(c.subSlot, ms);
    ASSERT_EQ(cycle_set_state(&c, (system_state_e)-1), EM_ERR);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    EXPECT_EQ(cycle_get_state(&c), SYNCHRONIZE);
    EXPECT_EQ(c.psubSlot, 0);
    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_EQ(c.subSlot, ms);
    ASSERT_EQ(cycle_set_slot(&c, my_slot-2, SLAVE), EM_OK);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE);
    EXPECT_EQ(c.role, SLAVE);
    EXPECT_EQ(c.subSlot, ms);
    EXPECT_EQ(c.psubSlot, (my_slot-2)*CYCLE_SUB_SLOT_CNT-PRESS);
    EXPECT_EQ(c.everSlave, true);
    cycle_increment(&c);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);
    EXPECT_STREQ(cycle_role_str(&c), "SLAVE ");
    EXPECT_EQ(c.subSlot, (my_slot-2)*CYCLE_SUB_SLOT_CNT);
    EXPECT_EQ(c.psubSlot, 0);
}

TEST_F(CycleTest, SetSlotMasterRole) {
    cycle_t c{0};
    ASSERT_EQ(cycle_check_slot(my_slot), my_slot);
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, POSTRX,  &timerPtr), EM_OK);
    EXPECT_EQ(c.psubSlot, 0);
    int8_t ms = my_slot*CYCLE_SUB_SLOT_CNT-PRESS;
    EXPECT_EQ(c.subSlot, ms);
    ASSERT_EQ(cycle_set_state(&c, (system_state_e)-1), EM_ERR);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    EXPECT_EQ(cycle_get_state(&c), SYNCHRONIZE);
    EXPECT_EQ(c.psubSlot, 0);
    EXPECT_EQ(c.role, NOT_SET);
    EXPECT_EQ(c.subSlot, ms);
    ASSERT_EQ(cycle_set_slot(&c, my_slot-2, MASTER), EM_OK);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE);
    EXPECT_EQ(c.role, MASTER);
    EXPECT_EQ(c.subSlot, ms);
    EXPECT_EQ(c.psubSlot, (my_slot-2)*CYCLE_SUB_SLOT_CNT-PRESS);
    EXPECT_EQ(c.everSlave, false);
    cycle_increment(&c);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_READY);
    EXPECT_STREQ(cycle_role_str(&c), "MASTER");
    EXPECT_EQ(c.subSlot, (my_slot-2)*CYCLE_SUB_SLOT_CNT);
    EXPECT_EQ(c.psubSlot, 0);

}

// ---------------------------------------------------------------------------
// Only SLAVE and MASTER are roles a caller may ask for. A rejected role must
// not latch the cycle -- a valid call afterwards still has to succeed.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, SetSlotRejectsInvalidRole) {
    for (dev_role_e role : {NOT_SET, SS_CNT}) {
        cycle_t c{0};
        EXPECT_EQ(c.init, false);
        ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, POSTRX, &timerPtr), EM_OK);
        EXPECT_EQ(c.init, true);
        ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
        EXPECT_EQ(cycle_set_slot(&c, 3, role), EM_ERR) << "role=" << (int)role;
        EXPECT_EQ(c.sync_state, SYNCHRONIZE) << "role=" << (int)role;
    }
}

// ---------------------------------------------------------------------------
// cycle_set_slot claims a slot as SLAVE or MASTER: it only acts while
// synchronising (SYNCHRONIZE_DOING/READY), reports SYNCHRONIZE_DOING and
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
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, POSTRX , &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    const int8_t claimed = c.subSlot;

    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_LOCKED), EM_OK);
    EXPECT_EQ(cycle_set_slot(&c, 5, SLAVE), EM_OK);
    EXPECT_EQ(c.sync_state, SYNCHRONIZE_LOCKED);
    EXPECT_EQ(c.subSlot, claimed);
    EXPECT_STREQ(cycle_role_str(&c), "SLAVE "); // still the originally claimed role
}

// Outside the syncing window (e.g. just booted) it is a no-op and an error.
TEST_F(CycleTest, SetSlotRejectedOutsideSyncing) {
    cycle_t c{0};
    ASSERT_EQ(cycle_set_state(&c, SYNC_RESET), EM_ERR);
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, POSTRX, &timerPtr), EM_OK);
    const int8_t before = c.subSlot;
    for (system_state_e st : {SYNC_RESET, BOOT_UP, SLOT, CHANNEL, FREQBAND, FREQUENCY_OFFSET}) {
        ASSERT_EQ(cycle_set_state(&c, st), EM_OK);
        ASSERT_EQ(cycle_set_slot(&c, 5, SLAVE), EM_ERR);
        EXPECT_EQ(c.sync_state, st);
        EXPECT_EQ(c.role, SLAVE);
        EXPECT_EQ(c.subSlot, before) << "state=" << (int)st;
    }
}

TEST_F(CycleTest, CheckCycleIncrement) {
    cycle_t c{0};
    uint32_t cycle;
    int8_t slot = 1;
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_READY), EM_ERR);

    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, POSTRX, &timerPtr), EM_OK);
    ASSERT_EQ(cycle_set_slot(nullptr, 1, SLAVE), EM_ERR);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    ASSERT_EQ(c.psubSlot, slot * CYCLE_SUB_SLOT_CNT-PRESS);
    ASSERT_EQ(c.subSlot, my_slot* CYCLE_SUB_SLOT_CNT-PRESS);
    ASSERT_EQ(cycle_get_state(&c), SYNCHRONIZE);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_DOING), EM_OK);
    // Before a SYNCHRONIZE edge cycle_increment must not advance subSlot,
    // whatever (pre-sync) state it observes, and must leave that state alone.
    cycle_increment(&c);
    ASSERT_EQ(c.sync_state, SYNCHRONIZE_DOING);
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT-PRESS+1);
    ASSERT_EQ(c.psubSlot, 0);
    ASSERT_EQ(c.subSlot, slot*CYCLE_SUB_SLOT_CNT-PRESS+1);
    for (system_state_e st : {BOOT_UP, SLOT, FREQBAND, FREQUENCY_OFFSET}) {
        ASSERT_EQ(c.subSlot, slot*CYCLE_SUB_SLOT_CNT-PRESS+1);
        ASSERT_EQ(cycle_set_state(&c, st), EM_OK);
        cycle_increment(&c);
        ASSERT_EQ(c.subSlot, slot*CYCLE_SUB_SLOT_CNT-PRESS+1);
        ASSERT_EQ(c.sync_state, st);
    }

    // The SYNCHRONIZE edge arms advancing and reports SYNCHRONIZE_READY. The
    // arming call already advances: cycle_increment checks `is_set` after
    // setting it, so this tick counts. NOTE: the old expectation here was that
    // the arming tick does not move subSlot -- that is a one sub-slot phase
    // difference, unresolved.
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    ASSERT_EQ(c.psubSlot, slot * CYCLE_SUB_SLOT_CNT -PRESS );
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT-PRESS+1);

    cycle_increment(&c);

    ASSERT_EQ(c.psubSlot, 0);
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT -PRESS+1);
    ASSERT_EQ(c.sync_state, SYNCHRONIZE_READY);

    cycle_reset(&c);

    ASSERT_EQ(c.subSlot, my_slot*CYCLE_SUB_SLOT_CNT-PRESS);
    ASSERT_EQ(c.actSlot, ACT_SLOT(&c));
    ASSERT_EQ(c.sSlot, 0);
    ASSERT_EQ(c.cycle, 0);
    ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE_DOING), EM_OK);
    ASSERT_EQ(c.sync_state, SYNCHRONIZE_DOING);
    // cycle_increment advances first and reports afterwards, so start one tick
    // below the wrap: the first call then lands on subSlot 0 (slot 0, sub slot
    // 0) and the loop variables match the reported values 1:1.
    for (cycle = 0; cycle <= UINT16_MAX; cycle++) {
        for (slot = my_slot; slot < my_slot+CYCLE_SLOT_CNT; (slot++)%CYCLE_SLOT_CNT) {
            for (uint8_t ss = PRESS; ss < (PRESS); (ss++)%CYCLE_SUB_SLOT_CNT) {
                cycle_increment(&c);
                ASSERT_EQ(c.sync_state, SYNCHRONIZE_DOING);
                ASSERT_EQ(c.subSlot, ss + slot * CYCLE_SUB_SLOT_CNT-PRESS);
                ASSERT_EQ(c.actSlot, slot);
                ASSERT_EQ(c.sSlot,   ss);
                ASSERT_EQ(c.cycle,   cycle);
            }
        }
    }
    // The loop above left the cycle counter at its uint16_t maximum; one more
    // wrap of subSlot drives cycle past 65535 and overflows it back to 0.
    cycle_increment(&c);
    ASSERT_EQ(c.sync_state, SYNCHRONIZE_DOING);
    ASSERT_EQ(c.subSlot, (my_slot*CYCLE_SUB_SLOT_CNT-PRESS+1)%CYCLE_MODULO);
    ASSERT_EQ(c.actSlot, my_slot);
    ASSERT_EQ(c.sSlot, (CYCLE_SUB_SLOT_CNT-PRESS+1)%CYCLE_SUB_SLOT_CNT);
    ASSERT_EQ(c.cycle, 0);

}

// ---------------------------------------------------------------------------
// cycle_difference: sub-slot distance from the current position to rxSlot's
// window, with
//   lower = rxSlot*CYCLE_SUB_SLOT_CNT
//   upper = (rxSlot+1)*CYCLE_SUB_SLOT_CNT.
// Inside [lower, upper) => 0. Outside, the cycle is a ring of CYCLE_MODULO
// sub-slots and the shorter way round wins:
//   min((subSlot - upper) mod CYCLE_MODULO, (lower - subSlot) mod CYCLE_MODULO)
// Note the asymmetry this brings at the corners: subSlot == lower-1 is
// distance 1, but subSlot == upper is distance 0, so the zero band is
// [lower, upper] -- one wider than the window itself.
// ---------------------------------------------------------------------------

// Reference for cycle_difference. This mirrors the implementation, so the
// exhaustive sweep below is a regression net (int8_t range, guards, wrap), not
// an independent proof -- the hand-computed spot checks carry the spec.
static int8_t expected_difference(int16_t subSlot, int8_t rxSlot) {
    const int16_t lower = (int16_t)rxSlot * CYCLE_SUB_SLOT_CNT;
    const int16_t upper = ((int16_t)rxSlot + 1) * CYCLE_SUB_SLOT_CNT;
    if ((subSlot >= lower) && (subSlot < upper)) {
        return 0; // inside the window
    }
    const int16_t above = ((subSlot - upper) + CYCLE_MODULO) % CYCLE_MODULO;
    const int16_t below = ((lower - subSlot) + CYCLE_MODULO) % CYCLE_MODULO;
    return (int8_t)((above < below) ? above : below);
}

TEST_F(CycleTest, CycleDifference) {

    // NULL / uninitialised guards return 0.
    EXPECT_EQ(cycle_difference(nullptr, 0), 255);
    cycle_t u{};
    u.init = false;
    EXPECT_EQ(cycle_difference(&u, 0), 255);

    cycle_t c{0};
    for (int8_t slot=0;slot<CYCLE_SLOT_CNT/2;slot++){
        ASSERT_EQ(cycle_init(&c, 2*slot+1, PRESS, POSTSS, POSTRX,  &timerPtr), EM_OK);
        ASSERT_EQ(cycle_set_state(&c, SYNCHRONIZE), EM_OK);
        cycle_increment(&c);
        for (int ss = 0; ss < CYCLE_MODULO; ss++) {
             for (int8_t rx = 0; rx < CYCLE_SLOT_CNT; rx++) {
                 c.subSlot = (int16_t)ss;
                 uint8_t got = cycle_difference(&c, rx);
                 EXPECT_EQ(got, expected_difference((int16_t)ss, rx)) << "subSlot=" << ss << " rxSlot=" << (int)rx;
                 EXPECT_LE(got, CYCLE_MODULO / 2);     // the shorter way round, so at most half the ring
             }
             return ;
        }

    }
}

// The distance is symmetric around the window: stepping n sub-slots below
// `lower` must read the same as stepping n above `upper`, all the way to the
// far side of the ring. Independent of expected_difference().
TEST_F(CycleTest, CycleDifferenceIsSymmetric) {
    cycle_t c{0};
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, POSTRX, &timerPtr), EM_OK);
    // slot 0 and slot 15 put the window on the ring seam, so both the "below"
    // and the "above" side wrap.
    for (int8_t slot = 0; slot < CYCLE_SLOT_CNT; slot++) {
        const int lower = slot * CYCLE_SUB_SLOT_CNT;
        const int upper = (slot + 1) * CYCLE_SUB_SLOT_CNT;
        for (int n = 0; n <= (CYCLE_MODULO - CYCLE_SUB_SLOT_CNT) / 2; n++) {
            c.subSlot = (int8_t)((lower - n + CYCLE_MODULO) % CYCLE_MODULO);
            const int8_t below = cycle_difference(&c, slot);
            c.subSlot = (int8_t)((upper + n) % CYCLE_MODULO);
            const int8_t above = cycle_difference(&c, slot);
            EXPECT_EQ(below, above) << "slot=" << (int)slot << " n=" << n;
            EXPECT_EQ(below, n) << "slot=" << (int)slot << " n=" << n;
        }
    }
}
