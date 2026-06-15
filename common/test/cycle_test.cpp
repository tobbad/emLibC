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
class cycleTest : public ::testing::Test {
  protected:
    cycle_t cycle;
    void SetUp() override {}
};

// ---------------------------------------------------------------------------
// cycle_init / cycle_reset
// ---------------------------------------------------------------------------
TEST(CycleTest, NullPtrReturnsError) { EXPECT_EQ(cycle_init(nullptr), EM_ERR); }

TEST(CycleTest, CheckSetSlot) {
    uint8_t slot;
    cycle_t c;
    ASSERT_EQ(cycle_init(&c), EM_OK);
    for (slot = 0; slot <SLOT_CNT; slot++) {
        if (slot % 2 == 1) {
            ASSERT_EQ(cycle_set_slot(&c, slot), EM_OK);
        } else {
            ASSERT_EQ(cycle_set_slot(&c, slot), EM_ERR);
        }
    }
    for (; slot < 2 *SLOT_CNT; slot++) {
        ASSERT_EQ(cycle_set_slot(&c, slot), EM_ERR);
    }
}

TEST(CycleTest, CheckCycleIncrement) {
    cycle_t c;
    uint32_t cycle;
    uint32_t subSlot;
    int8_t slot = 3;
    ASSERT_EQ(cycle_init(&c), EM_OK);
    ASSERT_EQ(cycle_set_slot(nullptr, 1), EM_ERR);
    ASSERT_EQ(cycle_set_slot(&c, slot), EM_OK);
    ASSERT_EQ(c.subSlot, slot * SUB_SLOT_CNT);
    system_state_e sync_state = SYNC_RESET;
    cycle_increment(&c, &sync_state);
    ASSERT_EQ(sync_state, SYNC_RESET);

    sync_state = BOOT_UP;
    cycle_increment(&c, &sync_state);
    ASSERT_EQ(c.subSlot, slot * SUB_SLOT_CNT);
    ASSERT_EQ(sync_state, BOOT_UP);

    sync_state = SLOT;
    cycle_increment(&c, &sync_state);
    ASSERT_EQ(c.subSlot, slot * SUB_SLOT_CNT);
    ASSERT_EQ(sync_state, SLOT);

    sync_state = FREQBAND;
    cycle_increment(&c, &sync_state);
    ASSERT_EQ(c.subSlot, slot * SUB_SLOT_CNT);
    ASSERT_EQ(sync_state, FREQBAND);

    sync_state = FREQUENCY_OFFSET;
    cycle_increment(&c, &sync_state);
    ASSERT_EQ(c.subSlot, slot * SUB_SLOT_CNT);
    ASSERT_EQ(sync_state, FREQUENCY_OFFSET);
    sync_state = SYNCHRONIZE;
    cycle_increment(&c, &sync_state);
    ASSERT_EQ(c.subSlot, slot * SUB_SLOT_CNT);
    ASSERT_EQ(sync_state, SYNCHRONIZE_READY);
    cycle_reset(&c);
    // cycle_increment advances first and reports afterwards, so start one tick
    // below the wrap: the first call then lands on subSlot 0 (slot 0, sub slot
    // 0) and the loop variables match the reported values 1:1.
    c.subSlot = SUB_SLOT_CNT * SLOT_CNT - 1;
    for (cycle = 0; cycle <= UINT16_MAX; cycle++) {
        for (slot = 0; slot < SLOT_CNT; slot++) {
            for (uint8_t ss = 0; ss < SUB_SLOT_CNT; ss++) {
                cycle_increment(&c, &sync_state);
                ASSERT_EQ(sync_state, SYNCHRONIZE_READY);
                ASSERT_EQ(c.subSlot, ss + slot * SUB_SLOT_CNT);
                ASSERT_EQ(c.actSlot, slot);
                ASSERT_EQ(c.sSlot, ss);
                ASSERT_EQ(c.cycle, cycle);
            }
        }
    }
    // The loop above left the cycle counter at its uint16_t maximum; one more
    // wrap of subSlot drives cycle past 65535 and overflows it back to 0.
    cycle_increment(&c, &sync_state);
    ASSERT_EQ(sync_state, SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot,0);
    ASSERT_EQ(c.actSlot, 0);
    ASSERT_EQ(c.sSlot, 0);
    ASSERT_EQ(c.cycle, 0);

}

// ---------------------------------------------------------------------------
// cycle_difference: signed sub-slot distance from the current position to the
// start of rxSlot's region, shortest way around the SUB_SLOT_CNT*SLOT_CNT ring.
// Negative => rxSlot lies ahead of us, positive => rxSlot lies behind us.
// ---------------------------------------------------------------------------
TEST(CycleTest, CycleDifference) {
    const int total = SUB_SLOT_CNT * SLOT_CNT; // 128 sub-slots per cycle
    const int half = total / 2;                // 64

    // NULL / uninitialised guards return 0.
    EXPECT_EQ(cycle_difference(nullptr, 0), 0);
    cycle_t u{};
    u.init = false;
    EXPECT_EQ(cycle_difference(&u, 0), 0);

    cycle_t c;
    ASSERT_EQ(cycle_init(&c), EM_OK);

    // Spot checks matching the documented behaviour.
    c.subSlot = 0;
    EXPECT_EQ(cycle_difference(&c, 0), 0); // sitting at the start of slot 0
    c.subSlot = 15 * SUB_SLOT_CNT;         // slot 15, sub-slot 0 -> linear 120
    EXPECT_EQ(cycle_difference(&c, 0), -8); // rxSlot 0 starts 8 sub-slots ahead
    c.subSlot = SUB_SLOT_CNT;               // slot 1, sub-slot 0
    EXPECT_EQ(cycle_difference(&c, 0), 8);  // rxSlot 0 is 8 sub-slots behind us
    c.subSlot = 3;                          // slot 0, sub-slot 3
    EXPECT_EQ(cycle_difference(&c, 0), 3);  // 3 sub-slots into rxSlot's region
    c.subSlot = 0;
    EXPECT_EQ(cycle_difference(&c, 1), -8); // next slot starts 8 ahead
    c.subSlot = half;                       // exactly half a cycle away
    EXPECT_EQ(cycle_difference(&c, 0), -half); // folds to the negative edge

    // Exhaustive cross-check against a reference for every position/slot pair.
    for (int ss = 0; ss < total; ss++) {
        for (int8_t rx = 0; rx < SLOT_CNT; rx++) {
            c.subSlot = (int8_t)ss;
            int expect = (ss - rx * SUB_SLOT_CNT) % total;
            if (expect < 0) expect += total;
            if (expect >= half) expect -= total;
            EXPECT_EQ(cycle_difference(&c, rx), expect)
                << "subSlot=" << ss << " rxSlot=" << (int)rx;
        }
    }
}
