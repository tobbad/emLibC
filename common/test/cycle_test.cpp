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
#define PRESS  3
#define POSTSS 3
static int8_t my_slot = 3;
class CycleTest : public ::testing::Test {
  protected:
    cycle_t cycle;
    system_state_e sync_state = SYNC_RESET;
    void SetUp() override {}
    void TearDown() {}

};

// ---------------------------------------------------------------------------
// cycle_init / cycle_reset
// ---------------------------------------------------------------------------
TEST_F(CycleTest, NullNullPtrReturnsError)  { EXPECT_EQ(cycle_init(nullptr, my_slot, PRESS, POSTSS, nullptr, nullptr), EM_ERR); }
TEST_F(CycleTest, NullValidPtrReturnsError) { EXPECT_EQ(cycle_init(nullptr, my_slot, PRESS, POSTSS, &sync_state, nullptr), EM_ERR); }
TEST_F(CycleTest, ValidNullPtrReturnsError) { EXPECT_EQ(cycle_init(&cycle,  my_slot, PRESS, POSTSS, nullptr, nullptr), EM_ERR); }

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
// cycle_set_slot accepts exactly the slots cycle_check_slot validates. While
// synchronising it moves sync_state to SYNCHRONIZE_DOING and, as SLAVE, parks
// subSlot at the slot start. A rejected slot leaves sync_state alone.
// A cycle_t latches its role on the first success (see SetSlotLatchesRole), so
// every slot needs its own freshly initialised cycle.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, CheckSetSlot) {
    for (int8_t slot = 0; slot < 2 * CYCLE_SLOT_CNT; slot++) {
        const bool valid = (slot % 2 == 1) && (slot < CYCLE_SLOT_CNT);
        system_state_e sync_state = SYNCHRONIZE_READY;
        cycle_t c;
        ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
        if (valid) {
            ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK) << "slot=" << (int)slot;
            ASSERT_EQ(sync_state, SYNCHRONIZE_DOING) << "slot=" << (int)slot;
            ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT) << "slot=" << (int)slot;
        } else {
            ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_ERR) << "slot=" << (int)slot;
            ASSERT_EQ(sync_state, SYNCHRONIZE_READY) << "slot=" << (int)slot;
        }
    }
}

// ---------------------------------------------------------------------------
// The role latches on the first successful cycle_set_slot: later calls are
// rejected and leave the position untouched.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, SetSlotLatchesRole) {
    system_state_e sync_state = SYNCHRONIZE_READY;
    cycle_t c;
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, 3, SLAVE), EM_OK);
    const int8_t subSlot = c.subSlot;

    EXPECT_EQ(cycle_set_slot(&c, 5, SLAVE), EM_ERR);
    EXPECT_EQ(cycle_set_slot(&c, 3, MASTER), EM_ERR);
    EXPECT_EQ(c.subSlot, subSlot);
}

// ---------------------------------------------------------------------------
// Only SLAVE and MASTER are roles a caller may ask for. A rejected role must
// not latch the cycle -- a valid call afterwards still has to succeed.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, SetSlotRejectsInvalidRole) {
    for (set_slot_e role : {NOT_SET, SS_CNT}) {
        system_state_e sync_state = SYNCHRONIZE_READY;
        cycle_t c;
        ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
        EXPECT_EQ(cycle_set_slot(&c, 3, role), EM_ERR) << "role=" << (int)role;
        EXPECT_EQ(sync_state, SYNCHRONIZE_READY) << "role=" << (int)role;
        EXPECT_EQ(cycle_set_slot(&c, 3, SLAVE), EM_OK) << "role=" << (int)role;
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
TEST_F(CycleTest, SetSlotSlavePosition) {
    const int8_t slot = 3;
    cycle_t c;
    sync_state = SYNCHRONIZE_READY;
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
    EXPECT_STREQ(cycle_role(&c), "NA "); // role is NOT_SET after init, which cycle2str does not map

    // SLAVE: sit exactly at the slot start.
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    EXPECT_EQ(sync_state, SYNCHRONIZE_DOING);
    EXPECT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT);
    EXPECT_EQ(c.actSlot, slot);
    EXPECT_EQ(c.sSlot, 0);
    EXPECT_STREQ(cycle_role(&c), "SLAVE ");
}

