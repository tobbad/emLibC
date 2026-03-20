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
uint16_t rBufCharCnt = 48;

class BufferTest : public ::testing::Test {
  protected:
    uint8_t wbuffer[BUFFER_SIZE];
    uint8_t rbuffer[BUFFER_SIZE]; // referemc data
    buffer_t *abuf[ACNT];
    buffer_t *buf;
    buffer_pool_t *pool = NULL;

    void SetUp() override {
        memset(wbuffer, 0, BUFFER_SIZE);
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
            rbuffer[i] = '0'+i;
        }
        buf = buffer_new(rBufCharCnt, LINEAR);
        for (uint8_t i = 0; i < ACNT; i++) {
            abuf[i] = buffer_new(rBufCharCnt, LINEAR);
        }
        //pool = buffer_pool_new(rBufLine, rBufCharCnt, LINEAR, LINEAR);
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
        //buffer_pool_free(pool);
    }
};

TEST_F(BufferTest, NewBufferZeroSize) {
    // TEST_F(BufferTest, DISABLED_NewBufferZeroSize) {

    buffer_t *bufp = buffer_new(0, LINEAR);
    EXPECT_EQ(bufp, nullptr);
}

TEST_F(BufferTest, NewBufferTest) {
//TEST_F(BufferTest, DISABLED_NewBufferTest) {

    ASSERT_NE(buf, nullptr);
    //buffer_print(buf, (char*)"New buffer");

    for (uint16_t i = 0; i < rBufCharCnt; i++) {
        EXPECT_EQ(buf->mem[i], 0);
    }
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(buf->state, BUFFER_READY);
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(buf->size, rBufCharCnt);
}


TEST_F(BufferTest, SetDataToBufferAndReadItBack) {
//TEST_F(BufferTest, DISABLED_SetDataToBufferAndReadItBack) {
    int16_t size = rBufCharCnt;
    int16_t rsize=rBufCharCnt;
    int16_t wsize = 16;
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        buf->mem[i] = 0x30 + i;
    }
    // buffer_print(buf, (char *)"ASCII");
    /// print_buffer(wbuffer, rBufCharCnt, "wbuffer");
    em_msg res = buffer_set(buf, (uint8_t *)rbuffer, wsize);
    //print_buffer(rbuffer, rBufCharCnt, "Send");
    //buffer_print(buf, (char*)"Receiving (buf)");
    for (uint8_t i = 0; i < wsize; i++) {
        EXPECT_EQ(buf->mem[i], rbuffer[i]);
    }
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buffer_used(buf), wsize);
    EXPECT_EQ(buf->state, BUFFER_USED);
    EXPECT_EQ(buf->size, size);
    memset(wbuffer, 0, rBufCharCnt);
    buffer_print(buf, (char *)"buf");
    res = buffer_get(buf, (uint8_t *)wbuffer, &rsize);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(rsize, wsize);
    EXPECT_EQ(buf->state, BUFFER_READY);
    EXPECT_EQ(buffer_used(buf), 0);
    print_buffer(wbuffer, rBufCharCnt, (char*)"wbuffer");
    for (uint8_t i = 0; i < wsize; i++) {
        EXPECT_EQ(wbuffer[i], rbuffer[i]);
    }
}

TEST_F(BufferTest, ArrayNewBufferTest) {
//TEST_F(BufferTest, DISABLED_ArrayNewBufferTest) {
    for (uint8_t i = 0; i < ACNT; i++) {
        // buffer_print(abuf[i], (char*)"abuf[i]");
        for (uint8_t j = 0; j < rBufCharCnt; j++) {
            // printf("abuf[%2d].mem[%2d] = %d"NL, i, j, abuf[i]->mem[j]);
            EXPECT_EQ(abuf[i]->mem[j], 0);
        }
        EXPECT_EQ(buffer_used(abuf[i]), 0);
        EXPECT_EQ(abuf[i]->size, rBufCharCnt);
    }
}

TEST_F(BufferTest, ArraySetDataToBufferAndReadItBack) {
//TEST_F(BufferTest, DISABLED_SetArrayDataToBufferAndReadItBack) {
    int16_t size = rBufCharCnt;
    int16_t rsize = size;
    em_msg res;
    for (uint8_t i = 0; i < ACNT; i++) {
        for (uint8_t j = 0; j < rBufCharCnt; j++) {
            abuf[i]->mem[j] = 0x30 + j;
        }
        // buffer_print(abuf[i], (char *)"abuf");
        // print_buffer(wbuffer, rBufCharCnt, "wbuffer");
        res = buffer_set(abuf[i], (uint8_t *)rbuffer, size);
        EXPECT_EQ(res, EM_OK);
        EXPECT_EQ(size, rBufCharCnt);
        EXPECT_EQ(buffer_used(abuf[i]), rBufCharCnt);
        memset(wbuffer, 0, BUFFER_SIZE);
        res = buffer_get(abuf[i], (uint8_t *)wbuffer, &rsize);
        // print_buffer(wbuffer, size, (char *)"wbuffer obtained from abuf");
        EXPECT_EQ(abuf[i]->state, BUFFER_READY);
        EXPECT_EQ(res, EM_OK);
        EXPECT_EQ(rsize, rBufCharCnt);
        EXPECT_EQ(buffer_used(abuf[i]), 0);
        for (uint8_t j = 0; j < rBufCharCnt; j++) {
            EXPECT_EQ(wbuffer[j], rbuffer[j]);
        }
    }
}
/*
 * Check ring buffer functionality
 */
