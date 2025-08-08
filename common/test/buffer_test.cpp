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

#define BUFFER_SIZE 96
#define BUFFER_CNT 20

#include "gtest/gtest.h"
uint8_t wbuffer[BUFFER_SIZE];
uint8_t sbuffer[BUFFER_SIZE];
uint8_t rBufSize= 42;
uint8_t rBufCnt= 5;
class BufferTest : public ::testing::Test {
    protected:

    void SetUp() override    {
        for (uint8_t i=0;i<BUFFER_SIZE;i++){
            wbuffer[i]=i;
        }
        memcpy(sbuffer, wbuffer, BUFFER_SIZE);
    }

    void TearDown() {
        for (uint8_t i=0;i<rBufSize;i++){
            EXPECT_EQ(wbuffer[i], i);
        }
    }
};

TEST_F(BufferTest, ErrorIfBufferAllocIsOK){

//TEST_F(BufferTest, ErrorIfBufferAllocIsOK)
	buffer_t * buf;
	buf= buffer_new(0);
	EXPECT_EQ(buf, (buffer_t *)NULL);
    buf= buffer_new(rBufSize);
    EXPECT_NE(buf, (buffer_t *)NULL);
}

TEST_F(BufferTest, SetDataToBufferAndReadItBack){
//TEST_F(BufferTest, DISABLED_SetDataToBufferAndReadItBack){
	const uint8_t size = rBufSize;
	uint8_t rsize = size;
    buffer_t * buf= buffer_new(size);
    em_msg res = buffer_set(buf, (uint8_t*)&wbuffer, BUFFER_SIZE);
    EXPECT_EQ(res, EM_OK);
    res = buffer_set(buf,(uint8_t*) &wbuffer, size);
    EXPECT_EQ(res, EM_OK);
    memset(wbuffer, 0, BUFFER_SIZE);
    res = buffer_get(buf, (uint8_t*)&wbuffer, &rsize);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(rsize, size);
    for (uint8_t i=0;i<size;i++){
        EXPECT_EQ(wbuffer[i], sbuffer[i]);
    }
}

TEST_F(BufferTest, CreateBufferPool){
	buffer_pool_t * pool = buffer_pool_new(rBufCnt, rBufSize);

}