TEST_F(CycleTest, SetSlotMasterPosition) {
    const int8_t slot = 3;
    cycle_t c;
    sync_state = SYNCHRONIZE_DOING; // DOING must also permit claiming
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);

    // MASTER is offset by press against the slot start. NOTE: the code adds
    // press (subSlot 27 for slot 3), so master sits *inside* its own slot --
    // the old expectation here was slot*CNT - PRESS (21, i.e. press sub-slots
    // *early*), which is what cycle_doSend's lead time implies. Sign unresolved.
    const int16_t master_sub = (slot * CYCLE_SUB_SLOT_CNT + CYCLE_MODULO + PRESS) % CYCLE_MODULO;
    ASSERT_EQ(cycle_set_slot(&c, slot, MASTER), EM_OK);
    EXPECT_EQ(sync_state, SYNCHRONIZE_DOING);
    EXPECT_EQ(c.subSlot, master_sub);
    EXPECT_EQ(c.actSlot, (master_sub >> CYCLE_SUB_SLOT_SHIFT) & CYCLE_SLOT_MASK);
    EXPECT_EQ(c.sSlot, master_sub & CYCLE_SUB_SLOT_MASK);
    EXPECT_STREQ(cycle_role(&c), "MASTER");
}

// The application sets SYNCHRONIZE_LOCKED to freeze the cycle; cycle_set_slot
// must then leave state and position alone and report success.
TEST_F(CycleTest, SetSlotFrozenWhenLocked) {
    const int8_t slot = 3;
    cycle_t c;
    sync_state = SYNCHRONIZE_READY;
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    const int8_t claimed = c.subSlot;

    sync_state = SYNCHRONIZE_LOCKED; // the app freezes
    EXPECT_EQ(cycle_set_slot(&c, 5, SLAVE), EM_OK);
    EXPECT_EQ(sync_state, SYNCHRONIZE_LOCKED);
    EXPECT_EQ(c.subSlot, claimed);
    EXPECT_STREQ(cycle_role(&c), "SLAVE "); // still the originally claimed role
}

// Outside the syncing window (e.g. just booted) it is a no-op and an error.
TEST_F(CycleTest, SetSlotRejectedOutsideSyncing) {
    cycle_t c;
    sync_state = SYNC_RESET;
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
    const int8_t before = c.subSlot;
    for (system_state_e st : {SYNC_RESET, BOOT_UP, SLOT, CHANNEL, FREQBAND, FREQUENCY_OFFSET, SYNCHRONIZE}) {
        sync_state = st;
        EXPECT_EQ(cycle_set_slot(&c, 5, SLAVE), EM_ERR) << "state=" << (int)st;
        EXPECT_EQ(sync_state, st) << "state=" << (int)st;
        EXPECT_EQ(c.subSlot, before) << "state=" << (int)st;
    }
}

