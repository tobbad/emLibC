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
#define PRESS 3
class CycleTest : public ::testing::Test {
  protected:
    cycle_t cycle;
    system_state_e sync_state = SYNC_RESET;
    void SetUp() override {}
};

// ---------------------------------------------------------------------------
// cycle_init / cycle_reset
// ---------------------------------------------------------------------------
TEST_F(CycleTest, NullNullPtrReturnsError)  { EXPECT_EQ(cycle_init(nullptr, PRESS, nullptr), EM_ERR); }
TEST_F(CycleTest, NullValidPtrReturnsError) { EXPECT_EQ(cycle_init(nullptr, PRESS, &sync_state), EM_ERR); }
TEST_F(CycleTest, ValidNullPtrReturnsError) { EXPECT_EQ(cycle_init(&cycle, PRESS,  nullptr), EM_ERR); }

TEST_F(CycleTest, CheckSetSlot) {
    uint8_t slot;
    system_state_e sync_state = SYNCHRONIZE_READY;
    cycle_t c;
    ASSERT_EQ(cycle_init(&c, PRESS, &sync_state), EM_OK);
    // cycle_set_slot only acts while the device is synchronising
    // (SYNCHRONIZE_DOING/READY) and locks (-> SYNCHRONIZE_LOCKED) on success,
    // so re-arm sync_state before every call.
    for (slot = 0; slot < CYCLE_SLOT_CNT; slot++) {
        sync_state = SYNCHRONIZE_READY;
        if (slot % 2 == 1) {
            ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
        } else {
            ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_ERR);
        }
    }
    for (; slot < 2 * CYCLE_SLOT_CNT; slot++) {
        sync_state = SYNCHRONIZE_READY;
        ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_ERR);
    }
}

// ---------------------------------------------------------------------------
// cycle_set_slot locks the device into SLAVE or MASTER mode: it only acts while
// synchronising (SYNCHRONIZE_DOING/READY), moves sync_state to
// SYNCHRONIZE_LOCKED, and positions subSlot at the slot start (SLAVE) or
// CYCLE_PRESS sub-slots early (MASTER). Once locked / outside syncing it is a
// no-op for state and position.
// ---------------------------------------------------------------------------
TEST_F(CycleTest, SetSlotLock) {
    const int8_t slot = 3;
    cycle_t c;
    sync_state = SYNCHRONIZE_READY;
    ASSERT_EQ(cycle_init(&c, PRESS, &sync_state), EM_OK);
    EXPECT_STREQ(cycle_role(&c), "SLAVE "); // default role after init

    // SLAVE lock: sit exactly at the slot start.
    sync_state = SYNCHRONIZE_READY;
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    EXPECT_EQ(sync_state, SYNCHRONIZE_LOCKED);
    EXPECT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT);
    EXPECT_EQ(c.actSlot, slot);
    EXPECT_EQ(c.sSlot, 0);
    EXPECT_STREQ(cycle_role(&c), "SLAVE ");

    // MASTER lock: positioned PRESS sub-slots early, so actSlot is the slot
    // below and sSlot is near its upper end.
    const int16_t master_sub = slot * CYCLE_SUB_SLOT_CNT - PRESS;
    sync_state = SYNCHRONIZE_DOING; // DOING must also permit locking
    ASSERT_EQ(cycle_set_slot(&c, slot, MASTER), EM_OK);
    EXPECT_EQ(sync_state, SYNCHRONIZE_LOCKED);
    EXPECT_EQ(c.subSlot, master_sub);
    EXPECT_EQ(c.actSlot, (master_sub >> CYCLE_SUB_SLOT_SHIFT) & CYCLE_SLOT_MASK);
    EXPECT_EQ(c.sSlot, master_sub & CYCLE_SUB_SLOT_MASK);
    EXPECT_STREQ(cycle_role(&c), "MASTER");

    // Already LOCKED: cycle_set_slot must not re-lock; state and position stay.
    sync_state = SYNCHRONIZE_LOCKED;
    EXPECT_EQ(cycle_set_slot(&c, 5, SLAVE), EM_ERR);
    EXPECT_EQ(sync_state, SYNCHRONIZE_LOCKED);
    EXPECT_EQ(c.subSlot, master_sub);

    // Outside the syncing window (e.g. just booted) it is also a no-op.
    sync_state = SYNC_RESET;
    EXPECT_EQ(cycle_set_slot(&c, 5, SLAVE), EM_ERR);
    EXPECT_EQ(sync_state, SYNC_RESET);
    EXPECT_EQ(c.subSlot, master_sub);
}

