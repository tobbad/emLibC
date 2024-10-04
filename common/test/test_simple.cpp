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
#include "memory_device.h"
#include "simple_test.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;


class SlipTest : public ::testing::Test {
    protected:

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(SIMPLE_1, ErrorIfSlipStartWithNullDevice)
{
	uint8_t res;
	for (uint16_t i=0;i++;i<255){
		res = square(i);
		EXPECT_EQ(res, i*i);
	}
	EXPECT_EQ(res, 1);
}