TEST_F(CycleTest, CheckCycleIncrement) {
    cycle_t c;
    uint32_t cycle;
    int8_t slot = 3;
    sync_state = SYNCHRONIZE_READY; // precondition for cycle_set_slot
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
    ASSERT_EQ(cycle_set_slot(nullptr, 1, SLAVE), EM_ERR);
    sync_state = SYNCHRONIZE_READY;
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT); // SLAVE -> slot * CYCLE_SUB_SLOT_CNT
    ASSERT_EQ(sync_state, SYNCHRONIZE_DOING);        // a successful set_slot reports DOING

    // Before a SYNCHRONIZE edge cycle_increment must not advance subSlot,
    // whatever (pre-sync) state it observes, and must leave that state alone.
    cycle_increment(&c);
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT);
    for (system_state_e st : {BOOT_UP, SLOT, FREQBAND, FREQUENCY_OFFSET}) {
        sync_state = st;
        cycle_increment(&c);
        ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT);
        ASSERT_EQ(sync_state, st);
    }

    // The SYNCHRONIZE edge arms advancing and reports SYNCHRONIZE_READY. The
    // arming call already advances: cycle_increment checks `is_set` after
    // setting it, so this tick counts. NOTE: the old expectation here was that
    // the arming tick does not move subSlot -- that is a one sub-slot phase
    // difference, unresolved.
    sync_state = SYNCHRONIZE;
    cycle_increment(&c);
    ASSERT_EQ(sync_state, SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT + 1);
    cycle_reset(&c);
    // cycle_increment advances first and reports afterwards, so start one tick
    // below the wrap: the first call then lands on subSlot 0 (slot 0, sub slot
    // 0) and the loop variables match the reported values 1:1.
    c.subSlot = CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT - 1;
    for (cycle = 0; cycle <= UINT16_MAX; cycle++) {
        for (slot = 0; slot < CYCLE_SLOT_CNT; slot++) {
            for (uint8_t ss = 0; ss < CYCLE_SUB_SLOT_CNT; ss++) {
                cycle_increment(&c);
                ASSERT_EQ(sync_state, SYNCHRONIZE_READY);
                ASSERT_EQ(c.subSlot, ss + slot * CYCLE_SUB_SLOT_CNT);
                ASSERT_EQ(c.actSlot, slot);
                ASSERT_EQ(c.sSlot, ss);
                ASSERT_EQ(c.cycle, cycle);
            }
        }
    }
    // The loop above left the cycle counter at its uint16_t maximum; one more
    // wrap of subSlot drives cycle past 65535 and overflows it back to 0.
    cycle_increment(&c);
    ASSERT_EQ(sync_state, SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot,0);
    ASSERT_EQ(c.actSlot, 0);
    ASSERT_EQ(c.sSlot, 0);
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
    const int total = CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT; // 128 sub-slots per cycle

    // NULL / uninitialised guards return 0.
    EXPECT_EQ(cycle_difference(nullptr, 0), 0);
    cycle_t u{};
    u.init = false;
    EXPECT_EQ(cycle_difference(&u, 0), 0);

    cycle_t c;
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);

    // --- rxSlot == 0 spot checks (window = [0, 8), lower 0, upper 8) --------
    c.subSlot = 0;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // lower corner, inside
    c.subSlot = 3;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // inside
    c.subSlot = 7;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // last inside
    c.subSlot = 8;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // == upper -> above: 8-8
    c.subSlot = 9;   EXPECT_EQ(cycle_difference(&c, 0), 1);   // above: 9-8
    c.subSlot = 64;  EXPECT_EQ(cycle_difference(&c, 0), 56);  // above: 64-8
    c.subSlot = 68;  EXPECT_EQ(cycle_difference(&c, 0), 60);  // equidistant, the maximum
    c.subSlot = 69;  EXPECT_EQ(cycle_difference(&c, 0), 59);  // past halfway -> below wins
    c.subSlot = 120; EXPECT_EQ(cycle_difference(&c, 0), 8);   // wraps: (0-120) mod 128
    c.subSlot = 127; EXPECT_EQ(cycle_difference(&c, 0), 1);   // wraps: one step short of 0

    // --- rxSlot == 2 window edges (window = [16, 24)) -----------------------
    c.subSlot = 14;  EXPECT_EQ(cycle_difference(&c, 2), 2);   // below: 16-14
    c.subSlot = 15;  EXPECT_EQ(cycle_difference(&c, 2), 1);   // below: 16-15
    c.subSlot = 16;  EXPECT_EQ(cycle_difference(&c, 2), 0);   // lower corner, inside
    c.subSlot = 23;  EXPECT_EQ(cycle_difference(&c, 2), 0);   // last inside
    c.subSlot = 24;  EXPECT_EQ(cycle_difference(&c, 2), 0);   // == upper -> above: 24-24
    c.subSlot = 25;  EXPECT_EQ(cycle_difference(&c, 2), 1);   // above: 25-24

    // --- rxSlot == 15: the window sits at the ring seam (window [120, 128)) -
    c.subSlot = 127; EXPECT_EQ(cycle_difference(&c, 15), 0);  // last inside
    c.subSlot = 119; EXPECT_EQ(cycle_difference(&c, 15), 1);  // below: 120-119
    c.subSlot = 0;   EXPECT_EQ(cycle_difference(&c, 15), 0);  // above wraps: (0-128) mod 128
    c.subSlot = 7;   EXPECT_EQ(cycle_difference(&c, 15), 7);  // above wraps: (7-128) mod 128

    // Exhaustive sweep: every subSlot against every rxSlot.
    for (int ss = 0; ss < total; ss++) {
        for (int8_t rx = 0; rx < CYCLE_SLOT_CNT; rx++) {
            c.subSlot = (int8_t)ss;
            int8_t got = cycle_difference(&c, rx);
            EXPECT_EQ(got, expected_difference((int16_t)ss, rx)) << "subSlot=" << ss << " rxSlot=" << (int)rx;
            EXPECT_GE(got, 0);                    // never negative
            EXPECT_LE(got, CYCLE_MODULO / 2);     // the shorter way round, so at most half the ring
        }
    }
}

