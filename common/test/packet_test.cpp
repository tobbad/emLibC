/*
 * packet_test.cpp
 *
 *  Created on: 14.08.2020
 *      Author: badi
 */
#include <cstdio>
#include <string.h>
#include <stdint.h>
#include "device.h"
#include "packet.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;



class PacketTest : public ::testing::Test {
    protected:

    void SetUp() override
    {

    }

    void TearDown() override
    {
    }
};


TEST_F(PacketTest, initSucceeds)
{
    elres_t res = packet_init();

    EXPECT_EQ(EMLIB_OK, res);
}

