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
#define BLK_SIZE 2000
class PacketTest : public ::testing::Test {
    protected:

    dev_handle_t hdl;
    uint8_t buffer[BLK_SIZE];
    mbuf buffer_t = {
    	.size = BLK_SIZE,
    	.used =0
    	.mem = &buffer
    	.pl = &buffer,
    };
    void SetUp() override
    {
    	hdl = device_init(memory_device, mbuf)
		decice_write(hdl);
        for (uint16_t idx=0;idx<BLK_SIZE - PACKET_HEADER_SIZE;idx++) {
            buffer.pl[idx] = BYTE_VAL(idx);
        }
		decice_write(hdl, buffer, BLK_SIZE);
    }

    void TearDown() override
    {
    }
};


TEST_F(PacketTest, initSucceed)
{
    em_msg res = packet_init();

    EXPECT_EQ(EM_OK, res);
}

TEST_F(PacketTest, headerSize)
{

    EXPECT_EQ(4, sizeof(packet_bitfield_t));
}

TEST_F(PacketTest, openInvalidParameterFail)
{
    dev_handle_t hdl;

    packet_init();
    hdl = packet_open((packet_t)0, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
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
    em_msg res;
    dev_handle_t pktHdl = DEV_HANDLE_NOTDEFINED;
    packet_init();
    memory_device_print(&buffer);
    pktHdl = packet_open(&memory_device, PACKET_CH13, PACKET_NOT_RELIABLE, PACKET_NOT_CHECKED);
    res = packet_write(pktHdl, &buffer);
    EXPECT_EQ(EM_OK, res);
    memory_device_print(&memory_device_buffer);
    for (uint8_t idx=0;idx<buffer.used;idx++) {
        uint8_t expVal = BYTE_VAL(idx);
        uint8_t curVal = memory_device_buffer.pl[idx+PACKET_HEADER_SIZE];
        EXPECT_EQ(expVal, curVal);
    }
    res = packet_close(pktHdl);
    EXPECT_EQ(EM_OK, res);
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
    em_msg res;
    dev_handle_t pktHdl = DEV_HANDLE_NOTDEFINED;
    packet_init();
    memory_device_print(&buffer);
    pktHdl = packet_open(&memory_device, PACKET_CH13, PACKET_NOT_RELIABLE, PACKET_IS_CHECKED);
    res = packet_write(pktHdl, &buffer);
    EXPECT_EQ(EM_ERR, res);
    memory_device_print(&memory_device_buffer);
    for (uint8_t idx=0;idx<buffer.used;idx++) {
        uint8_t expVal = 0;
        uint8_t curVal = memory_device_buffer.pl[idx+PACKET_HEADER_SIZE];
        ASSERT_EQ(expVal, curVal);
    }
    res = packet_close(pktHdl);
    EXPECT_EQ(EM_OK, res);
    packet_header_t head = *((packet_header_t*)&memory_device_buffer.mem[0]);
    packet_head_print(head);
    EXPECT_EQ(0, head.bf.length);
    EXPECT_EQ(0, head.bf.is_checked);
}

TEST_F(PacketTest, WriteValidPacketWithCheckSucceed)
{
    em_msg res;
    dev_handle_t pktHdl = DEV_HANDLE_NOTDEFINED;
    packet_init();
    memory_device_print(&buffer);
    pktHdl = packet_open(&memory_device, PACKET_CH13, PACKET_NOT_RELIABLE, PACKET_IS_CHECKED);
    res = packet_write(pktHdl, &buffer);
    EXPECT_EQ(EM_ERR, res);
    buffer.used -= 2;
    printf("Used count of bytes in buffer = %d\n", buffer.used);
    res = packet_write(pktHdl, &buffer);
    EXPECT_EQ(EM_OK, res);
    memory_device_print(&memory_device_buffer);
    for (uint8_t idx=0;idx<buffer.used;idx++) {
        uint8_t expVal = BYTE_VAL(idx);
        uint8_t curVal = memory_device_buffer.pl[idx+PACKET_HEADER_SIZE];
        ASSERT_EQ(expVal, curVal);
    }
    res = packet_close(pktHdl);
    EXPECT_EQ(EM_OK, res);
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

