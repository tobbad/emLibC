/*
 * slip_test.cpp
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */
#include "buffer.h"
#include "buffer_pool.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 96
#define BUFFER_CNT 5
#define ACNT 2

#include "gtest/gtest.h"
uint8_t wbuffer[BUFFER_SIZE];
uint8_t sbuffer[BUFFER_SIZE];
buffer_t *buf[ACNT];
uint16_t rBufLine = 5;
uint16_t rBufCharCnt = 40;
class BufferTest : public ::testing::Test {
  protected:
    void SetUp() override {
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
            wbuffer[i] = i;
        }
        memcpy(sbuffer, wbuffer, BUFFER_SIZE);
    }

    void TearDown() {
        for (uint8_t i = 0; i < rBufCharCnt; i++) {
            EXPECT_EQ(wbuffer[i], i);
        }

    }
};

TEST_F(BufferTest, NewBufferTest) {
    // TEST_F(BufferTest, DISABLED_ErrorIfBufferAllocIsOK)
    buffer_t *bufp;
    bufp = buffer_new(0);
    EXPECT_TRUE(bufp == NULL);
    bufp = buffer_new(rBufCharCnt);
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        EXPECT_EQ(bufp->mem[i], 0);
    }
    EXPECT_EQ(bufp->used, 0);
    EXPECT_EQ(bufp->size, rBufCharCnt);
}

/*
 * Buffer pool does not work
 */
TEST_F(BufferTest, SetDataToBufferAndReadItBack) {
    // TEST_F(BufferTest, DISABLED_SetDataToBufferAndReadItBack){
    int16_t size = rBufCharCnt;
    int16_t rsize = size;
    buffer_t *buf = buffer_new(rBufCharCnt);
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        buf->mem[i] = 0x30 + i;
    }
    buffer_print(buf, (char *)"buf");
    print_buffer(wbuffer, BUFFER_SIZE, "wbuffer");
    em_msg res = buffer_set(buf, (uint8_t *)&wbuffer, &size);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(size, rBufCharCnt);
    EXPECT_EQ(buf->used, rBufCharCnt);
    memset(wbuffer, 0, BUFFER_SIZE);
    buffer_print(buf, (char *)"buf filled with wbuffer");
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        wbuffer[i] = 0;
    }
    res = buffer_get(buf, (uint8_t *)&wbuffer, &rsize);
    EXPECT_EQ(buf->state, BUFFER_READY);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(rsize, rBufCharCnt);
    EXPECT_EQ(buf->used, 0);
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        EXPECT_EQ(wbuffer[i], sbuffer[i]);
    }
}

TEST_F(BufferTest, ArrayNewBufferTest) {
    // TEST_F(BufferTest, DISABLED_ErrorIfBufferAllocIsOK)
    for (uint8_t i = 0; i < ACNT; i++) {
        buf[i] = buffer_new(rBufCharCnt);
        buffer_print(buf[i], (char*)"buf[i]");
        for (uint8_t j = 0; j < rBufCharCnt; j++) {
            printf("buf[%2d]->mem[%2d] = %d"NL, i, j, buf[i]->mem[j]);
            EXPECT_EQ(buf[i]->mem[j], 0);
        }
        EXPECT_EQ(buf[i]->used, 0);
        EXPECT_EQ(buf[i]->size, rBufCharCnt);
    }
}

/*
 * Buffer pool does not work
 */
TEST_F(BufferTest, ArraySetDataToBufferAndReadItBack) {
    // TEST_F(BufferTest, DISABLED_SetDataToBufferAndReadItBack){
    int16_t size = rBufCharCnt;
    int16_t rsize = size;
    for (uint8_t i = 0; i < ACNT; i++) {
        buf[i] = buffer_new(rBufCharCnt);
        for (uint8_t j = 0; j < rBufCharCnt; j++) {
            buf[i]->mem[j] = 0x30 + i;
        }
        buffer_print(buf[i], (char *)"buf");
        print_buffer(wbuffer, BUFFER_SIZE, "wbuffer");
        em_msg res = buffer_set(buf[i], (uint8_t *)wbuffer, &size);
        EXPECT_EQ(res, EM_OK);
        EXPECT_EQ(size, rBufCharCnt);
        EXPECT_EQ(buf[i]->used, rBufCharCnt);
        memset(wbuffer, 0, BUFFER_SIZE);
        buffer_print(buf[i], (char *)"buf filled with wbuffer");
        res = buffer_get(buf[i], (uint8_t *)wbuffer, &rsize);
        print_buffer(wbuffer, BUFFER_SIZE, "wbuf from buffer");
        EXPECT_EQ(res, rsize);
        EXPECT_EQ(buf[i]->state, BUFFER_READY);
        EXPECT_EQ(res, EM_OK);
        EXPECT_EQ(rsize, rBufCharCnt);
        EXPECT_EQ(buf[i]->used, 0);
        printf((char *)"Before check"NL);
        for (uint8_t j = 0; j< rBufCharCnt; j++) {
            EXPECT_EQ(wbuffer[j], sbuffer[j]);
        }
        printf((char *)"finished"NL);

    }
}

// TEST_F(BufferTest, DISABLED_CreateBufferPool) {
TEST_F(BufferTest, DISABLED_CreateBufferPool) {
    buffer_pool_t *pool = buffer_pool_new(rBufLine, rBufCharCnt);
    buffer_pool_print(pool);
    buffer_t *buffer;
    for (uint8_t i = 0; i < rBufLine; i++) {
        buffer = buffer_pool_get(pool);
        buffer_print(buffer, (char *)"buffer");
        EXPECT_NE(buffer, (buffer_t *)NULL);
    }
    buffer = buffer_pool_get(pool);
    EXPECT_EQ(buffer, (buffer_t *)NULL);
}
