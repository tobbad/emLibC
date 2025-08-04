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
#include "buffer.h"
#include "buffer_pool.h"


#include "gtest/gtest.h"
buffet_pool_t pool;

using ::testing::Return;


class StateTest : public ::testing::Test {
    protected:

    void SetUp() override
    {
        state_init(&state);
    }

    void TearDown
    {
    }
};

TEST_F(StateTest, ErrorIfStateNotGood)
{
    slip_handle_e hdl = slip_start(NULL, SLIP_ENCODE_SIMPLE);

    EXPECT_EQ(SLIP_HANDLE_ERROR, hdl);
}

TEST_F(StateTest, ErrorIfSlipStartWithNullWriteFunc)
{
    device_t test = memory_device;
    test.write = NULL;

    slip_handle_e hdl = slip_start(&test, SLIP_ENCODE_SIMPLE);

    EXPECT_EQ(SLIP_HANDLE_ERROR, hdl);
}

TEST_F(StateTest, OkIfSlipStartWithValidFunction)
{
    slip_handle_e hdl = slip_start(&memory_device, SLIP_ENCODE_SIMPLE);
    EXPECT_EQ(SLIP_HANDLE_0, hdl);

    hdl = slip_start(&memory_device, SLIP_ENCODE_SIMPLE);
    EXPECT_EQ(SLIP_HANDLE_1, hdl);

    hdl = slip_start(&memory_device, SLIP_ENCODE_SIMPLE);
    EXPECT_EQ(SLIP_HANDLE_ERROR, hdl);

}

TEST_F(StateTest, ErrorOnInvalidState)
{
    slip_handle_e hdl = slip_start(&memory_device, SLIP_STATE_CNT);

    EXPECT_EQ(SLIP_HANDLE_ERROR, hdl);
}

TEST_F(StateTest, GetStartValueInReceiveBuffer)
{
    uint8_t value = 42;
    slip_handle_e hdl = slip_start(&memory_device, SLIP_ENCODE_SIMPLE);
    EXPECT_EQ(SLIP_HANDLE_0, hdl);
    em_msg res = slip_write(hdl, &value, 1);
    uint16_t size = slip_end(hdl);
    EXPECT_EQ(EM_OK, res);
    EXPECT_EQ(3, size);
    EXPECT_EQ(SLIP_PKT_LIMIT, rcv_buffer[0]);
    EXPECT_EQ(value, rcv_buffer[1]);
    EXPECT_EQ(SLIP_PKT_LIMIT, rcv_buffer[2]);
}

TEST_F(StateTest, CheckEncodeMapping)
{
    uint8_t buf[] = {0xC0};

    em_msg res;
    uint16_t ser_size;
    for (uint16_t value = 0; value<256;value++)
    {
        bool is_esc = false;
        uint8_t escMapIdx;
        slip_function_t encode_map[] = {SLIP_ENCODE_SIMPLE, SLIP_ENCODE_OOF_FLOW_CONTROL};
        for (uint8_t enc =0;enc<2;enc++)
        {
            uint16_t idx=1;
            memory_device_reset(&memory_device_buffer);
            slip_init();
            slip_handle_e hdl = slip_start(&memory_device, encode_map[enc]);
            EXPECT_NE(SLIP_HANDLE_ERROR, hdl);

            for (escMapIdx = 0; escMapIdx<2*(enc+1);escMapIdx++)
            {
                if (value == slip_map[escMapIdx][0])
                {
                    is_esc = true;
                    break;
                }
            }
            buf[0] = (uint8_t)value;
            res = slip_write(hdl, buf, 1);
            ser_size = slip_end(hdl);

            EXPECT_EQ(EM_OK, res);
            if (is_esc)
            {
                EXPECT_EQ(4, ser_size);
                EXPECT_EQ(SLIP_ESCAPE, rcv_buffer[idx++]);
                EXPECT_EQ(slip_map[escMapIdx][1], rcv_buffer[idx++]);
            } else {
                EXPECT_EQ(3, ser_size);
                EXPECT_EQ(value, rcv_buffer[idx++]) << "State " << encode_map[enc];
            }

            EXPECT_EQ(SLIP_PKT_LIMIT, rcv_buffer[idx++]) << "State " << encode_map[enc];
        }
    }
}


TEST_F(StateTest, CheckDecodeMapping)
{
    uint8_t buf[10];

    uint16_t res;
}


TEST_F(StateTest, AllUint8ValueCodec)
{
    static const uint32_t PRSIZE = (6+8*3+2*8*3+3+16+1)*17+1;
    em_msg res;
    uint16_t size;
    uint8_t buffer[1024];
    //char prbuf[PRSIZE];
    uint16_t value;
    for (value = 0; value<256; value++)
    {
        buffer[value] = value;
    }
    /*
     * Set up the test
     */
}

TEST_F(StateTest, TestcaseRunner)
{
    uint8_t tc_idx;
    for (tc_idx=0;tc_idx<tc_cnt;tc_idx++)
    {
     }

}

