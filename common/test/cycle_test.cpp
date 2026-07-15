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
};

// ---------------------------------------------------------------------------
// cycle_init / cycle_reset
// ---------------------------------------------------------------------------
TEST_F(CycleTest, NullNullPtrReturnsError)  { EXPECT_EQ(cycle_init(nullptr, my_slot, PRESS, POSTSS, nullptr, nullptr), EM_ERR); }
TEST_F(CycleTest, NullValidPtrReturnsError) { EXPECT_EQ(cycle_init(nullptr, my_slot, PRESS, POSTSS, &sync_state, nullptr), EM_ERR); }
TEST_F(CycleTest, ValidNullPtrReturnsError) { EXPECT_EQ(cycle_init(&cycle,  my_slot, PRESS, POSTSS, nullptr, nullptr), EM_ERR); }

TEST_F(CycleTest, CheckSetSlot) {
    uint8_t slot;
    system_state_e sync_state = SYNCHRONIZE_READY;
    cycle_t c;
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
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
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
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
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
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
// cycle_difference: positive sub-slot distance from the current position to
// rxSlot's window, with
//   lower = rxSlot*CYCLE_SUB_SLOT_CNT
//   upper = (rxSlot+1)*CYCLE_SUB_SLOT_CNT.
// Below the lower corner => lower - subSlot; above the upper corner =>
// subSlot - lower; inside [lower, upper] => 0. Always >= 0, no ring wrap.
// ---------------------------------------------------------------------------

// Reference for cycle_difference: positive distance to rxSlot's window.
static int8_t expected_difference(int16_t subSlot, int8_t rxSlot) {
    const int16_t lower = (int16_t)rxSlot * CYCLE_SUB_SLOT_CNT;
    const int16_t upper = ((int16_t)rxSlot + 1) * CYCLE_SUB_SLOT_CNT;
    if (subSlot < lower) {
        return (int8_t)(lower - subSlot); // below the lower corner
    }
    if (subSlot >= upper) {
        return (int8_t)(subSlot - lower); // at/above the upper edge
    }
    return 0; // inside the window
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

    // --- rxSlot == 0 spot checks (window = [0, 7], 8 wide) -----------------
    c.subSlot = 0;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // lower corner, inside
    c.subSlot = 3;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // inside
    c.subSlot = 7;   EXPECT_EQ(cycle_difference(&c, 0), 0);   // upper corner, inside
    c.subSlot = 8;   EXPECT_EQ(cycle_difference(&c, 0), 8);   // upper edge -> above: subSlot - lower
    c.subSlot = 64;  EXPECT_EQ(cycle_difference(&c, 0), 64);  // above
    c.subSlot = 127; EXPECT_EQ(cycle_difference(&c, 0), 127); // above, max
    c.subSlot = 0;   EXPECT_EQ(cycle_difference(&c, 1), 8);   // below: lower - subSlot = 8 - 0

    // --- window edges for rxSlot == 2 (window = [16, 23], 8 wide) ----------
    c.subSlot = 14;  EXPECT_EQ(cycle_difference(&c, 2), 2);   // below: 16 - 14
    c.subSlot = 15;  EXPECT_EQ(cycle_difference(&c, 2), 1);   // below: 16 - 15
    c.subSlot = 16;  EXPECT_EQ(cycle_difference(&c, 2), 0);   // lower corner
    c.subSlot = 23;  EXPECT_EQ(cycle_difference(&c, 2), 0);   // upper corner, inside
    c.subSlot = 24;  EXPECT_EQ(cycle_difference(&c, 2), 8);   // upper edge -> above: 24 - 16

    // Exhaustive cross-check against the reference for rxSlot 0..CYCLE_SLOT_CNT-1.
    for (int ss = 0; ss < total; ss++) {
        for (int8_t rx = 0; rx < CYCLE_SLOT_CNT; rx++) {
            c.subSlot = (int8_t)ss;
            int8_t got = cycle_difference(&c, rx);
            EXPECT_EQ(got, expected_difference((int16_t)ss, rx)) << "subSlot=" << ss << " rxSlot=" << (int)rx;
            EXPECT_GE(got, 0); // always positive or zero
        }
    }
}

TEST_F(CycleTest, MyCycleDifference) {
    int8_t diff;
    int8_t exp;
    cycle_t c;
    const int total = CYCLE_SUB_SLOT_CNT * CYCLE_SLOT_CNT; // 128 sub-slots per cycle
    ASSERT_EQ(cycle_init(&c, my_slot, PRESS, POSTSS, &sync_state, NULL), EM_OK);
    for (uint8_t press =0; press<CYCLE_SUB_SLOT_CNT;press++){
        // slot 0..CYCLE_SLOT_CNT-1 covers both boundaries: slot 0 (lower corner,
        // "below" wraps onto subSlot 120..127) and slot 15 (upper corner,
        // "above" wraps onto subSlot 0..7).
        for (uint8_t slot=0;slot < CYCLE_SLOT_CNT; slot++){
            cycle_set_slot(&c, slot, SLAVE);
            printf("slot %d"NL, slot);
            for (uint8_t ss=0;ss<total;ss++){
                c.subSlot = ss;
                diff = cycle_difference(&c, slot);
                int lower = slot*CYCLE_SUB_SLOT_CNT;
                int upper = (slot+1)*CYCLE_SUB_SLOT_CNT;
                if (ss < lower){
                    exp = lower - ss;       // below
                } else if (ss >= upper){
                    exp = ss - lower;       // at/above the upper edge
                } else {
                    exp = 0;                // inside [lower, upper)
                }
                EXPECT_EQ(diff, exp) << "subSlot=" << (int)ss << " rxSlot=" << (int)slot
                                     << " diff=" << (int)diff << " exp=" << (int)exp;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// cycle_isOk: true when the distance to rxSlot's window exceeds the press
// tolerance, i.e. cycle_difference(cycle, rxSlot) > press. Guards (NULL /
// uninitialised / invalid rxSlot) return false. Valid rxSlot are the odd
// slots 1..CYCLE_SLOT_CNT-1 (see cycle_check_slot).
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
    c.subSlot = 27; EXPECT_FALSE(cycle_isOk(&c, 3)); // inside window -> diff 0
    c.subSlot = 23; EXPECT_FALSE(cycle_isOk(&c, 3)); // below, diff 1 <= press
    c.subSlot = 21; EXPECT_FALSE(cycle_isOk(&c, 3)); // below, diff 3 == press
    c.subSlot = 20; EXPECT_TRUE(cycle_isOk(&c, 3));  // below, diff 4 > press
    c.subSlot = 32; EXPECT_TRUE(cycle_isOk(&c, 3));  // above, diff 8 > press

    // --- exhaustive cross-check over all slots 0..CYCLE_SLOT_CNT-1 ----------
    // Even slots and slot 0 are invalid (cycle_check_slot) -> always false;
    // valid odd slots follow cycle_difference > press.
    for (int8_t rx = 0; rx < CYCLE_SLOT_CNT; rx++) {
        const bool valid = (rx > 0) && (rx % 2 == 1);
        for (int ss = 0; ss < total; ss++) {
            c.subSlot = (int8_t)ss;
            const bool expected = valid && (cycle_difference(&c, rx) > cycle_press(&c));
            EXPECT_EQ(cycle_isOk(&c, rx), expected)
                << "subSlot=" << ss << " rxSlot=" << (int)rx;
        }
    }
}

