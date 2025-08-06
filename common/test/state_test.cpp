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

TEST_F(StateTest, TestNullTestCorrectLabelsAndInitState)
{
	em_msg res = state_init(NULL);
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
}

