/*
 * slip_test.cpp
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */
#include <string.h>
#include <stdint.h>
#include "buffer.h"
#include "buffer_pool.h"


#include "gtest/gtest.h"
buffer_t *buffer;


class BufferTest : public ::testing::Test {
    protected:

    void SetUp() override
    {
        
    }

    void TearDown()
    {
    }
};

TEST_F(BufferTest, ErrorIfStateNotGood)
{
    buffer_t * buf= buffer_new(32);

    EXPECT_NE(buf, (buffer_t *)NULL);
}
