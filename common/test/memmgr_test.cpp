
#include <string.h>
#include <stdint.h>
#include "memmgr.h"

#include "gtest/gtest.h"



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
    //em_msg res = memmgr_init(0,0);

    //EXPECT_GT(EM_ERR, res);
    FAIL();
}
