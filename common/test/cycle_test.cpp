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
    cycle_increment(&c, &sync_state);
    ASSERT_EQ(sync_state, SYNCHRONIZE_READY);
    ASSERT_EQ(c.subSlot,0);
    ASSERT_EQ(c.actSlot, 0);
    ASSERT_EQ(c.sSlot, 0);
    ASSERT_EQ(c.cycle, 0);

}
