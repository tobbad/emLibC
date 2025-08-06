/*
 * slip_test.cpp
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */
#include <cstdio>
#include <string.h>
#include <stdint.h>
#include "device.h"
#include "state.h"


#include "gtest/gtest.h"
state_t state;


class StateTest : public ::testing::Test {
    protected:

    void SetUp() override
    {
    }

    void TearDown()
    {
    }
};

TEST_F(StateTest, TestNullTestCorrectLabelsAndInitState){
     em_msg res = state_init(NULL);
     uint32_t stat;
    EXPECT_EQ(res, EM_ERR);
     res = state_init(&state);
    EXPECT_EQ(state.first, 0);
    EXPECT_EQ(state.cnt , MAX_STATE_CNT);
    EXPECT_EQ(state.dirty, false);
    EXPECT_EQ(state.clabel.cmd, 0);

    for (uint8_t i=0;i<MAX_STATE_CNT;i++){
         EXPECT_EQ(state.state[i], OFF);
         EXPECT_EQ(state.label[i], int2hchar(i));
    }
    stat = state_get_u32(&state);
     EXPECT_EQ(stat, 0);

}

TEST_F(StateTest, Teststate_ch2idx){
     state_t state;
     state_init(&state);
     char ch;

     for (uint8_t i=0;i<MAX_STATE_CNT;i++){
         ch = state.label[i];
         uint8_t r = state_ch2idx(&state, ch);
         EXPECT_EQ(i, r);
     }

}

TEST_F(StateTest, TestSetAllStates){
     state_t state1;
     state_init(&state1);
     uint32_t stat;

    for (uint8_t i=0;i<MAX_STATE_CNT;i++){
         for (uint8_t j=0;j<STATE_CNT;j++){
              state_set_value(&state1, i, (key_state_e)j);
              stat = state_get_u32(&state);
              printf("%d %s: %08x\n", i, key2char[j], stat);
              stat =state_get_state(&state, state.label[i]);
              EXPECT_EQ(j, stat);
         }
     }
    stat = state_get_u32(&state);
    EXPECT_EQ(stat, 0);

}

