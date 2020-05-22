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

#ifdef UNIT_TEST
#define STATIC
#include <stdio.h>
#else
#define STATIC static
#endif

typedef enum {
    EMLIB_ERROR = -1,
    EMLIB_OK = 0,
} elres_t;

#define ELCNT(array) (sizeof((array))/sizeof((array[0])))

/*
 * Function to serialize the content of buffer as neaty formated
 * string in out. Format is:
 * 0xXXXX xx xx xx xx xx xx xx xx xx  xx xx xx xx xx xx xx xx  aaaaaaaa aaaaaaaa
 * XXXX = Addres relative to start of buffer of the 16 following bytes
 * xx   = Hex encoded data byte
 * a    = printable asci otherwise "."
 */
uint16_t to_hex(char *out, uint16_t out_size, uint8_t *buffer, uint16_t buffer_size, bool write_asci);

#ifdef __cplusplus
}
#endif
#endif /* INC_EMLIB_COMMON_H_ */
