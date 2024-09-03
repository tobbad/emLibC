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

#ifdef UNIT_TEST
#define STATIC
#include <stdio.h>
#else
#define STATIC static
#endif

#define ELCNT(array) (sizeof((array))/sizeof((array[0])))

#define NEWLINE  "\r\n"
#define NL  NEWLINE

#if !defined(MIN)
#define MIN(a, b) ((a)<(b)?(a):(b))
#endif

#if !defined(MAX)
#define MAX(a, b) ((a)>(b)?(a):(b))
#endif

typedef enum {
    EMLIB_NOT_READY = -2,
    EMLIB_ERROR = -1,
    EMLIB_OK = 0,
} elres_t;

typedef union {
    const uint8_t *cptr;
    uint8_t *ptr;
} cPtrAway_u;


typedef struct buffer_t_ {
    uint16_t size;
    uint16_t used; /* used count of bytes pl[used] is  the to next usable byte */
    uint8_t* pl;   /* pointer to first byte used in buffer */
    uint8_t* mem;  /* Start of memory */
} buffer_t;

#define DEV_CNT 1
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

#ifdef __cplusplus
}
#endif
#endif /* INC_EMLIB_COMMON_H_ */
