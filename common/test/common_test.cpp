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
    uint8_t data = 0xFF;
    EXPECT_EQ(common_crc16(&data, 1), 0x00FFu);
}

TEST_F(Crc16Test, SingleByte0x00DiffersFromLengthZero) {
    uint8_t data = 0x00;
    EXPECT_NE(common_crc16(&data, 1), 0x0000u);
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

TEST_F(Crc16Test, LengthSensitive) {
    // One zero byte vs two zero bytes must produce different CRCs.
    uint8_t buf[2] = {0x00, 0x00};
    EXPECT_NE(common_crc16(buf, 1), common_crc16(buf, 2));
}

// ---------------------------------------------------------------------------
// int2hchar
// ---------------------------------------------------------------------------
class Int2HcharTest : public ::testing::Test {};

TEST_F(Int2HcharTest, AllValidHexDigits) {
    const char expected[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < 16; i++) {
        EXPECT_EQ(int2hchar(i), expected[i]) << "for i=" << (int)i;
    }
}

TEST_F(Int2HcharTest, OutOfRangeReturnsSpace) {
    // MAX_STATE_CNT == MAX_BUTTON_CNT == 16; values >= 16 return ' '
    EXPECT_EQ(int2hchar(16),  ' ');
    EXPECT_EQ(int2hchar(20),  ' ');
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
    EXPECT_EQ(str2uint((char *)"42"),  42);
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

TEST_F(Clable2TypeTest, EmptyStringIsHexnum) {
    // strtol("", &stop, 10) → stop zeigt auf str[0], strlen(stop)==0 → hexnum
    fill("");
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
    lbl.str[0] = (char)0x80;
    lbl.str[1] = '\0';
    lbl.str[2] = '\0';
    lbl.str[3] = '\0';
    EXPECT_EQ(clable2type(&lbl), nonasci);
}

// ---------------------------------------------------------------------------
// to_hex
// Row layout (16 bytes, no ASCII):
//   "0xXXXX " (7) + 16 * "XX " (48) + SP78 nach Byte 7 (2) + SP78 nach Byte 15 (2) + "\n" (1) = 60
// Mit ASCII: + 16 Zeichen = 76
// ---------------------------------------------------------------------------
static const uint16_t LINE_NO_ASCII   = 60;
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
    char out2[OUT_SIZE] = {};
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
    char *nl = strchr(out, '\n');
    ASSERT_NE(nl, nullptr);
    char *ascii_start = nl - 16;
    ASSERT_GE(ascii_start, out);
    EXPECT_EQ(ascii_start[0], 'A');
}

TEST_F(ToHexTest, TwoRowsFor32Bytes) {
    uint8_t buf[32] = {};
    uint16_t len = to_hex(out, OUT_SIZE, buf, sizeof(buf), false);
    EXPECT_EQ(len, 2 * LINE_NO_ASCII);
    EXPECT_NE(strstr(out, "0x0000"), nullptr);
    EXPECT_NE(strstr(out, "0x0010"), nullptr);
}

TEST_F(ToHexTest, ReturnedLengthMatchesStrlen) {
    uint8_t buf[48] = {};
    for (uint8_t i = 0; i < sizeof(buf); i++) buf[i] = i;
    uint16_t len = to_hex(out, OUT_SIZE, buf, sizeof(buf), true);
    EXPECT_EQ(len, (uint16_t)strlen(out));
}

TEST_F(ToHexTest, ExactRowStructure) {
    // Prüft die genauen Byte-Positionen einer vollständigen 16-Byte-Zeile.
    // [0..6]   "0x0000 "  — Adresse
    // [7..9]   "00 "      — Byte 0
    // [28..30] "07 "      — Byte 7  (7 + 7*3 = 28)
    // [31..32] "  "       — SP78 nach Byte 7
    // [33..35] "08 "      — Byte 8
    // [54..56] "0F "      — Byte 15 (33 + 7*3 = 54)
    // [57..58] "  "       — SP78 nach Byte 15
    // [59]     '\n'
    uint8_t buf[16];
    for (int i = 0; i < 16; i++) buf[i] = (uint8_t)i;
    to_hex(out, OUT_SIZE, buf, 16, false);

    EXPECT_EQ(strncmp(out,      "0x0000 ", 7), 0);
    EXPECT_EQ(strncmp(out +  7, "00 ",     3), 0);
    EXPECT_EQ(strncmp(out + 28, "07 ",     3), 0);
    EXPECT_EQ(strncmp(out + 31, "  ",      2), 0);
    EXPECT_EQ(strncmp(out + 33, "08 ",     3), 0);
    EXPECT_EQ(strncmp(out + 54, "0F ",     3), 0);
    EXPECT_EQ(strncmp(out + 57, "  ",      2), 0);
    EXPECT_EQ(out[59], '\n');
}

// ---------------------------------------------------------------------------
// board_get_unique_id (UNIT_TEST-Stub: id[i] = i für i < 12)
// ---------------------------------------------------------------------------
class BoardGetUniqueIdTest : public ::testing::Test {};

TEST_F(BoardGetUniqueIdTest, ReturnsMaxLen) {
    uint8_t id[16] = {};
    EXPECT_EQ(board_get_unique_id(id, 16), 16u);
}

TEST_F(BoardGetUniqueIdTest, First12BytesAreIndex) {
    uint8_t id[12] = {};
    board_get_unique_id(id, 12);
    for (uint8_t i = 0; i < 12; i++) {
        EXPECT_EQ(id[i], i) << "at index " << (int)i;
    }
}

TEST_F(BoardGetUniqueIdTest, PaddingBeyond12IsZero) {
    uint8_t id[16] = {};
    board_get_unique_id(id, 16);
    for (uint8_t i = 12; i < 16; i++) {
        EXPECT_EQ(id[i], 0u) << "at index " << (int)i;
    }
}

TEST_F(BoardGetUniqueIdTest, SmallerThan12BytesClipped) {
    uint8_t id[4] = {};
    board_get_unique_id(id, 4);
    for (uint8_t i = 0; i < 4; i++) {
        EXPECT_EQ(id[i], i);
    }
}

// ---------------------------------------------------------------------------
// modulo_sub
// ---------------------------------------------------------------------------
class ModuloSubTest : public ::testing::Test {};

TEST_F(ModuloSubTest, EqualSlotsReturnsZero) {
    EXPECT_EQ(modulo_sub(3, 3, 10), 0u);
    EXPECT_EQ(modulo_sub(0, 0,  8), 0u);
}

TEST_F(ModuloSubTest, SlotBeforeOSlot) {
    // slot < oSlot → else-Zweig: (oSlot + modulo - slot) % modulo = oSlot - slot
    EXPECT_EQ(modulo_sub(2, 5, 10), 3u);
    EXPECT_EQ(modulo_sub(0, 7,  8), 7u);
}

TEST_F(ModuloSubTest, SlotAfterOSlotWrapsAsUint8) {
    // slot > oSlot → if-Zweig: liefert (uint8_t)(oSlot - slot), NICHT den
    // korrekten zirkulären Abstand (oSlot - slot + modulo) % modulo.
    // Dokumentiert das bekannte Verhalten des if-Zweigs.
    // modulo_sub(5, 3, 10): 3-5 = -2 → (uint8_t)254, erwartet wären 8.
    EXPECT_EQ(modulo_sub(5, 3, 10), (uint8_t)(3 - 5));
}

// ---------------------------------------------------------------------------
// idx2str / idxa2str
// ---------------------------------------------------------------------------
class Idx2StrTest : public ::testing::Test {
  protected:
    idx2str_t map[3];
    void SetUp() override {
        // String-Literale direkt zuweisen — kein strncpy auf uninitialisierten char*.
        map[0] = { (char *)"zero", 0 };
        map[1] = { (char *)"one",  1 };
        map[2] = { (char *)"two",  2 };
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

// ---------------------------------------------------------------------------
// synca2str — globale Lookup-Tabelle für system_state_e
// ---------------------------------------------------------------------------
class SyncA2StrTest : public ::testing::Test {};

TEST_F(SyncA2StrTest, AllStatesResolvable) {
    // Jeder Zustand in [SYNC_RESET, SYNC_CNT) muss einen Eintrag haben.
    for (int s = SYNC_RESET; s < SYNC_CNT; s++) {
        EXPECT_STRNE(idxa2str(&synca2str, (uint8_t)s), "NA ")
            << "state " << s << " fehlt in synca2str";
    }
}

TEST_F(SyncA2StrTest, BootUpContainsName) {
    EXPECT_NE(strstr(idxa2str(&synca2str, BOOT_UP), "BOOT_UP"), nullptr);
}

TEST_F(SyncA2StrTest, SynchronizeOkContainsName) {
    EXPECT_NE(strstr(idxa2str(&synca2str, SYNCHRONIZE_OK), "SYNCHRONIZE_OK"), nullptr);
}

TEST_F(SyncA2StrTest, OutOfRangeReturnsNA) {
    EXPECT_STREQ(idxa2str(&synca2str, SYNC_CNT), "NA ");
}
