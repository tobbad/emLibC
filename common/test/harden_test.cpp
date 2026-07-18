/*
 * harden_test.cpp
 *
 * Guard-/Härtungstests: jede gehärtete öffentliche API wird mit NULL,
 * out-of-range Indizes, size<=0 usw. aufgerufen. Erwartet wird ein sauberer
 * Fehler-Rückgabewert -- KEIN Crash. Unter AddressSanitizer (Debug-Build)
 * belegt ein grüner Lauf zusätzlich, dass kein OOB-Zugriff passiert.
 */
#include "buffer.h"
#include "buffer_pool.h"
#include "common.h"
#include "device.h"
#include "state.h"
#include "gtest/gtest.h"
#include <cstdint>
#include <cstring>

// ---------------------------------------------------------------------------
// buffer.c
// ---------------------------------------------------------------------------
TEST(HardenBuffer, NullAccessors) {
    EXPECT_EQ(buffer_check(nullptr, false), EM_ERR);
    EXPECT_EQ(buffer_check(nullptr, true), EM_ERR); // reduced==true darf NICHT derefen
    EXPECT_EQ(buffer_used(nullptr), 0);
    EXPECT_EQ(buffer_writeable(nullptr), 0);
    EXPECT_FALSE(buffer_is_used(nullptr));
    buffer_print(nullptr, nullptr); // darf nicht crashen
    EXPECT_EQ(buffer_new(0, LINEAR), nullptr);
}

TEST(HardenBuffer, NullDataAndSizes) {
    buffer_t *b = buffer_new(64, RING);
    ASSERT_NE(b, nullptr);
    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int16_t n = 8;

    EXPECT_EQ(buffer_set(nullptr, data, 8), EM_ERR);
    EXPECT_EQ(buffer_set(b, nullptr, 8), EM_ERR);
    EXPECT_EQ(buffer_set(b, data, 0), EM_ERR);
    EXPECT_EQ(buffer_set(b, data, -3), EM_ERR);

    EXPECT_EQ(buffer_get(nullptr, data, &n), EM_ERR);
    EXPECT_EQ(buffer_get(b, nullptr, &n), EM_ERR);
    EXPECT_EQ(buffer_get(b, data, nullptr), EM_ERR);

    buffer_free(b);
}

// Der frühere VLA scratch[len] wird jetzt in 64-Byte-Blöcken kopiert:
// ein RING->RING-Transfer > 64 Byte muss die Daten korrekt und ohne
// Stack-Overflow übertragen.
TEST(HardenBuffer, RingToRingChunkedTransfer) {
    buffer_t *from = buffer_new(256, RING);
    buffer_t *to = buffer_new(256, RING);
    ASSERT_NE(from, nullptr);
    ASSERT_NE(to, nullptr);

    uint8_t src[150];
    for (int i = 0; i < 150; i++)
        src[i] = (uint8_t)(i & 0xFF);
    ASSERT_EQ(buffer_set(from, src, 150), EM_OK);

    ASSERT_EQ(buffer_transfer(from, to), EM_OK);
    EXPECT_EQ(buffer_used(to), 150);
    EXPECT_EQ(buffer_used(from), 0);

    uint8_t dst[150] = {0};
    int16_t n = 150;
    ASSERT_EQ(buffer_get(to, dst, &n), EM_OK);
    EXPECT_EQ(n, 150);
    EXPECT_EQ(memcmp(src, dst, 150), 0);

    buffer_free(from);
    buffer_free(to);
}

// ---------------------------------------------------------------------------
// buffer_pool.c
// ---------------------------------------------------------------------------
TEST(HardenBufferPool, NullAndLifecycle) {
    EXPECT_EQ(buffer_pool_get(nullptr), nullptr);
    EXPECT_EQ(buffer_pool_return(nullptr, nullptr), EM_ERR);
    EXPECT_EQ(buffer_pool_print(nullptr, (char *)"x"), EM_ERR);
    buffer_pool_free(nullptr); // darf nicht crashen

    buffer_pool_t *bp = buffer_pool_new(3, 64, LINEAR, RING);
    ASSERT_NE(bp, nullptr);
    EXPECT_EQ(buffer_pool_return(bp, nullptr), EM_ERR);

    buffer_t *b = buffer_pool_get(bp);
    ASSERT_NE(b, nullptr);
    b->state = BUFFER_READY; // damit return nicht "used" ablehnt
    EXPECT_EQ(buffer_pool_return(bp, b), EM_OK);

    buffer_pool_free(bp);
}

