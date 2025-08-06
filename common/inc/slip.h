/*
 * slip.h
 *
 * Stream based slip based encoding and decoding as used in HCI UART transport
 *
 * 1. Packet starts with 0xC0
 * 2. Any occurence of 0xC0 and 0xDB (Escape code) is escaped:
 *    0xC0 <-> 0xDB 0xDC
 *    0xDB <-> 0xDB 0xDD
 * 3. Optional following codes are escaped if the out of frame
 *    flow control is enabled:
 *    0x11 <-> 0xDB 0xDE
 *    0x13 <-> 0xDB 0xDF
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */

#ifndef COMMON_INC_SLIP_H_
#define COMMON_INC_SLIP_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"

typedef enum {
    SLIP_ENCODE_SIMPLE,
    SLIP_DECODE_SIMPLE,
    SLIP_ENCODE_OOF_FLOW_CONTROL,
    SLIP_DECODE_OOF_FLOW_CONTROL,
    SLIP_STATE_CNT,
} slip_function_t;

typedef enum {
    SLIP_HANDLE_ERROR = EM_ERR,
    SLIP_HANDLE_0,
    SLIP_HANDLE_1,
    SLIP_HANDLE_CNT,
} slip_handle_e;

#define SLIP_PKT_LIMIT 0xC0
#define SLIP_PKT_LIMIT_ESC 0xDC
#define SLIP_ESCAPE 0xDB
#define SLIP_ESCAPE_ESC 0xDD
#define SLIP_DC1 0x11
#define SLIP_DC1_ESC 0xDE
#define SLIP_DC3 0x13
#define SLIP_DC3_ESC 0xDF

/*
 * Map of values (col 0)
 * to escaped values (col 1)
 */
extern const uint8_t slip_map[][2];
//
void slip_init(void);
slip_handle_e slip_start(device_t *dev, slip_function_t state);
em_msg slip_write(slip_handle_e hdl, const uint8_t * buffer, uint16_t length);
uint16_t slip_end(slip_handle_e hdl);

#ifdef __cplusplus
}
#endif
#endif /* COMMON_INC_SLIP_H_ */
