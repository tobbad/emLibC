/*
 * common_test.cpp
 *
 *  Created on: 24.03.2017
 *      Author: badi
 */
#include "common.h"
#include <stdint.h>
#include <string.h>
#include "gtest/gtest.h"

// ---------------------------------------------------------------------------
// CRC-16 (x-25 / CRC-16/IBM-SDLC with byte-swapped output)
// ---------------------------------------------------------------------------
class Crc16Test : public ::testing::Test {};

TEST_F(Crc16Test, LengthZeroReturnsZero) {
    // length==0 branch: returns ~0xffff == 0x0000
    EXPECT_EQ(common_crc16(nullptr, 0), 0x0000);
}

TEST_F(Crc16Test, SingleByte0xFF) {
    // For 0xFF all data bits match CRC LSB every iteration → pure right-shifts.
    // crc after loop = 0x00FF, ~crc = 0xFFFFFF00, byte-swap yields 0x00FF.
    uint8_t data = 0xFF;
    EXPECT_EQ(common_crc16(&data, 1), 0x00FFu);
}

TEST_F(Crc16Test, KnownVector123456789) {
    // Standard CRC-16/IBM-SDLC check value for "123456789" is 0x906E.
    // This implementation byte-swaps the final result, giving 0x6E90.
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    EXPECT_EQ(common_crc16(data, sizeof(data)), 0x6E90u);
}

TEST_F(Crc16Test, IsDeterministic) {
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(common_crc16(data, sizeof(data)),
              common_crc16(data, sizeof(data)));
}

TEST_F(Crc16Test, DifferentInputsDifferentCRC) {
    uint8_t a = 0x00;
    uint8_t b = 0x01;
    EXPECT_NE(common_crc16(&a, 1), common_crc16(&b, 1));
}

// ---------------------------------------------------------------------------
// int2hchar
// ---------------------------------------------------------------------------
class Int2HcharTest : public ::testing::Test {};

TEST_F(Int2HcharTest, Digits0to9) {
    EXPECT_EQ(int2hchar(0), '0');
    EXPECT_EQ(int2hchar(5), '5');
    EXPECT_EQ(int2hchar(9), '9');
}

TEST_F(Int2HcharTest, HexLettersAtoF) {
    EXPECT_EQ(int2hchar(10), 'A');
    EXPECT_EQ(int2hchar(11), 'B');
    EXPECT_EQ(int2hchar(15), 'F');
}

TEST_F(Int2HcharTest, OutOfRangeReturnsSpace) {
    // MAX_STATE_CNT == MAX_BUTTON_CNT == 16; values >= 16 return ' '
    EXPECT_EQ(int2hchar(16), ' ');
    EXPECT_EQ(int2hchar(20), ' ');
    EXPECT_EQ(int2hchar(255), ' ');
}

// ---------------------------------------------------------------------------
// str2uint / clabel2uint
// ---------------------------------------------------------------------------
class Str2UintTest : public ::testing::Test {};

TEST_F(Str2UintTest, Zero) {
    EXPECT_EQ(str2uint((char *)"0"), 0);
}

TEST_F(Str2UintTest, PositiveValue) {
    EXPECT_EQ(str2uint((char *)"42"), 42);
    EXPECT_EQ(str2uint((char *)"127"), 127);
}

TEST_F(Str2UintTest, Clabel2UintMatchesStr2Uint) {
    clabel_u lbl;
    memcpy(lbl.str, "10", 3);
    lbl.str[3] = '\0';
    EXPECT_EQ(clabel2uint(&lbl), str2uint((char *)"10"));
}

// ---------------------------------------------------------------------------
// clable2type
// ---------------------------------------------------------------------------
class Clable2TypeTest : public ::testing::Test {
  protected:
    clabel_u lbl;
    void fill(const char *s) {
        memset(lbl.str, 0, CMD_LEN);
        strncpy(lbl.str, s, CMD_LEN - 1);
    }
};

TEST_F(Clable2TypeTest, DecimalNumberIsHexnum) {
    fill("123");
    EXPECT_EQ(clable2type(&lbl), hexnum);
}

TEST_F(Clable2TypeTest, ZeroIsHexnum) {
    fill("0");
    EXPECT_EQ(clable2type(&lbl), hexnum);
}

TEST_F(Clable2TypeTest, NegativeDecimalIsHexnum) {
    fill("-1");
    EXPECT_EQ(clable2type(&lbl), hexnum);
}

TEST_F(Clable2TypeTest, AlphanumNonDecimalIsAscii) {
    fill("AB1");
    EXPECT_EQ(clable2type(&lbl), ascii);
}

TEST_F(Clable2TypeTest, PureAlphaIsAscii) {
    fill("abc");
    EXPECT_EQ(clable2type(&lbl), ascii);
}

TEST_F(Clable2TypeTest, NonAsciiBytesIsNonasci) {
    lbl.str[0] = (char)0x80; // not in ASCII range
    lbl.str[1] = '\0';
    lbl.str[2] = '\0';
    lbl.str[3] = '\0';
    EXPECT_EQ(clable2type(&lbl), nonasci);
}