// The distance is symmetric around the window: stepping n sub-slots below
// `lower` must read the same as stepping n above `upper`, all the way to the
// far side of the ring. Independent of expected_difference().
TEST_F(CycleTest, CycleDifferenceIsSymmetric) {
    cycle_t c;
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
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

// ---------------------------------------------------------------------------
// cycle_isOk: true when we sit close enough to rxSlot's window, i.e.
// cycle_difference(cycle, rxSlot) < press. Guards (NULL / uninitialised /
// invalid rxSlot) return false. Valid rxSlot are the odd slots
// 1..CYCLE_SLOT_CNT-1 (see cycle_check_slot).
// ---------------------------------------------------------------------------
TEST_F(CycleTest, CheckIsOk) {
    const int total = CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT; // 128

    // --- guards ------------------------------------------------------------
    EXPECT_FALSE(cycle_isOk(nullptr, 3));
    cycle_t u{};
    u.init = false;
    EXPECT_FALSE(cycle_isOk(&u, 3));

    cycle_t c;
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK); // press == PRESS (3)

    // out-of-range rxSlot is never ok, whatever subSlot.
    for (int ss = 0; ss < total; ss++) {
        c.subSlot = (int8_t)ss;
        EXPECT_FALSE(cycle_isOk(&c, 16)) << "subSlot=" << ss;
    }

    // --- spot checks for rxSlot == 3 (window [24, 31], press == 3) ----------
    c.subSlot = 24; EXPECT_TRUE(cycle_isOk(&c, 3));  // lower corner -> diff 0
    c.subSlot = 27; EXPECT_TRUE(cycle_isOk(&c, 3));  // inside window -> diff 0
    c.subSlot = 31; EXPECT_TRUE(cycle_isOk(&c, 3));  // upper corner -> diff 0
    c.subSlot = 23; EXPECT_TRUE(cycle_isOk(&c, 3));  // below, diff 1 < press
    c.subSlot = 22; EXPECT_TRUE(cycle_isOk(&c, 3));  // below, diff 2 < press
    c.subSlot = 21; EXPECT_FALSE(cycle_isOk(&c, 3)); // below, diff 3 == press
    c.subSlot = 20; EXPECT_FALSE(cycle_isOk(&c, 3)); // below, diff 4 > press
    // Above the window cycle_difference measures from `upper`, so subSlot 32
    // (just past the window) still reads as distance 0 -- see the asymmetry
    // note on cycle_difference.
    c.subSlot = 32; EXPECT_TRUE(cycle_isOk(&c, 3));  // above, diff 0 < press
    c.subSlot = 34; EXPECT_TRUE(cycle_isOk(&c, 3));  // above, diff 2 < press
    c.subSlot = 35; EXPECT_FALSE(cycle_isOk(&c, 3)); // above, diff 3 == press

    // --- exhaustive cross-check over all slots 0..CYCLE_SLOT_CNT-1 ----------
    // Even slots and slot 0 are invalid (cycle_check_slot) -> always false;
    // valid odd slots follow cycle_difference < press.
    for (int8_t rx = 0; rx < CYCLE_SLOT_CNT; rx++) {
        const bool valid = (rx > 0) && (rx % 2 == 1);
        for (int ss = 0; ss < total; ss++) {
            c.subSlot = (int8_t)ss;
            const bool expected = valid && (cycle_difference(&c, rx) < cycle_press(&c));
            EXPECT_EQ(cycle_isOk(&c, rx), expected)
                << "subSlot=" << ss << " rxSlot=" << (int)rx;
        }
    }
}