class RingBufferTest : public ::testing::Test {
  protected:
    uint8_t wbuffer[BUFFER_SIZE];
    uint8_t rbuffer[BUFFER_SIZE]; // referemc data
    uint8_t blank[BUFFER_SIZE]; // referemc data
    buffer_t *abuf[ACNT];
    buffer_t *buf;
    buffer_pool_t *pool = NULL;

    void SetUp() override {
        memset(wbuffer, 0, BUFFER_SIZE);
        for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
            wbuffer[i] = i;
        }
        memset(blank, 0, BUFFER_SIZE);
        buf = buffer_new(rBufCharCnt, RING);
        //printf("buffer new result %p"NL, buf);
        for (uint8_t i = 0; i < ACNT; i++) {
            abuf[i] = buffer_new(rBufCharCnt, RING);
        }
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
    }
};
TEST_F(RingBufferTest, NewBufferZeroSize) {
//TEST_F(RingBufferTest, DISABLED_NewBufferZeroSize) {

    buffer_t *bufp = buffer_new(0, RING);
    EXPECT_EQ(bufp, nullptr);
}

//TEST_F(RingBufferTest, DISABLED_NewBufferTest) {
TEST_F(RingBufferTest, NewBufferTest) {

    ASSERT_NE(buf, nullptr);
    //buffer_print(buf, (char*)"New buffer");
    for (uint16_t i = 0; i < rBufCharCnt; i++) {
        EXPECT_EQ(buf->mem[i], 0);
    }
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(buf->state, BUFFER_READY);
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(buffer_writeable(buf), rBufCharCnt);
    EXPECT_EQ(buf->size, rBufCharCnt);
}

TEST_F(RingBufferTest, SetDataTwoToBufferAndReadItBack) {
//TEST_F(RingBufferTest, DISABLED_SetDataToBufferAndReadItBack) {
    int16_t size = rBufCharCnt;
    int16_t rsize = size;
    EXPECT_EQ(buffer_used(buf), 0);
    buffer_print(buf, (char *)"Initial");
    print_buffer(wbuffer, rBufCharCnt, "wbuffer");
    printf("Set %d data"NL, size>>1); 
    em_msg res = buffer_set(buf, (uint8_t *)wbuffer, size>>1);
    buffer_print(buf, (char *)"After set 1");
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buffer_used(buf), size>>1);
    EXPECT_EQ(buf->state, BUFFER_USED);
    res = buffer_set(buf, (uint8_t *)&wbuffer[size>>1], size>>1);
    buffer_print(buf, (char *)"After set 2");
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buffer_used(buf), rBufCharCnt);
    EXPECT_EQ(buf->state, BUFFER_USED);
    memset(rbuffer, 0, rBufCharCnt);
    // print_buffer(wbuffer, rBufCharCnt, (char *)"wbuffer before retrive copy of rbuf");
    res = buffer_get(buf, (uint8_t *)&rbuffer, &rsize);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buf->state, BUFFER_READY);
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(rsize, rBufCharCnt);
    EXPECT_EQ(buffer_used(buf), 0);
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        EXPECT_EQ(wbuffer[i], rbuffer[i]);
    }
}

TEST_F(RingBufferTest, WrapAroundWriteAndReadItBack) {
//TEST_F(RingBufferTest, DISABLED_WrapAroundWriteAndReadItBack) {
    int16_t size = rBufCharCnt;
    int16_t rsize = size;
    int16_t offset = 16;
    EXPECT_EQ(buffer_used(buf), 0);
    buffer_print(buf, (char *)"Initial");
    print_buffer(wbuffer, rBufCharCnt, "wbuffer");
    em_msg res;
    
    buf->first = offset;
    buf->used   = 0;
    buffer_print(buf, (char *)"Offset");
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buffer_used(buf), 0);
    printf("Set %d data"NL, size>>1); 
    res = buffer_set(buf, (uint8_t *)wbuffer, size>>1);
    buffer_print(buf, (char *)"After set 1");
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buffer_used(buf), (size>>1));
    EXPECT_EQ(buf->state, BUFFER_USED);
    res = buffer_set(buf, (uint8_t *)&wbuffer[size>>1], size>>1); // This fails, nothing is written
    buffer_print(buf, (char *)"After set 2");
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buffer_used(buf), size);
    EXPECT_EQ(buf->state, BUFFER_USED);
    memset(rbuffer, 0, rBufCharCnt);
    // print_buffer(wbuffer, rBufCharCnt, (char *)"wbuffer before retrive copy of rbuf");
    res = buffer_get(buf, (uint8_t *)&rbuffer, &rsize);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buf->state, BUFFER_READY);
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(rsize, (size));
    EXPECT_EQ(buffer_used(buf), 0);
    print_buffer(rbuffer, rBufCharCnt, "rbuffer");
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        EXPECT_EQ(wbuffer[i], rbuffer[i]);
    }
}

/*
 * Buffer pool does not work
 */
TEST_F(RingBufferTest, DISABLED_CreateBufferPool) {
    // TEST_F(RingBufferTest, CreateBufferPool) {
    em_msg res;
    buffer_t *buffer[rBufLine];
    buffer_t *cbuffer;
    for (uint8_t i = 0; i < rBufLine; i++) {
        buffer[i] = buffer_pool_get(pool);
        ASSERT_NE(buffer[i], nullptr);
    }

    cbuffer = buffer_pool_get(pool);
    ASSERT_EQ(cbuffer, nullptr);

    for (uint8_t i = 0; i < rBufLine; i++) {
        res = buffer_pool_return(pool, buffer[i]);
        ASSERT_EQ(res, EM_OK);
    }
}
