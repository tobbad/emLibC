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
#include "slip.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;


class SlipTest : public ::testing::Test {
    protected:

    void SetUp() override
    {
        memory_device_reset(&memory_device_buffer);
        slip_init();
    }

    void TearDown() override
    {
    }
};

TEST_F(SlipTest, ErrorIfSlipStartWithNullDevice)
{
    slip_handle_e hdl = slip_start(NULL, SLIP_ENCODE_SIMPLE);

    EXPECT_EQ(SLIP_HANDLE_ERROR, hdl);
}

TEST_F(SlipTest, ErrorIfSlipStartWithNullWriteFunc)
{
    device_t test = memory_device;
    test.write = NULL;

    slip_handle_e hdl = slip_start(&test, SLIP_ENCODE_SIMPLE);

    EXPECT_EQ(SLIP_HANDLE_ERROR, hdl);
}

TEST_F(SlipTest, OkIfSlipStartWithValidFunction)
{
    slip_handle_e hdl = slip_start(&memory_device, SLIP_ENCODE_SIMPLE);
    EXPECT_EQ(SLIP_HANDLE_0, hdl);

    hdl = slip_start(&memory_device, SLIP_ENCODE_SIMPLE);
    EXPECT_EQ(SLIP_HANDLE_1, hdl);

    hdl = slip_start(&memory_device, SLIP_ENCODE_SIMPLE);
    EXPECT_EQ(SLIP_HANDLE_ERROR, hdl);

}

TEST_F(SlipTest, ErrorOnInvalidState)
{
    slip_handle_e hdl = slip_start(&memory_device, SLIP_STATE_CNT);

    EXPECT_EQ(SLIP_HANDLE_ERROR, hdl);
}

TEST_F(SlipTest, GetStartValueInReceiveBuffer)
{
    uint8_t value = 42;
    slip_handle_e hdl = slip_start(&memory_device, SLIP_ENCODE_SIMPLE);
    EXPECT_EQ(SLIP_HANDLE_0, hdl);
    elres_t res = slip_write(hdl, &value, 1);
    uint16_t size = slip_end(hdl);
    EXPECT_EQ(EMLIB_OK, res);
    EXPECT_EQ(3, size);
    EXPECT_EQ(SLIP_PKT_LIMIT, rcv_buffer[0]);
    EXPECT_EQ(value, rcv_buffer[1]);
    EXPECT_EQ(SLIP_PKT_LIMIT, rcv_buffer[2]);
}

