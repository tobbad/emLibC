
#include <string.h>
#include <stdint.h>
#include "ringbuffer.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;


class RingbufferInitTest : public ::testing::Test {
    protected:

    void SetUp() override {

    }

    void TearDown() override
    {
    }



};

/*
 * Test to succeed:
 * - New returns different and valid handles up to EMCBOR_INSTANCE_COUNT
 * - Free succeeds on allocated
 * - Free does returns error on not allocated
 */
TEST_F(RingbufferInitTest, fail)
{
    FAIL();
 }
