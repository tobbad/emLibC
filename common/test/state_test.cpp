/*
 * state_test.cpp
 */
#include "state.h"
#include "device.h"
#include <cstdio>
#include <stdint.h>
#include <string.h>
#include "gtest/gtest.h"

// ---------------------------------------------------------------------------
// Hilfsfunktion: prüft ob ein key_state_e-Rückgabewert ein Fehler ist.
// state_get_key_by_lbl/idx liefern STATE_CNT als Fehlersentinel.
// ---------------------------------------------------------------------------
static inline bool is_key_err(int v) {
    return (v < 0 || (key_state_e)v >= STATE_CNT);
}

// ---------------------------------------------------------------------------
// Fixture: frisch initialisierter state
// ---------------------------------------------------------------------------
class StateTest : public ::testing::Test {
  protected:
    state_t state;
    void SetUp() override { state_init(&state); }
};

// ---------------------------------------------------------------------------
// state_init / state_reset
// ---------------------------------------------------------------------------
TEST(StateInitTest, NullPtrReturnsError) {
    EXPECT_EQ(state_init(nullptr), EM_ERR);
}

TEST(StateInitTest, DefaultFieldsAfterInit) {
    state_t s;
    ASSERT_EQ(state_init(&s), EM_OK);
    EXPECT_EQ(s.first,      0);
    EXPECT_EQ(s.cnt,        MAX_STATE_CNT);
    EXPECT_EQ(s.dirty,      false);
    EXPECT_EQ(s.clabel.cmd, 0u);
}

TEST(StateInitTest, AllKeysOffAfterInit) {
    state_t s;
    state_init(&s);
    for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
        EXPECT_EQ(s.state[i], OFF) << "index " << (int)i;
    }
}

TEST(StateInitTest, LabelsAreHexDigits) {
    state_t s;
    state_init(&s);
    for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
        EXPECT_EQ(s.label[i], int2hchar(i)) << "index " << (int)i;
    }
}

TEST(StateInitTest, GetU32IsZeroAfterInit) {
    state_t s;
    state_init(&s);
    EXPECT_EQ(state_get_u32(&s), 0u);
}

TEST(StateInitTest, ResetClearsAllKeys) {
    state_t s;
    state_init(&s);
    state_propagate_by_idx(&s, 0);
    state_propagate_by_idx(&s, 1);
    ASSERT_EQ(state_reset(&s), EM_OK);
    for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
        EXPECT_EQ(state_get_key_by_idx(&s, i), OFF) << "index " << (int)i;
    }
    EXPECT_EQ(s.dirty, false);
}

// ---------------------------------------------------------------------------
// state_ch2idx
// ---------------------------------------------------------------------------
TEST_F(StateTest, Ch2IdxFindsAllLabels) {
    for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
        EXPECT_EQ(state_ch2idx(&state, state.label[i]), (int8_t)i);
    }
}

TEST_F(StateTest, Ch2IdxMissingCharReturnsError) {
    EXPECT_EQ(state_ch2idx(&state, '!'), EM_ERR);
}

// ---------------------------------------------------------------------------
// state_set / state_get (direkt)
// ---------------------------------------------------------------------------
TEST_F(StateTest, SetAndGetOFF) {
    EXPECT_EQ(state_set(&state, 0, OFF),      EM_OK);
    EXPECT_EQ(state_get(&state, 0),           (key_state_e)OFF);
}

TEST_F(StateTest, SetAndGetBLINKING) {
    EXPECT_EQ(state_set(&state, 0, BLINKING), EM_OK);
    EXPECT_EQ(state_get(&state, 0),           (key_state_e)BLINKING);
}

TEST_F(StateTest, SetAndGetON) {
    EXPECT_EQ(state_set(&state, 0, ON),       EM_OK);
    EXPECT_EQ(state_get(&state, 0),           (key_state_e)ON);
}

TEST_F(StateTest, SetOutOfRangeReturnsError) {
    EXPECT_EQ(state_set(&state, MAX_STATE_CNT, ON), EM_ERR);
}

// ---------------------------------------------------------------------------
// state_propagate_by_idx / state_propagate_by_lbl
// ---------------------------------------------------------------------------
TEST_F(StateTest, PropagateByIdxCyclesOFF_BLINKING_ON) {
    for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
        EXPECT_EQ(state_get_key_by_idx(&state, i), OFF);
        EXPECT_EQ(state_propagate_by_idx(&state, i), EM_TRUE);
        EXPECT_EQ(state_get_key_by_idx(&state, i), BLINKING);
        EXPECT_EQ(state_propagate_by_idx(&state, i), EM_TRUE);
        EXPECT_EQ(state_get_key_by_idx(&state, i), ON);
    }
    EXPECT_EQ(state_get_u32(&state), 0xAAAAAAAAu);
}

