/*
 * string_hash_test.cpp
 */
#include "common.h"
#include "string_hash.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <stdint.h>
#include <string.h>

class StringHashTest : public ::testing::Test {
  protected:
    sh_hash_map_t mp;
    void SetUp() override { ASSERT_EQ(sh_init(&mp), EM_OK); }
    void TearDown() override {}
};

TEST_F(StringHashTest, init) {
    EXPECT_EQ(mp.numOfElements, 0);
    for (int i = 0; i < SH_BUCKET_CNT; i++) {
        EXPECT_EQ(mp.bucket[i], nullptr);
    }
}

TEST_F(StringHashTest, hashSpreadsOverBuckets) {
    char key[SH_KEY_LEN];
    bool hit[SH_BUCKET_CNT] = {};
    int used = 0;
    for (int i = 0; i < SH_BUCKET_CNT * 4; i++) {
        snprintf(key, sizeof(key), "k%d", i);
        uint16_t b = sh_hash_function(key);
        ASSERT_LT(b, SH_BUCKET_CNT);
        if (!hit[b]) {
            hit[b] = true;
            used++;
        }
    }
    // Mindestens die Haelfte der Buckets muss belegt sein
    EXPECT_GE(used, SH_BUCKET_CNT / 2);
}

TEST_F(StringHashTest, insertSearch) {
    int8_t v = 0;
    ASSERT_EQ(sh_insert(&mp, "loop", 3), EM_OK);
    ASSERT_EQ(sh_insert(&mp, "rx", -7), EM_OK);
    EXPECT_EQ(sh_search(&mp, "loop", &v), EM_OK);
    EXPECT_EQ(v, 3);
    EXPECT_EQ(sh_search(&mp, "rx", &v), EM_OK);
    EXPECT_EQ(v, -7);
    EXPECT_EQ(sh_search(&mp, "tx", &v), EM_ERR);
    EXPECT_EQ(mp.numOfElements, 2);
}

TEST_F(StringHashTest, keyIsCopiedNotReferenced) {
    char buf[SH_KEY_LEN] = "abc";
    int8_t v = 0;
    ASSERT_EQ(sh_insert(&mp, buf, 1), EM_OK);
    strcpy(buf, "xyz");
    EXPECT_EQ(sh_search(&mp, "abc", &v), EM_OK);
    EXPECT_EQ(v, 1);
}

// Ein voller time_meas_t.line (TIME_MEAS_CHAR_PER_LINE Zeichen + '\0') wird
// samt Terminator gespeichert -> der Schluessel ist direkt ausgebbar.
TEST_F(StringHashTest, storedKeyIsTerminated) {
    char line[TIME_MEAS_CHAR_PER_LINE + 1];
    memset(line, 'x', TIME_MEAS_CHAR_PER_LINE);
    line[TIME_MEAS_CHAR_PER_LINE] = '\0';
    ASSERT_EQ(sh_insert(&mp, line, 1), EM_OK);
    const sh_node_t *node = mp.bucket[sh_hash_function(line)];
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->key[TIME_MEAS_CHAR_PER_LINE], '\0');
    EXPECT_STREQ(node->key, line);
}

TEST_F(StringHashTest, insertExistingOverwrites) {
    int8_t v = 0;
    ASSERT_EQ(sh_insert(&mp, "a", 1), EM_OK);
    ASSERT_EQ(sh_insert(&mp, "a", 2), EM_OK);
    EXPECT_EQ(mp.numOfElements, 1);
    EXPECT_EQ(sh_search(&mp, "a", &v), EM_OK);
    EXPECT_EQ(v, 2);
}

TEST_F(StringHashTest, keyLength) {
    char maxKey[SH_KEY_LEN];
    memset(maxKey, 'k', SH_KEY_LEN - 1);
    maxKey[SH_KEY_LEN - 1] = '\0';
    EXPECT_EQ(sh_insert(&mp, maxKey, 1), EM_OK);

    char tooLong[SH_KEY_LEN + 1];
    memset(tooLong, 'k', SH_KEY_LEN);
    tooLong[SH_KEY_LEN] = '\0';
    EXPECT_EQ(sh_insert(&mp, tooLong, 1), EM_ERR);
    EXPECT_EQ(sh_insert(&mp, nullptr, 1), EM_ERR);
}

TEST_F(StringHashTest, deleteFromChain) {
    // Mehr Schluessel als Buckets erzwingt Kollisionen
    char key[SH_KEY_LEN];
    const int cnt = SH_BUCKET_CNT * 2;
    for (int i = 0; i < cnt; i++) {
        snprintf(key, sizeof(key), "k%d", i);
        ASSERT_EQ(sh_insert(&mp, key, (int8_t)i), EM_OK);
    }
    for (int i = 0; i < cnt; i += 2) {
        snprintf(key, sizeof(key), "k%d", i);
        ASSERT_EQ(sh_delete(&mp, key), EM_OK);
    }
    EXPECT_EQ(mp.numOfElements, cnt / 2);
    for (int i = 0; i < cnt; i++) {
        int8_t v = 0;
        snprintf(key, sizeof(key), "k%d", i);
        if (i % 2) {
            EXPECT_EQ(sh_search(&mp, key, &v), EM_OK) << key;
            EXPECT_EQ(v, i);
        } else {
            EXPECT_EQ(sh_search(&mp, key, &v), EM_ERR) << key;
        }
    }
    EXPECT_EQ(sh_delete(&mp, "k0"), EM_ERR);
}

TEST_F(StringHashTest, poolFull) {
    char key[SH_KEY_LEN];
    for (int i = 0; i < SH_SIZE; i++) {
        snprintf(key, sizeof(key), "k%d", i);
        ASSERT_EQ(sh_insert(&mp, key, 0), EM_OK);
    }
    EXPECT_EQ(sh_insert(&mp, "overflow", 0), EM_ERR);
    // Freigegebener Knoten ist wieder verwendbar
    ASSERT_EQ(sh_delete(&mp, "k5"), EM_OK);
    EXPECT_EQ(sh_insert(&mp, "overflow", 0), EM_OK);
}
