/*
 * slip_test.cpp
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "buffer.h"
#include "buffer_pool.h"

#define BUFFER_SIZE 96
#define BUFFER_CNT 5

#include "gtest/gtest.h"
uint8_t wbuffer[BUFFER_SIZE];
uint8_t sbuffer[BUFFER_SIZE];
uint8_t rBufLine= 5;
uint8_t rBufCharCnt= 42;
class BufferTest : public ::testing::Test {
    protected:

    void SetUp() override    {
        for (uint8_t i=0;i<BUFFER_SIZE;i++){
            wbuffer[i]=i;
        }
        memcpy(sbuffer, wbuffer, BUFFER_SIZE);
    }

    void TearDown() {
        for (uint8_t i=0;i<rBufCharCnt;i++){
            EXPECT_EQ(wbuffer[i], i);
        }
    }
};

TEST_F(BufferTest, ErrorIfBufferAllocIsOK){

//TEST_F(BufferTest, ErrorIfBufferAllocIsOK)
	buffer_t * buf;
	buf= buffer_new(0);
	EXPECT_EQ(buf, (buffer_t *)NULL);
    buf= buffer_new(rBufCharCnt);
    EXPECT_NE(buf, (buffer_t *)NULL);
}

TEST_F(BufferTest, SetDataToBufferAndReadItBack){
//TEST_F(BufferTest, DISABLED_SetDataToBufferAndReadItBack){
	const uint8_t size = rBufCharCnt;
	uint8_t rsize = size;
    buffer_t * buf= buffer_new(size);
    buffer_print(buf, 0);
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
	buffer_pool_t * pool = buffer_pool_new(rBufLine, rBufCharCnt);
	buffer_pool_print(pool);
	buffer_t *buffer;
	for (uint8_t i=0;i<rBufLine;i++){
		buffer = buffer_pool_get(pool);
		buffer_print(buffer, i);
		EXPECT_NE(buffer, (buffer_t *)NULL);
	}
	buffer = buffer_pool_get(pool);
	EXPECT_EQ(buffer, (buffer_t *)NULL);

}