TEST_F(CycleTest, CheckCycleIncrement) {
    cycle_t c;
    uint32_t cycle;
    int8_t slot = 3;
    sync_state = SYNCHRONIZE_READY; // precondition for cycle_set_slot
    ASSERT_EQ(cycle_init(&c, PRESS, &sync_state), EM_OK);
    ASSERT_EQ(cycle_set_slot(nullptr, 1, SLAVE), EM_ERR);
    sync_state = SYNCHRONIZE_READY;
    ASSERT_EQ(cycle_set_slot(&c, slot, SLAVE), EM_OK);
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT); // SLAVE -> slot * CYCLE_SUB_SLOT_CNT
    ASSERT_EQ(sync_state, SYNCHRONIZE_LOCKED);       // a successful set_slot locks

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

    // The SYNCHRONIZE edge arms advancing and reports SYNCHRONIZE_READY; the
    // arming call itself still does not move subSlot.
    sync_state = SYNCHRONIZE;
    cycle_increment(&c);
    ASSERT_EQ(sync_state, SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot, slot * CYCLE_SUB_SLOT_CNT);
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
// cycle_difference: signed sub-slot distance from the current position to
// rxSlot's window, which spans CYCLE_SUB_SLOT_CNT sub-slots
//   [rxSlot*CYCLE_SUB_SLOT_CNT, rxSlot*CYCLE_SUB_SLOT_CNT + CYCLE_SUB_SLOT_CNT-1].
// Inside the window => 0; below the lower corner => negative distance to it;
// above the upper corner => positive distance past it. Measured the shortest
// way around the CYCLE_SUB_SLOT_CNT*CYCLE_SLOT_CNT ring, so rxSlot 0 and rxSlot
// CYCLE_SLOT_CNT coincide.
// ---------------------------------------------------------------------------

// Reference for cycle_difference: fold to a signed distance to the lower corner,
// then clamp the window to 0 and re-base the upper side onto the upper corner.
static int8_t expected_difference(int16_t subSlot, int8_t rxSlot) {
    const int16_t total = CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT; // 128
    const int16_t half  = total / 2;                           // 64
    int16_t d = (subSlot - (int16_t)rxSlot * CYCLE_SUB_SLOT_CNT) % total;
    if (d < 0) {
        d += total;
    }
    if (d >= half) {
        d -= total; // signed distance to the lower corner, [-half, half)
    }
    if (d < 0) {
        return (int8_t)d; // below the lower corner
    }
    if (d < CYCLE_SUB_SLOT_CNT) {
        return 0; // inside the window
    }
    return (int8_t)(d - (CYCLE_SUB_SLOT_CNT - 1)); // above the upper corner
}

TEST_F(CycleTest, CycleDifference) {
    const int total = CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT; // 128 sub-slots per cycle
    const int half  = total / 2;                           // 64

    // NULL / uninitialised guards return 0.
    EXPECT_EQ(cycle_difference(nullptr, 0), 0);
    cycle_t u{};
    u.init = false;
    EXPECT_EQ(cycle_difference(&u, 0), 0);

    cycle_t c;
    ASSERT_EQ(cycle_init(&c, PRESS, &sync_state), EM_OK);

    // --- rxSlot == 0 spot checks (window = sub-slots 0..7) -----------------
    c.subSlot = 0;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // lower corner, inside
    c.subSlot = 3;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // inside
    c.subSlot = 7;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // upper corner, inside
    c.subSlot = 8;   EXPECT_EQ(cycle_difference(&c, 0), 1);   // 1 above the upper corner
    c.subSlot = 120; EXPECT_EQ(cycle_difference(&c, 0), -8);  // 8 below the lower corner
    c.subSlot = 127; EXPECT_EQ(cycle_difference(&c, 0), -1);  // 1 below the lower corner
    c.subSlot = 64;  EXPECT_EQ(cycle_difference(&c, 0), -half); // antipode folds negative
    c.subSlot = 0;   EXPECT_EQ(cycle_difference(&c, 1), -8);  // next window's lower corner is 8 ahead

    // --- window edges for rxSlot == 2 (window = sub-slots 16..23) ----------
    c.subSlot = 14;  EXPECT_EQ(cycle_difference(&c, 2), -2);
    c.subSlot = 15;  EXPECT_EQ(cycle_difference(&c, 2), -1);
    c.subSlot = 16;  EXPECT_EQ(cycle_difference(&c, 2), 0);   // lower corner
    c.subSlot = 23;  EXPECT_EQ(cycle_difference(&c, 2), 0);   // upper corner
    c.subSlot = 24;  EXPECT_EQ(cycle_difference(&c, 2), 1);
    c.subSlot = 25;  EXPECT_EQ(cycle_difference(&c, 2), 2);

    // --- rxSlot == CYCLE_SLOT_CNT corner case ------------------------------
    // rxSlot*CYCLE_SUB_SLOT_CNT == total, so slot CYCLE_SLOT_CNT == slot 0:
    // cycle_difference(&c, CYCLE_SLOT_CNT) must equal cycle_difference(&c, 0).
    for (int ss = 0; ss < total; ss++) {
        c.subSlot = (int8_t)ss;
        EXPECT_EQ(cycle_difference(&c, CYCLE_SLOT_CNT), cycle_difference(&c, 0))
            << "subSlot=" << ss;
    }

    // Exhaustive cross-check against the reference for rxSlot 0..CYCLE_SLOT_CNT.
    // Output range is [-half, half - CYCLE_SUB_SLOT_CNT] = [-64, 56].
    for (int ss = 0; ss < total; ss++) {
        for (int8_t rx = 0; rx <= CYCLE_SLOT_CNT; rx++) {
            c.subSlot = (int8_t)ss;
            int8_t got = cycle_difference(&c, rx);
            EXPECT_EQ(got, expected_difference((int16_t)ss, rx)) << "subSlot=" << ss << " rxSlot=" << (int)rx;
            EXPECT_GE(got, -half);
            EXPECT_LE(got, half - CYCLE_SUB_SLOT_CNT);
        }
    }
}