// ---------------------------------------------------------------------------
// to_hex
// ---------------------------------------------------------------------------
// Line layout (16 bytes per row, write_asci=false):
//   "0xXXXX " (7) + 16 * "XX " (48) + 2 SP78 after byte 7 + 2 SP78 after byte 15 + "\n" (1) = 60
// With write_asci=true: + 16 ASCII chars = 76
static const uint16_t LINE_NO_ASCII  = 60;
static const uint16_t LINE_WITH_ASCII = 76;

class ToHexTest : public ::testing::Test {
  protected:
    static const size_t OUT_SIZE = 512;
    char out[OUT_SIZE];

    void SetUp() override { memset(out, 0, OUT_SIZE); }
};

TEST_F(ToHexTest, EmptyBufferReturnsZero) {
    EXPECT_EQ(to_hex(out, OUT_SIZE, nullptr, 0, false), 0);
}

TEST_F(ToHexTest, SingleByteReturnsCorrectLength) {
    uint8_t buf[1] = {0x41};
    uint16_t len = to_hex(out, OUT_SIZE, buf, 1, false);
    EXPECT_EQ(len, LINE_NO_ASCII);
    EXPECT_EQ(len, (uint16_t)strlen(out));
}

TEST_F(ToHexTest, AsciiModeAdds16Chars) {
    uint8_t buf[1] = {0x41};
    char out2[OUT_SIZE] = {0};
    uint16_t len_no  = to_hex(out,  OUT_SIZE, buf, 1, false);
    uint16_t len_yes = to_hex(out2, OUT_SIZE, buf, 1, true);
    EXPECT_EQ(len_yes - len_no, 16u);
    EXPECT_EQ(len_yes, LINE_WITH_ASCII);
}

TEST_F(ToHexTest, OutputStartsWithAddress0x0000) {
    uint8_t buf[1] = {0x55};
    to_hex(out, OUT_SIZE, buf, 1, false);
    EXPECT_NE(strstr(out, "0x0000"), nullptr);
}

TEST_F(ToHexTest, OutputContainsHexEncodedByte) {
    uint8_t buf[1] = {0xAB};
    to_hex(out, OUT_SIZE, buf, 1, false);
    EXPECT_NE(strstr(out, "AB"), nullptr);
}

TEST_F(ToHexTest, NonPrintableByteShowsAsDot) {
    uint8_t buf[1] = {0x01};
    to_hex(out, OUT_SIZE, buf, 1, true);
    EXPECT_NE(strstr(out, "."), nullptr);
}

TEST_F(ToHexTest, PrintableByteShowsAsItselfInAsciiMode) {
    uint8_t buf[1] = {'A'};
    to_hex(out, OUT_SIZE, buf, 1, true);
    // The ASCII column should contain 'A'
    // ASCII part is appended after the hex dump, before '\n'
    char *nl = strchr(out, '\n');
    ASSERT_NE(nl, nullptr);
    // Search only in the ASCII tail (last 17 chars before \n)
    char *ascii_start = nl - 16;
    ASSERT_GE(ascii_start, out);
    EXPECT_EQ(ascii_start[0], 'A');
}

TEST_F(ToHexTest, TwoRowsFor32Bytes) {
    uint8_t buf[32] = {0};
    uint16_t len = to_hex(out, OUT_SIZE, buf, sizeof(buf), false);
    EXPECT_EQ(len, 2 * LINE_NO_ASCII);
    EXPECT_NE(strstr(out, "0x0000"), nullptr);
    EXPECT_NE(strstr(out, "0x0010"), nullptr);
}

TEST_F(ToHexTest, ReturnedLengthMatchesStrlen) {
    uint8_t buf[48] = {0};
    for (uint8_t i = 0; i < sizeof(buf); i++) buf[i] = i;
    uint16_t len = to_hex(out, OUT_SIZE, buf, sizeof(buf), true);
    EXPECT_EQ(len, (uint16_t)strlen(out));
}

// ---------------------------------------------------------------------------
// idx2str / idxa2str
// ---------------------------------------------------------------------------
class Idx2StrTest : public ::testing::Test {
  protected:
    idx2str_t map[3];
    void SetUp() override {
        strncpy(map[0].str, "zero",  sizeof(map[0].str)); map[0].idx = 0;
        strncpy(map[1].str, "one",   sizeof(map[1].str)); map[1].idx = 1;
        strncpy(map[2].str, "two",   sizeof(map[2].str)); map[2].idx = 2;
    }
};

TEST_F(Idx2StrTest, FindsExistingEntry) {
    EXPECT_STREQ(idx2str(map, 3, 0), "zero");
    EXPECT_STREQ(idx2str(map, 3, 1), "one");
    EXPECT_STREQ(idx2str(map, 3, 2), "two");
}

TEST_F(Idx2StrTest, MissingIndexReturnsNA) {
    EXPECT_STREQ(idx2str(map, 3, 5), "NA ");
}

TEST_F(Idx2StrTest, EmptyMapReturnsNA) {
    EXPECT_STREQ(idx2str(map, 0, 0), "NA ");
}

TEST_F(Idx2StrTest, Idxa2StrWrapper) {
    idxa2str_t amap = {3, map};
    EXPECT_STREQ(idxa2str(&amap, 0), "zero");
    EXPECT_STREQ(idxa2str(&amap, 2), "two");
    EXPECT_STREQ(idxa2str(&amap, 9), "NA ");
}
