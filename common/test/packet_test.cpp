/*
 * packet_test.cpp
 *
 *  Created on: 14.08.2020
 *      Author: badi
 */
#include <cstdio>
#include <string.h>
#include <stdint.h>
#include "common.h"
#include "device.h"
#include "memory_device.h"
#include "packet.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;

#define BLK_SIZE 64
#define BYTE_VAL(i) (i+10)

class PacketTest : public ::testing::Test {
    protected:

    uint8_t memory[BLK_SIZE];
    buffer_t buffer;
    uint8_t wrbuf[BLK_SIZE<<1];

    void SetUp() override
    {
        memset(memory, 0, BLK_SIZE);
        buffer.mem = memory;
        buffer.pl = &memory[PACKET_HEADER_SIZE];
        buffer.size = BLK_SIZE;
        for (uint16_t idx=0;idx<BLK_SIZE - PACKET_HEADER_SIZE;idx++) {
            buffer.pl[idx] = BYTE_VAL(idx);
        }
        buffer.used = BLK_SIZE - PACKET_HEADER_SIZE;
        memory_device_reset(&memory_device_buffer);
        memset(wrbuf,0, sizeof(wrbuf));
    }

    void TearDown() override
    {
    }
};


TEST_F(PacketTest, initSucceed)
{
    elres_t res = packet_init();

    EXPECT_EQ(EMLIB_OK, res);
}

TEST_F(PacketTest, headerSize)
{

    EXPECT_EQ(4, sizeof(packet_bitfield_t));
}

TEST_F(PacketTest, openInvalidParameterFail)
{
    dev_handle_t hdl;

    packet_init();
    hdl = packet_open(NULL, PACKET_CH0, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
    EXPECT_EQ(DEV_HANDLE_NOTDEFINED, hdl);

    packet_init();
    hdl = packet_open(&memory_device, PACKET_CH0, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
    EXPECT_EQ(0, hdl);

    packet_init();
    hdl = packet_open(&memory_device, PACKET_ACK, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
    EXPECT_EQ(DEV_HANDLE_NOTDEFINED, hdl);

    packet_init();
    hdl = packet_open(&memory_device, PACKET_LINK_CONTROL, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
    EXPECT_EQ(DEV_HANDLE_NOTDEFINED, hdl);

    packet_init();
    hdl = packet_open(&memory_device, PACKET_CH0, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
    EXPECT_EQ(0, hdl);

    packet_init();
    hdl = packet_open(&memory_device, PACKET_CH0, PACKET_IS_RELIABLE, PACKET_NOT_CHECKED);
    EXPECT_EQ(0, hdl);

    packet_init();
    hdl = packet_open(&memory_device, PACKET_CH0, PACKET_NOT_RELIABLE, PACKET_IS_CHECKED);
    EXPECT_EQ(0, hdl);

    packet_init();
    hdl = packet_open(&memory_device, PACKET_CH0, PACKET_IS_RELIABLE, PACKET_IS_CHECKED);
    EXPECT_EQ(0, hdl);
}

TEST_F(PacketTest, OpenMoreThanAllowedFails)
{
    dev_handle_t hdl;
    packet_init();
    for (uint8_t i=0;i<PACKET_DEV_COUNT;i++) {
        hdl = packet_open(&memory_device, PACKET_CH0, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
        EXPECT_EQ(i, hdl);
    }
    hdl = packet_open(&memory_device, PACKET_CH0, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
    EXPECT_EQ(DEV_HANDLE_NOTDEFINED, hdl);
}

TEST_F(PacketTest, WriteValidPacket)
{
    elres_t res;
    dev_handle_t pktHdl = DEV_HANDLE_NOTDEFINED;
    packet_init();
    memory_device_print(&buffer);
    pktHdl = packet_open(&memory_device, PACKET_CH13, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
    res = packet_write(pktHdl, &buffer);
    EXPECT_EQ(EMLIB_OK, res);
    memory_device_print(&memory_device_buffer);
    for (uint8_t idx=0;idx<buffer.used;idx++) {
        uint8_t expVal = BYTE_VAL(idx);
        uint8_t curVal = memory_device_buffer.pl[idx+PACKET_HEADER_SIZE];
        EXPECT_EQ(expVal, curVal);
    }
    res = packet_close(pktHdl);
    EXPECT_EQ(EMLIB_OK, res);
    packet_header_t head = *((packet_header_t*)&memory_device_buffer.mem[0]);
    packet_head_print(head);
    EXPECT_EQ(BLK_SIZE-PACKET_HEADER_SIZE, head.bf.length);
    uint16_t chk_sum = 0;
    for (uint8_t i=0;i<4;i++) {
        chk_sum = (chk_sum + head.u8[i]) % 256;
    }
    EXPECT_EQ(255, chk_sum);
}


TEST_F(PacketTest, WriteValidPacketWithCheckFailsOnShortInBuffer)
{
    elres_t res;
    dev_handle_t pktHdl = DEV_HANDLE_NOTDEFINED;
    packet_init();
    memory_device_print(&buffer);
    pktHdl = packet_open(&memory_device, PACKET_CH13, PACKET_NOT_RELIABLE, PACKET_IS_CHECKED);
    res = packet_write(pktHdl, &buffer);
    EXPECT_EQ(EMLIB_ERROR, res);
    memory_device_print(&memory_device_buffer);
    for (uint8_t idx=0;idx<buffer.used;idx++) {
        uint8_t expVal = 0;
        uint8_t curVal = memory_device_buffer.pl[idx+PACKET_HEADER_SIZE];
        ASSERT_EQ(expVal, curVal);
    }
    res = packet_close(pktHdl);
    EXPECT_EQ(EMLIB_OK, res);
    packet_header_t head = *((packet_header_t*)&memory_device_buffer.mem[0]);
    packet_head_print(head);
    EXPECT_EQ(0, head.bf.length);
    EXPECT_EQ(0, head.bf.is_checked);
}

TEST_F(PacketTest, WriteValidPacketWithCheckSucceed)
{
    elres_t res;
    dev_handle_t pktHdl = DEV_HANDLE_NOTDEFINED;
    packet_init();
    memory_device_print(&buffer);
    pktHdl = packet_open(&memory_device, PACKET_CH13, PACKET_NOT_RELIABLE, PACKET_IS_CHECKED);
    res = packet_write(pktHdl, &buffer);
    EXPECT_EQ(EMLIB_ERROR, res);
    buffer.used -= 2;
    printf("Used count of bytes in buffer = %d\n", buffer.used);
    res = packet_write(pktHdl, &buffer);
    EXPECT_EQ(EMLIB_OK, res);
    memory_device_print(&memory_device_buffer);
    for (uint8_t idx=0;idx<buffer.used;idx++) {
        uint8_t expVal = BYTE_VAL(idx);
        uint8_t curVal = memory_device_buffer.pl[idx+PACKET_HEADER_SIZE];
        ASSERT_EQ(expVal, curVal);
    }
    res = packet_close(pktHdl);
    EXPECT_EQ(EMLIB_OK, res);
    packet_header_t head = *((packet_header_t*)&memory_device_buffer.mem[0]);
    packet_head_print(head);
    EXPECT_EQ(BLK_SIZE-PACKET_HEADER_SIZE-PACKET_TAIL_SIZE, head.bf.length);
    EXPECT_EQ(1, head.bf.is_checked);
    uint16_t chk_sum = 0;
    for (uint8_t i=0;i<4;i++) {
        chk_sum = (chk_sum + head.u8[i]) % 256;
    }
    EXPECT_EQ(255, chk_sum);
}

