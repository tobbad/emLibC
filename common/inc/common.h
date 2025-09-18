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
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifndef UNIT_TEST
#include "hal_port.h"
#endif
#ifdef UNIT_TEST
#define STATIC
#include <stdio.h>
#else
#define STATIC static
#endif
#define ELCNT(array) (array==0)?0:(sizeof((array))/sizeof((array[0])))

#define NEWLINE  "\r\n"
#define NL  NEWLINE
#define LINE_LENGTH	96 // FIXME -> CHAR_PER_LINE
#define MAX_BUTTON_CNT 16
#if !defined(MIN)
#define MIN(a, b) ((a)<=(b)?(a):(b))
#endif
#if !defined(MAX)
#define MAX(a, b) ((a)>=(b)?(a):(b))
#endif

#if !defined(MAX)
#define MAX(a, b) ((a)>(b)?(a):(b))
#endif

typedef enum {
	EM_ERR = -1,
	EM_OK = 0,
    EM_TRUE,
} em_msg;

typedef union {
    const uint8_t *cptr;
    uint8_t *ptr;
} cPtrAway_u;

#define ISNUM       0x80
#define ISASCISTR   0x40
#define NAN         0x20
#define CMD_LEN 4

#define DEV_CNT 1
typedef union {
     uint32_t cmd;  // Kann ein pointer zu einem Pointer enthalten, das
                    // den anderen Geräten mitgeteilt wird oder NULL
     char str[CMD_LEN]; //CMD_LEN ist 4 ist leer (0) oder ein command
 }clabel_u;
#define ZERO4 ((32<<24)+(32<<16)+(32<<8)+32) // is "     " as str
/*
 * Function to serialize the content of buffer as neaty formated
 * string in out. Format is:
 * 0xXXXX xx xx xx xx xx xx xx xx xx  xx xx xx xx xx xx xx xx  aaaaaaaa aaaaaaaa
 * XXXX = Addres relative to start of buffer of the 16 following bytes
 * xx   = Hex encoded data byte
 * a    = printable asci otherwise "."
 */
uint16_t to_hex(char *out, uint16_t out_size, uint8_t *buffer, uint16_t buffer_size, bool write_asci);
uint16_t common_crc16(uint8_t *data_p, uint16_t length);
void PrintBuffer(uint8_t *buffer, uint8_t size, char *header);
uint8_t clable2type(clabel_u *lbl);
int8_t clabel2uint8(clabel_u *lbl);
char int2hchar(uint8_t idx);

#ifdef __cplusplus
}
#endif
#endif /* INC_EMLIB_COMMON_H_ */