TEST_F(StateTest, PropagateByLblCyclesOFF_BLINKING_ON) {
    for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
        char lbl = state.label[i];
        EXPECT_EQ(state_get_key_by_lbl(&state, lbl), OFF);
        state_propagate_by_lbl(&state, lbl);
        EXPECT_EQ(state_get_key_by_lbl(&state, lbl), BLINKING);
        state_propagate_by_lbl(&state, lbl);
        EXPECT_EQ(state_get_key_by_lbl(&state, lbl), ON);
    }
    EXPECT_EQ(state_get_u32(&state), 0xAAAAAAAAu);
}

TEST_F(StateTest, PropagateCrossIdxLbl) {
    for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
        state_propagate_by_lbl(&state, state.label[i]);
        EXPECT_EQ(state_get_key_by_idx(&state, i), BLINKING);
        state_propagate_by_lbl(&state, state.label[i]);
        EXPECT_EQ(state_get_key_by_idx(&state, i), ON);
    }
    EXPECT_EQ(state_get_u32(&state), 0xAAAAAAAAu);
}

TEST_F(StateTest, PropagateCrossLblIdx) {
    for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
        state_propagate_by_idx(&state, i);
        EXPECT_EQ(state_get_key_by_lbl(&state, state.label[i]), BLINKING);
        state_propagate_by_idx(&state, i);
        EXPECT_EQ(state_get_key_by_lbl(&state, state.label[i]), ON);
    }
    EXPECT_EQ(state_get_u32(&state), 0xAAAAAAAAu);
}

TEST_F(StateTest, PropagateSetsStateDirty) {
    EXPECT_EQ(state.dirty, false);
    state_propagate_by_idx(&state, 0);
    EXPECT_EQ(state.dirty, true);
}

// ---------------------------------------------------------------------------
// state_dirty / state_undirty
// ---------------------------------------------------------------------------
TEST_F(StateTest, SetDirtyAndUndirty) {
    EXPECT_EQ(state_set_dirty(&state),   EM_OK);
    EXPECT_EQ(state_get_dirty(&state),   true);
    EXPECT_EQ(state_set_undirty(&state), EM_OK);
    EXPECT_EQ(state_get_dirty(&state),   false);
    EXPECT_EQ(state.clabel.cmd,          0u);
}

// ---------------------------------------------------------------------------
// state_set_first / state_set_cnt / eingeschränkter Bereich
// ---------------------------------------------------------------------------
TEST_F(StateTest, LimitedRangePropagate) {
    const uint8_t low = 2, up = 6;
    state_set_first(&state, low);
    EXPECT_EQ(state_get_first(&state), low);
    state_set_cnt(&state, up);
    EXPECT_EQ(state_get_cnt(&state), up);

    for (uint8_t i = 0; i < MAX_STATE_CNT; i++) {
        int stat = state_get_key_by_lbl(&state, state.label[i]);
        if (is_key_err(stat)) continue;
        EXPECT_EQ(stat, (int)OFF);

        state_propagate_by_lbl(&state, state.label[i]);
        stat = state_get_key_by_idx(&state, i);
        if (is_key_err(stat)) continue;
        EXPECT_EQ(stat, (int)BLINKING);

        state_propagate_by_idx(&state, i);
        stat = state_get_key_by_lbl(&state, state.label[i]);
        if (!is_key_err(stat)) {
            EXPECT_EQ(stat, (int)ON);
        }
    }
    EXPECT_EQ(state_get_u32(&state), 0x0000AAA0u);
}

// ---------------------------------------------------------------------------
// state_key_diff
// ---------------------------------------------------------------------------
class StateKeyDiffTest : public ::testing::Test {};

TEST_F(StateKeyDiffTest, SameStateIsOFF) {
    EXPECT_EQ(state_key_diff(OFF,      OFF),      OFF);
    EXPECT_EQ(state_key_diff(BLINKING, BLINKING), OFF);
    EXPECT_EQ(state_key_diff(ON,       ON),       OFF);
}

TEST_F(StateKeyDiffTest, ForwardDiff) {
    EXPECT_EQ(state_key_diff(OFF,      BLINKING), BLINKING);
    EXPECT_EQ(state_key_diff(OFF,      ON),       ON);
    EXPECT_EQ(state_key_diff(BLINKING, ON),       BLINKING);
}

TEST_F(StateKeyDiffTest, WrapAroundDiff) {
    EXPECT_EQ(state_key_diff(BLINKING, OFF), ON);
    EXPECT_EQ(state_key_diff(ON,       OFF), BLINKING);
    EXPECT_EQ(state_key_diff(ON,       BLINKING), ON);
}

