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
uint16_t rBufLine = 5;
uint16_t rBufCharCnt = 40;

class BufferTest : public ::testing::Test {
  protected:
    uint8_t wbuffer[BUFFER_SIZE];
    uint8_t sbuffer[BUFFER_SIZE];
    buffer_t *abuf[ACNT];
    buffer_t *buf;
    buffer_pool_t *pool=NULL;

    void SetUp() override {
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
            wbuffer[i] = i;
        }
        memcpy(sbuffer, wbuffer, BUFFER_SIZE);
        buf = buffer_new(rBufCharCnt);
        ASSERT_NE(buf, nullptr);
        for (uint8_t i = 0; i < ACNT; i++) {
            abuf[i] = buffer_new(rBufCharCnt);
            ASSERT_NE(abuf[i], nullptr);
        }
        pool = buffer_pool_new(rBufLine, rBufCharCnt, LINEAR);
    }

    void TearDown() {
        if (buf) {
            buffer_free(buf);
            buf = nullptr;
        }

        for (uint8_t i = 0; i < ACNT; i++) {
            if (abuf[i]) {
                buffer_free(abuf[i]);
                abuf[i] = nullptr;
            }
        }
        buffer_pool_free(pool);
    }
};


TEST_F(BufferTest, NewBufferZeroSize)
{
    buffer_t *bufp = buffer_new(0);
    EXPECT_EQ(bufp, nullptr);
}

TEST_F(BufferTest, NewBufferTest) {
    ASSERT_NE(buf, nullptr);

    for (uint16_t i = 0; i < rBufCharCnt; i++) {
        EXPECT_EQ(buf->mem[i], 0);
    }

    EXPECT_EQ(buf->used, 0);
    EXPECT_EQ(buf->size, rBufCharCnt);
}

/*
 * Buffer pool does not work
 */
TEST_F(BufferTest, SetDataToBufferAndReadItBack) {
    // TEST_F(BufferTest, DISABLED_SetDataToBufferAndReadItBack){
    int16_t size = rBufCharCnt;
    int16_t rsize = size;
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        buf->mem[i] = 0x30 + i;
    }
    buffer_print(buf, (char *)"buf");
    print_buffer(wbuffer, rBufCharCnt, "wbuffer");
    em_msg res = buffer_set(buf, (uint8_t *)&wbuffer, &size);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(size, rBufCharCnt);
    EXPECT_EQ(buf->used, rBufCharCnt);
    memset(wbuffer, 0, rBufCharCnt);
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
        buffer_print(abuf[i], (char*)"abuf[i]");
        for (uint8_t j = 0; j < rBufCharCnt; j++) {
            //printf("abuf[%2d].mem[%2d] = %d"NL, i, j, abuf[i]->mem[j]);
            EXPECT_EQ(abuf[i]->mem[j], 0);
        }
        EXPECT_EQ(abuf[i]->used, 0);
        EXPECT_EQ(abuf[i]->size, rBufCharCnt);
    }
}

TEST_F(BufferTest, ArraySetDataToBufferAndReadItBack) {
    // TEST_F(BufferTest, DISABLED_SetDataToBufferAndReadItBack){
    int16_t size = rBufCharCnt;
    int16_t rsize = size;
    for (uint8_t i = 0; i < ACNT; i++) {
        for (uint8_t j = 0; j < rBufCharCnt; j++) {
            abuf[i]->mem[j] = 0x30 + j;
        }
        buffer_print(abuf[i], (char *)"abuf{i]");
        print_buffer(wbuffer, rBufCharCnt, "wbuffer");
        em_msg res = buffer_set(abuf[i], (uint8_t *)wbuffer, &size);
        EXPECT_EQ(res, EM_OK);
        EXPECT_EQ(size, rBufCharCnt);
        EXPECT_EQ(abuf[i]->used, rBufCharCnt);
        memset(wbuffer, 0, rBufCharCnt);
        buffer_print(abuf[i], (char *)"abuf[i] filled with wbuffer");
        res = buffer_get(abuf[i], (uint8_t *)wbuffer, &rsize);
        print_buffer(wbuffer, rBufCharCnt, "wbuffer from buffer");
        EXPECT_EQ(abuf[i]->state, BUFFER_READY);
        EXPECT_EQ(res, EM_OK);
        EXPECT_EQ(rsize, rBufCharCnt);
        EXPECT_EQ(abuf[i]->used, 0);
        std::cout << "Before check\n";
        for (uint8_t j = 0; j< rBufCharCnt; j++) {
            EXPECT_EQ(wbuffer[j], sbuffer[j]);
        }
        printf("finished" NL);

    }
}
/*
 * Buffer pool does not work
 */
// TEST_F(BufferTest, DISABLED_CreateBufferPool) {
TEST_F(BufferTest, CreateBufferPool) {

    buffer_pool_print(pool);
    buffer_t *buffer;
    for (uint8_t i = 0; i < rBufLine; i++) {
        buffer = buffer_pool_get(pool);
        buffer_print(buffer, (char *)"BoolBuffer");
        EXPECT_NE(buffer, (buffer_t *)NULL);
    }
    buffer = buffer_pool_get(pool);
    EXPECT_EQ(buffer, (buffer_t *)NULL);
}
