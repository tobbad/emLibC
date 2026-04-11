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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#ifndef UNIT_TEST
#include "hal_port.h"
#endif
#ifdef UNIT_TEST
#define STATIC
#include <stdio.h>
#else
#define STATIC static
#endif
#define ELCNT(array) (array == 0) ? 0 : (sizeof((array)) / sizeof((array[0])))

//#ifdef UNIT_TEST
//#warning "Building emLibC with UNIT_TEST enabled"
//#endif

#define NEWLINE "\r\n"
#define NL NEWLINE
#define LINE_LENGTH 96 // FIXME -> CHAR_PER_LINE
#define MAX_BUTTON_CNT 16

#ifndef MIN
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef enum {
  EM_ERR = -1,
  EM_OK = 0,
  EM_TRUE,
} em_msg;

typedef enum {
  SYNC_RESET,
  BOOT_UP,
  SLOT,
  CHANNEL,
  FREQBAND,
  FREQUENCY_OFFSET,
  SYNCHRONIZE,
  SYNCHRONIZE_READY,
  SYNCHRONIZE_DOING,
  SYNCHRONIZE_ERROR,
  SYNCHRONIZE_OK,
  SYNC_CNT
} system_state_e;

typedef union {
  const uint8_t *cptr;
  uint8_t *ptr;
} cPtrAway_u;

typedef enum  {
    hexnum  = 0x10,
    ascii   = 0x20,
    nonasci = 0x80
}type_e;

#define CMD_LEN 4

#define DEV_CNT 1
typedef union {
     uint32_t cmd;  // Kann ein pointer zu einem Pointer enthalten, das
                    // den anderen Geräten mitgeteilt wird oder NULL
     char str[CMD_LEN]; //CMD_LEN ist 4 ist leer (0) oder ein command
 }clabel_u;
#define ZERO4 ((32<<24)+(32<<16)+(32<<8)+32) // is "     " as str

 typedef struct idx2str_s {
     char    *str;
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
size_t board_get_unique_id(uint8_t id[], size_t max_len);
uint16_t to_hex(char *out, uint16_t out_size, uint8_t *buffer,
                uint16_t buffer_size, bool write_asci);
uint16_t common_crc16(const uint8_t *data_p, uint16_t length);
uint8_t modulo_sub(int8_t slot, int8_t oSlot, uint8_t modulo);
void print_buffer(const uint8_t *buffer, uint8_t size, const char *header);
type_e clable2type(clabel_u *lbl);
int8_t str2uint(char *str);
int8_t clabel2uint(clabel_u *lbl);
char* idxa2str(idxa2str_t *map, uint8_t idx);
char* idx2str(idx2str_t *map, uint8_t cnt, uint8_t idx);
char int2hchar(uint8_t idx);

#ifdef __cplusplus
}
#endif
#endif /* INC_EMLIB_COMMON_H_ */
