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


class _Simple_Test : public ::testing::Test {
    protected:

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(_Simple_Test, SeeIfItCompiles)
{
	uint8_t res;
	uint16_t i;
	for (i=0;i<16;i++){
		res = square(i);
		printf("%d *%d = %d\n", i, i, res);
		EXPECT_EQ(res, i*i) << "Errro";
	}
	res = square(i);
	printf("%d *%d = %d\n", i, i, res);
	EXPECT_EQ(res, 0);
}
