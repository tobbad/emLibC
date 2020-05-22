
#include <string.h>
#include <stdint.h>
#include "memmgr.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;


static const uint32_t MEM_SIZE = 1024;
static uint8_t memory[MEM_SIZE];


class MemmgrInitTest : public ::testing::Test {
    protected:

    void SetUp() override {
    }

    void TearDown() override
    {
    }
};

/* Specification:
 */
TEST_F(MemmgrInitTest, CheckNoMemory)
{
    //elres_t res = memmgr_init(0,0);

    //EXPECT_GT(EMLIB_ERROR, res);
    FAIL();
}
