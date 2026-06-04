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
            rbuffer[i] = '0' + i;
        }
        buf = buffer_new(rBufCharCnt, LINEAR);
        for (uint8_t i = 0; i < ACNT; i++) {
            abuf[i] = buffer_new(rBufCharCnt, LINEAR);
        }
        // pool = buffer_pool_new(rBufLine, rBufCharCnt, LINEAR, LINEAR);
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
        // buffer_pool_free(pool);
    }
};

TEST_F(BufferTest, NewBufferZeroSize) {
    // TEST_F(BufferTest, DISABLED_NewBufferZeroSize) {

    buffer_t *bufp = buffer_new(0, LINEAR);
    EXPECT_EQ(bufp, nullptr);
}

TEST_F(BufferTest, NewBufferTest) {
    // TEST_F(BufferTest, DISABLED_NewBufferTest) {

    ASSERT_NE(buf, nullptr);
    // buffer_print(buf, (char*)"New buffer");

    for (uint16_t i = 0; i < rBufCharCnt; i++) {
        EXPECT_EQ(buf->mem[i], 0);
    }
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(buf->state, BUFFER_READY);
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(buf->size, rBufCharCnt);
}

TEST_F(BufferTest, SetDataToBufferAndReadItBack) {
    // TEST_F(BufferTest, DISABLED_SetDataToBufferAndReadItBack) {
    int16_t size = rBufCharCnt;
    int16_t rsize = rBufCharCnt;
    int16_t wsize = 16;
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        buf->mem[i] = 0x30 + i;
    }
    // buffer_print(buf, (char *)"ASCII");
    /// print_buffer(wbuffer, rBufCharCnt, "wbuffer");
    em_msg res = buffer_set(buf, (uint8_t *)rbuffer, wsize);
    // print_buffer(rbuffer, rBufCharCnt, "Send");
    // buffer_print(buf, (char*)"Receiving (buf)");
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
    print_buffer(wbuffer, rBufCharCnt, (char *)"wbuffer");
    for (uint8_t i = 0; i < wsize; i++) {
        EXPECT_EQ(wbuffer[i], rbuffer[i]);
    }
}