// ---------------------------------------------------------------------------
// state.c
// ---------------------------------------------------------------------------
TEST(HardenState, NullGuards) {
    EXPECT_EQ(state_check(nullptr), EM_ERR);
    EXPECT_EQ(state_init(nullptr), EM_ERR);
    EXPECT_EQ(state_set(nullptr, 0, OFF), EM_ERR);
    (void)state_get(nullptr, 0); // darf nicht crashen
    EXPECT_EQ(state_propagate(nullptr, 0), EM_ERR);
    EXPECT_EQ(state_reset(nullptr), EM_ERR);
}

TEST(HardenState, OutOfRangeIndex) {
    state_t s;
    ASSERT_EQ(state_init(&s), EM_OK);
    EXPECT_EQ(state_set(&s, 200, ON), EM_ERR);      // nr >= cnt
    EXPECT_EQ(state_propagate(&s, 200), EM_ERR);    // idx ausserhalb [first,first+cnt)
}

// Cross-Buffer-Funktionen greifen einen zweiten state parallel ab -> ohne
// cnt-Gleichheit drohte ein OOB. Jetzt: sauberer Fehler-Return.
TEST(HardenState, MismatchedCntRejected) {
    state_t a, b;
    ASSERT_EQ(state_init(&a), EM_OK);
    ASSERT_EQ(state_init(&b), EM_OK);
    b.cnt = 4; // != a.cnt (MAX_STATE_CNT)
    EXPECT_EQ(state_add(&a, &b), EM_ERR);
    EXPECT_EQ(state_is_same(&a, &b), EM_ERR);
}

// ---------------------------------------------------------------------------
// device.c  (keine Geräte registriert -> alle Zugriffe müssen sauber failen)
// ---------------------------------------------------------------------------
TEST(HardenDevice, InvalidHandles) {
    uint8_t data[4] = {0};
    int16_t cnt = 4;
    EXPECT_EQ(device_check(-1, DEV_WRITE), EM_ERR);
    EXPECT_EQ(device_check(99, DEV_WRITE), EM_ERR);
    EXPECT_EQ(device_write(-1, data, 4), EM_ERR);
    EXPECT_EQ(device_write(99, data, 4), EM_ERR);
    EXPECT_EQ(device_write(0, nullptr, 4), EM_ERR);
    EXPECT_EQ(device_read(-1, data, &cnt), EM_ERR);
    EXPECT_EQ(device_read(0, nullptr, &cnt), EM_ERR);
    EXPECT_EQ(device_read(0, data, nullptr), EM_ERR);
    EXPECT_EQ(device_reset(-1), EM_ERR);
    device_print(-1); // darf nicht crashen
    device_print(99);
}

// ---------------------------------------------------------------------------
// common.c
// ---------------------------------------------------------------------------
TEST(HardenCommon, NullAndZero) {
    EXPECT_EQ(modulo_sub(5, 3, 0), 0);          // Division durch 0 vermieden
    EXPECT_EQ(to_hex(nullptr, 10, nullptr, 5, true), 0);
    print_buffer(nullptr, 0, "x");              // darf nicht crashen
    EXPECT_STREQ(idx2str(nullptr, 0, 0), "NA ");
    EXPECT_STREQ(idxa2str(nullptr, 0), "NA ");
    EXPECT_EQ(board_get_unique_id(nullptr, 16), 0u);
    clabel_u lbl;
    lbl.cmd = 0;
    EXPECT_EQ(clable2type(nullptr), nonasci);
    EXPECT_EQ(clabel2uint(nullptr), -1);
}