TEST_F(SlipTest, CheckEncodeMapping)
{
    uint8_t buf[] = {0xC0};

    elres_t res;
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

            EXPECT_EQ(EMLIB_OK, res);
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


TEST_F(SlipTest, CheckDecodeMapping)
{
    uint8_t buf[10];

    uint16_t res;
    slip_function_t encode_map[] = {SLIP_ENCODE_SIMPLE, SLIP_ENCODE_OOF_FLOW_CONTROL};
    slip_function_t decode_map[] = {SLIP_DECODE_SIMPLE, SLIP_DECODE_OOF_FLOW_CONTROL};
    for (uint8_t enc =0;enc<2;enc++)
    {
        for (uint16_t value = 0; value<256;value++)
        {
            uint8_t encode_size = 0;
            memory_device_reset(&memory_device_buffer);
            slip_handle_e hdl_e = slip_start(&memory_device, encode_map[enc]);
            slip_handle_e hdl_dut = slip_start(&memory_device, decode_map[enc]);

            buf[0] = value;
            slip_write(hdl_e, buf, 1);
            encode_size = slip_end(hdl_e);

            memcpy(buf, memory_device_buffer.mem, encode_size);
            EXPECT_LE(encode_size, 4);
            /* printf("0x%02x ->", value);
            for (uint8_t i=0;i<encode_size;i++)
            {
                printf(" 0x%02x", buf[i]);
            }
            printf("\n");*/
            memory_device_reset(&memory_device_buffer);
            slip_write(hdl_dut, buf, encode_size);
            res = slip_end(hdl_dut);
            EXPECT_EQ(1, res);
            EXPECT_EQ(value, rcv_buffer[0]) << "State " << decode_map[enc];
        }
    }
}


TEST_F(SlipTest, AllUint8ValueCodec)
{
    static const uint32_t PRSIZE = (6+8*3+2*8*3+3+16+1)*17+1;
    elres_t res;
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
    memory_device_reset(&memory_device_buffer);
    slip_handle_e hdl_dut = slip_start(&memory_device, SLIP_ENCODE_SIMPLE);
    res = slip_write(hdl_dut, buffer, value);
    size = slip_end(hdl_dut);
    EXPECT_EQ(size, 256+/*Start/Stop 0x0c*/2+/*Count of ESC*/2);
    EXPECT_EQ(EMLIB_OK, res);
    EXPECT_EQ(SLIP_PKT_LIMIT, memory_device_buffer.mem[size-1]);
    memcpy(buffer, memory_device_buffer.mem, size);

    /*
    printf("Serialized %d bytes\n", size);
    to_hex(prbuf, sizeof(prbuf), buffer, size, true);
    printf(prbuf);
    */

    memory_device_reset(&memory_device_buffer);
    hdl_dut = slip_start(&memory_device, SLIP_DECODE_SIMPLE);
    res = slip_write(hdl_dut, buffer, size);
    size = slip_end(hdl_dut);
    EXPECT_EQ(EMLIB_OK, res);
    EXPECT_EQ(value, size);

    //printf("Deserialized %d bytes\n", size);
    //to_hex(prbuf, sizeof(prbuf), rbuf.buffer, size, true);
    //printf(prbuf);
    for (value = 0; value<256; value++)
    {
        EXPECT_EQ(value, memory_device_buffer.mem[value]) << "At " << value;
    }
}

struct slip_tc_t_ {
    const slip_function_t fun;
    const uint8_t *in;
    const uint16_t in_size;
    const uint8_t *out;
    const uint16_t out_size;
};

static const uint8_t tc0_in[] = {192,192,0,192};
static const uint8_t tc0_out[] = {0};
static const uint8_t tc1_in[] = {192,192,0,192};
static const uint8_t tc1_out[] = {0};
static const slip_tc_t_ tc[]={
        {SLIP_DECODE_SIMPLE,           tc0_in, sizeof(tc0_in), tc0_out, sizeof(tc0_out)},
        {SLIP_DECODE_OOF_FLOW_CONTROL, tc0_in, sizeof(tc0_in), tc0_out, sizeof(tc0_out)},
};
static const uint8_t tc_cnt = ELCNT(tc);

TEST_F(SlipTest, TestcaseRunner)
{
    uint8_t tc_idx;
    for (tc_idx=0;tc_idx<tc_cnt;tc_idx++)
    {
        uint16_t dec_size;
        slip_function_t fun = tc[tc_idx].fun;
        const uint8_t * in = tc[tc_idx].in;
        uint16_t in_size = tc[tc_idx].in_size;
        const uint8_t * exp = tc[tc_idx].out;
        uint16_t exp_size = tc[tc_idx].out_size;
        slip_handle_e hdl_dut = slip_start(&memory_device, fun);
        /*
         * Set up the test
         */
        memory_device_reset(&memory_device_buffer);
        slip_write(hdl_dut, in, in_size);
        dec_size = slip_end(hdl_dut);
        /*
         *
         */
        EXPECT_EQ(exp_size, dec_size) << "Testcase " << tc_idx;
        if (exp_size == dec_size)
        {
            for (uint16_t i=0;i<dec_size;i++)
            {
                EXPECT_EQ(exp[i], rcv_buffer[i]) << "Testcase " << tc_idx;
            }
        }

    }

}