TEST_F(BufferTest, ArrayNewBufferTest) {
    // TEST_F(BufferTest, DISABLED_ArrayNewBufferTest) {
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
    // TEST_F(BufferTest, DISABLED_SetArrayDataToBufferAndReadItBack) {
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

TEST_F(BufferTest, SetRejectsNonPositiveSize) {
    EXPECT_EQ(buffer_set(buf, rbuffer, 0), EM_ERR);
    EXPECT_EQ(buffer_set(buf, rbuffer, -5), EM_ERR);
    EXPECT_EQ(buffer_used(buf), 0); // nichts passiert
    EXPECT_EQ(buf->state, BUFFER_READY);
}

TEST_F(BufferTest, SetRejectsOversize) {
    EXPECT_EQ(buffer_set(buf, rbuffer, buf->size + 1), EM_ERR); // 49 > writable 48
    EXPECT_EQ(buffer_used(buf), 0);                             // Buffer unverändert
    EXPECT_EQ(buf->state, BUFFER_READY);
}

TEST_F(BufferTest, GetRejectsNonPositiveSize) {
    EXPECT_EQ(buffer_set(buf, rbuffer, 16), EM_OK);
    int16_t rsize = 0;
    EXPECT_EQ(buffer_get(buf, wbuffer, &rsize), EM_ERR); // *size <= 0
    EXPECT_EQ(buffer_used(buf), 16);                     // Daten unberührt
}

TEST_F(BufferTest, GetFromEmptyReturnsZeroSize) {
    int16_t rsize = 10;
    EXPECT_EQ(buffer_get(buf, wbuffer, &rsize), EM_OK); // leer → OK, aber …
    EXPECT_EQ(rsize, 0);                                // … 0 Bytes geliefert
    EXPECT_EQ(buf->state, BUFFER_READY);
}

TEST_F(BufferTest, ResetWipesDataAndRewindsFirst) {
    int16_t wsize = 16;
    buf->id = 7; // soll von reset genullt werden
    EXPECT_EQ(buffer_set(buf, rbuffer, wsize), EM_OK);
    EXPECT_EQ(buffer_used(buf), wsize);
    EXPECT_EQ(buf->state, BUFFER_USED);

    EXPECT_EQ(buffer_reset(buf), EM_OK);

    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(buf->state, BUFFER_READY);
    EXPECT_EQ(buf->first, 0); // zurück auf 0 (vs. clear: vorgeschoben)
    EXPECT_EQ(buf->id, 0);
    EXPECT_EQ(buf->pl, buf->mem);
    EXPECT_EQ(buffer_writeable(buf), buf->size);

    // Speicher physisch genullt — DAS unterscheidet reset von clear:
    for (int16_t i = 0; i < buf->size; i++) {
        EXPECT_EQ(buf->mem[i], 0) << "at index " << i;
    }
}

TEST_F(BufferTest, ClearEmptiesButKeepsDataAndAdvancesFirst) {
    int16_t wsize = 16;
    EXPECT_EQ(buffer_set(buf, wbuffer, wsize), EM_OK);
    EXPECT_EQ(buffer_used(buf), wsize);
    EXPECT_EQ(buf->state, BUFFER_USED);

    EXPECT_EQ(buffer_clear(buf), EM_OK);

    EXPECT_EQ(buffer_used(buf), 0); // logisch leer
    EXPECT_EQ(buf->state, BUFFER_READY);
    EXPECT_EQ(buffer_writeable(buf), buf->size); // wieder voll beschreibbar
    EXPECT_EQ(buf->first, wsize);                // first um altes used vorgeschoben (0+16)%48

    // Daten physisch noch da — das unterscheidet clear von reset:
    for (int16_t i = 0; i < wsize; i++) {
        EXPECT_EQ(buf->mem[i], wbuffer[i]);
    }
}

TEST_F(BufferTest, ToLowerOnlyAffectsUppercaseLetters) {
    const uint8_t in[] = "0ABZ@[7+}-'&%|";     // @ und [ liegen in 0x40-0x5F, sind aber keine Buchstaben
    const uint8_t expect[] = "0abz@[7+}-'&%|"; // nur A--Z klein, Rest unverändert
    int16_t n = (int16_t)(sizeof(in) - 1);

    EXPECT_EQ(buffer_set(buf, in, n), EM_OK);
    EXPECT_EQ(buffer_tolower(buf), EM_OK);

    for (int16_t i = 0; i < n; i++) {
        EXPECT_EQ(buf->mem[i], expect[i]) << "at index " << i;
    }
}

TEST_F(BufferTest, FreeReadyBufferReturnsOk) {
    buffer_t *b = buffer_new(rBufCharCnt, LINEAR);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->state, BUFFER_READY);
    EXPECT_EQ(buffer_free(b), EM_OK); // sauberer Buffer -> OK
}

TEST_F(BufferTest, FreeUsedBufferReturnsErrorButStillFrees) {
    buffer_t *b = buffer_new(rBufCharCnt, LINEAR);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(buffer_set(b, rbuffer, 16), EM_OK);
    ASSERT_EQ(b->state, BUFFER_USED);
    EXPECT_EQ(buffer_free(b), EM_ERR); // USED -> Fehler ans Aufrufer …
    // … aber ASan bestätigt: mem + struct sind trotzdem weg, kein Leak
}

/*
 * Check ring buffer functionality
 */