// ---------------------------------------------------------------------------
// state_copy / state_merge / state_is_same / state_diff / state_add
// ---------------------------------------------------------------------------
class StateCopyMergeTest : public ::testing::Test {
  protected:
    state_t s1, s2, ref;
    void SetUp() override {
        state_init(&s1); state_init(&s2); state_init(&ref);
        // s1[i] = i/3 → OFF,OFF,OFF,BLI,BLI,BLI,ON,ON,ON
        // s2[i] = i%3 → OFF,BLI,ON, OFF,BLI,ON, OFF,BLI,ON
        for (uint8_t i = 0; i < 9; i++) {
            state_set_key_by_idx(&s1, i, (key_state_e)(i / 3));
            state_set_key_by_idx(&s2, i, (key_state_e)(i % 3));
        }
    }
};



TEST_F(StateCopyMergeTest, MergeResult) {
    state_t check;
    em_msg res;
    bool resb;
    res = state_init(&check);
    EXPECT_EQ(res, EM_OK);
    for (uint8_t nr=0;nr<MAX_STATE_CNT;nr++){
        res = state_reset(&check);
        EXPECT_EQ(res, EM_OK);
    	res = state_set_key_by_idx(&check, nr , BLINKING);
        EXPECT_EQ(res, EM_OK);
        //state_print(&ref, "ref", true);
        res = state_reset(&ref);
        EXPECT_EQ(res, EM_OK);
        resb = state_merge(&ref, &check);
        EXPECT_EQ(resb, 1);
        res  = state_is_same(&check, &ref);
        EXPECT_EQ(res, EM_OK);
        res = state_reset(&check);
        EXPECT_EQ(res, EM_OK);
    	res = state_set_key_by_idx(&check, nr , ON);
        EXPECT_EQ(res, EM_OK);
        res = state_reset(&ref);
        EXPECT_EQ(res, EM_OK);
        resb = state_merge(&ref, &check);
        EXPECT_EQ(resb, 1);
        res  = state_is_same(&check, &ref);
        EXPECT_EQ(res, EM_OK);
    }
}


TEST_F(StateCopyMergeTest, PropagateResult) {
    state_t prop;
    em_msg res;
    bool resb;
    state_init(&prop);
    res = state_copy(&s2, &prop);
    state_print(&prop, "ref", false);
    EXPECT_EQ(res, EM_OK);
    state_print(&s1, "add", false);
    resb = state_add(&prop, &s1);
    state_print(&prop, "outState_a", false);
    EXPECT_EQ(resb, 1);

    state_set_key_by_idx(&ref, 0, OFF);
    state_set_key_by_idx(&ref, 1, BLINKING);
    state_set_key_by_idx(&ref, 2, ON);
    state_set_key_by_idx(&ref, 3, BLINKING);
    state_set_key_by_idx(&ref, 4, ON);
    state_set_key_by_idx(&ref, 5, OFF);
    state_set_key_by_idx(&ref, 6, ON);
    state_set_key_by_idx(&ref, 7, OFF);
    state_set_key_by_idx(&ref, 8, BLINKING);
    state_print(&ref, "ref", false);
    EXPECT_EQ(state_is_same(&prop, &ref), EM_OK);
}

TEST_F(StateCopyMergeTest, DiffVec) {
    state_t diff;
    state_init(&diff);
    state_diff(&s1, &s2, &diff);

    state_set_key_by_idx(&ref, 0, OFF);
    state_set_key_by_idx(&ref, 1, BLINKING);
    state_set_key_by_idx(&ref, 2, ON);
    state_set_key_by_idx(&ref, 3, ON);
    state_set_key_by_idx(&ref, 4, OFF);
    state_set_key_by_idx(&ref, 5, BLINKING);
    state_set_key_by_idx(&ref, 6, BLINKING);
    state_set_key_by_idx(&ref, 7, ON);
    state_set_key_by_idx(&ref, 8, OFF);
    EXPECT_EQ(state_is_same(&ref, &diff), true);
}

TEST_F(StateCopyMergeTest, AddDiffConverges) {
    // state1 + diff(state1, state2) soll state2 ergeben
    state_t diff;
    state_init(&diff);
    state_diff(&s1, &s2, &diff);
    state_add(&s1, &diff);
    EXPECT_EQ(state_is_same(&s1, &s2), true);
}

TEST_F(StateCopyMergeTest, IsSameDetectsOneDifference) {
    state_t copy;
    state_copy(&s1, &copy);
    state_set_key_by_idx(&copy, 0, ON);  // flip one key
    EXPECT_NE(state_is_same(&s1, &copy), EM_OK);
}
