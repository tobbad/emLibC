/*
 * slip_test.cpp
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */
#include <cstdio>
#include <string.h>
#include <stdint.h>
#include "simple_test.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;


class simple_test : public ::testing::Test {
    protected:

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(SIMPLE_1, SeeIfItCompiles)
{
	uint8_t res;
	for (uint16_t i=0;i<255;i++){
		res = square(i);
		EXPECT_EQ(res, i*i);
	}
	EXPECT_EQ(res, 1);
}
