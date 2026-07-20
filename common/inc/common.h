/*
 * common.h
 *
 *  Created on: 09.12.2015
 *      Author: badi
 */

#ifndef INC_EMLIB_COMMON_H_
#define INC_EMLIB_COMMON_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math.h"

#ifndef UNIT_TEST
#include "hal_port.h"
#else
#define STATIC static
#endif

#define ELCNT(array) (array == 0) ? 0 : (sizeof((array)) / sizeof((array[0])))
// #ifdef UNIT_TEST
// #warning "Building emLibC with UNIT_TEST enabled"
// #endif

#define NEWLINE "\r\n"
#define NEW_LINE_LEN strlen(NEWLINE)
#define NL NEWLINE
#define TRUNCT_NL "*" NL
#define TRUCT_NL_LEN strlen(TRUNCT_NL)
#define LINE_LENGTH 96 // FIXME -> CHAR_PER_LINE
#define MAX_BUTTON_CNT 16

#ifndef MIN
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef enum {
    EM_ERR = -1,
    EM_OK = 0,
    EM_TRUE,
} em_msg;

/*
 * Hardening guards. Every public entry point validates its inputs and returns
 * a safe value instead of crashing on NULL / out-of-range arguments. The guards
 * are silent by default; a diagnostic is emitted only when OPTION_VERBOSE == 1
 * (see Core/Inc/options.h) so the firmware/ISR build stays free of printf cost.
 */
#ifndef OPTION_VERBOSE
#define OPTION_VERBOSE 0
#endif

#if OPTION_VERBOSE == 1
#define EM_GUARD_LOG(msg) printf("%s: %s" NL, __func__, (msg))
#else
#define EM_GUARD_LOG(msg) ((void)0)
#endif

/* Return `ret` (logging `#cond`) when `cond` holds -- the invalid-input case. */
#define EM_RETURN_IF(cond, ret)                                                \
    do {                                                                       \
        if (cond) {                                                            \
            EM_GUARD_LOG(#cond);                                               \
            return (ret);                                                      \
        }                                                                      \
    } while (0)

/* Common specialisation: bail out when a required pointer is NULL. */
#define EM_RETURN_IF_NULL(ptr, ret) EM_RETURN_IF((ptr) == NULL, ret)

typedef enum {
    SYNC_RESET,          // 0
    BOOT_UP,             // 1
    SLOT,                // 2
    CHANNEL,             // 3
    FREQBAND,            // 4
    FREQUENCY_OFFSET,    // 5
    SYNCHRONIZE,         // 6
    SYNCHRONIZE_READY,   // 7
    SYNCHRONIZE_DOING,   // 8
    SYNCHRONIZE_ERROR,   // 9
    SYNCHRONIZE_LOCKED,  // 10
    SYNCHRONIZE_OK,      // 11
    SYNC_CNT             // 12
} system_state_e;

typedef union {
    const uint8_t *cptr;
    uint8_t *ptr;
} cPtrAway_u;

typedef enum { hexnum = 0x10, ascii = 0x20, nonasci = 0x80 } type_e;

#define CMD_LEN 4
#define DEV_CNT 1
typedef union {
    uint32_t cmd;      // Kann ein pointer zu einem Pointer enthalten, das
                       // den anderen Geräten mitgeteilt wird oder NULL
    char str[CMD_LEN]; // CMD_LEN ist 4 ist leer (0) oder ein command
} clabel_u;
#define ZERO4 ((32 << 24) + (32 << 16) + (32 << 8) + 32) // is "     " as str

typedef struct idx2str_s {
    char *str;
    uint8_t idx;
} idx2str_t;

typedef struct idxa2str_s {
    uint8_t cnt;
    idx2str_t *entry;
} idxa2str_t;

extern idxa2str_t synca2str;
/*
 * Function to serialize the content of buffer as neaty formated
 * string in out. Format is:
 * 0xXXXX xx xx xx xx xx xx xx xx xx  xx xx xx xx xx xx xx xx  aaaaaaaa aaaaaaaa
 * XXXX = Addres relative to start of buffer of the 16 following bytes
 * xx   = Hex encoded data byte
 * a    = printable asci otherwise "."
 */
size_t board_get_unique_id(uint8_t *id, size_t max_len);
uint32_t csss2uint32(uint32_t cycle, uint8_t slot, uint8_t sSlot);
uint16_t to_hex(char *out, uint16_t out_size, uint8_t *buffer, uint16_t buffer_size, bool write_asci);
uint16_t common_crc16(const uint8_t *data_p, uint16_t length);
uint8_t modulo_sub(int8_t slot, int8_t oSlot, uint8_t modulo);
void print_buffer(const uint8_t *buffer, uint8_t size, const char *header);
type_e clable2type(clabel_u *lbl);
int16_t uint_pow(unsigned base, unsigned exp);
int8_t str2uint(char *str);
int8_t clabel2uint(clabel_u *lbl);
char *idxa2str(idxa2str_t *map, uint8_t idx);
char *idx2str(idx2str_t *map, uint8_t cnt, uint8_t idx);
int in_interrupt(void);
char int2hchar(uint8_t idx);
int8_t int8bit_cnt(int8_t val);
uint32_t swap(uint32_t val);
bool ReadModify_write(int8_t *mem, int8_t add);

#ifdef __cplusplus
}
#endif
#endif /* INC_EMLIB_COMMON_H_ */