class RingBufferTest : public ::testing::Test {
  protected:
    uint8_t wbuffer[BUFFER_SIZE];
    uint8_t rbuffer[BUFFER_SIZE]; // referemc data
    uint8_t blank[BUFFER_SIZE];   // referemc data
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
        // printf("buffer new result %p"NL, buf);
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

TEST_F(RingBufferTest, GetPartialKeepsRemainder) {
    EXPECT_EQ(buffer_set(buf, wbuffer, 16), EM_OK);
    int16_t rsize = 8;
    EXPECT_EQ(buffer_get(buf, rbuffer, &rsize), EM_OK);
    EXPECT_EQ(rsize, 8);
    EXPECT_EQ(buffer_used(buf), 8); // Rest bleibt
    EXPECT_EQ(buf->state, BUFFER_USED);
    for (int16_t i = 0; i < 8; i++)
        EXPECT_EQ(rbuffer[i], wbuffer[i]);

    rsize = 8; // Rest nachlesen
    EXPECT_EQ(buffer_get(buf, rbuffer, &rsize), EM_OK);
    EXPECT_EQ(rsize, 8);
    EXPECT_EQ(buffer_used(buf), 0);
    for (int16_t i = 0; i < 8; i++)
        EXPECT_EQ(rbuffer[i], wbuffer[8 + i]);
}

TEST_F(RingBufferTest, NewBufferZeroSize) {
    // TEST_F(RingBufferTest, DISABLED_NewBufferZeroSize) {

    buffer_t *bufp = buffer_new(0, RING);
    EXPECT_EQ(bufp, nullptr);
}

// TEST_F(RingBufferTest, DISABLED_NewBufferTest) {
TEST_F(RingBufferTest, NewBufferTest) {
    ASSERT_NE(buf, nullptr);
    // buffer_print(buf, (char*)"New buffer");
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
    // TEST_F(RingBufferTest, DISABLED_SetDataToBufferAndReadItBack) {
    int16_t size = rBufCharCnt;
    int16_t rsize = size;
    EXPECT_EQ(buffer_used(buf), 0);
    buffer_print(buf, (char *)"Initial");
    print_buffer(wbuffer, rBufCharCnt, "wbuffer");
    printf("Set %d data" NL, size >> 1);
    em_msg res = buffer_set(buf, (uint8_t *)wbuffer, size >> 1);
    buffer_print(buf, (char *)"After set 1");
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buffer_used(buf), size >> 1);
    EXPECT_EQ(buf->state, BUFFER_USED);
    res = buffer_set(buf, (uint8_t *)&wbuffer[size >> 1], size >> 1);
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
    // TEST_F(RingBufferTest, DISABLED_WrapAroundWriteAndReadItBack) {
    int16_t size = rBufCharCnt; // 48
    int16_t rsize;
    em_msg res;

    // first über die API nach vorn schieben: size>>2 schreiben und wieder lesen
    res = buffer_set(buf, wbuffer, size >> 2);
    EXPECT_EQ(res, EM_OK);
    rsize = size >> 2;
    res = buffer_get(buf, rbuffer, &rsize);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(rsize, size >> 2);
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(buf->first, size >> 2); // Ausgangszustand über die API erreicht

    // zwei size>>1-Schreibvorgänge erzwingen den Wrap beim zweiten Set
    res = buffer_set(buf, wbuffer, size >> 1);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buffer_used(buf), size >> 1);
    res = buffer_set(buf, &wbuffer[size >> 1], size >> 1); // wraps
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(buffer_used(buf), size);

    rsize = size;
    memset(rbuffer, 0, rBufCharCnt);
    res = buffer_get(buf, rbuffer, &rsize);
    EXPECT_EQ(res, EM_OK);
    EXPECT_EQ(rsize, size);
    EXPECT_EQ(buffer_used(buf), 0);
    EXPECT_EQ(buf->state, BUFFER_READY);
    for (uint8_t i = 0; i < rBufCharCnt; i++) {
        EXPECT_EQ(wbuffer[i], rbuffer[i]);
    }
}

class BufferPoolTest : public ::testing::Test {
  protected:
    uint8_t line_cnt = 8;
    uint8_t column_cnt = 18;
    buffer_pool_t *pool = NULL;

    void SetUp() override {}

    void TearDown() { buffer_pool_free(pool); }
};

/*
 * Buffer pool does not work
 */
TEST_F(BufferPoolTest, CreateBufferPool) {
    // TEST_F(RingBufferTest, CreateBufferPool) {
    pool = buffer_pool_new(line_cnt, column_cnt, LINEAR, LINEAR);
    ASSERT_NE(pool, nullptr);
    for (uint8_t lnr = 0; lnr < line_cnt; lnr++) {
        ASSERT_NE(pool, nullptr);
    }
    buffer_pool_free(pool);
    pool = NULL;
}
