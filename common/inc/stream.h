/*
 * stream.h
 *
 *  Packets according to the HOST controller interface Three wire transport layer.
 *
 * Requirements:
 * 1st Priority: Unreliable transport without acknowledge.
 *   1. ack_nr is set to 0
 *   2. is_checked is set to 0
 *   3. is_reliable is set to 0
 *   4. type is set to channel opened when open a stream
 *   We allocate a fixed stream size amount of memory in the module given in the init.
 *   Each time we reach Block size we send a packet over the output device.
 *
 *
 *  Created on: 14.08.2020
 *      Author: badi
 */

#ifndef COMMON_INC_STREAM_H_
#define COMMON_INC_STREAM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "common.h"
#include "device.h"
#include "buffer.h"

typedef struct stream_dev_t_ {
    buffer_t send_buffer;
} stream_dev_t;

typedef enum {
    PACKET_SEND = DEV_CMD_LAST,
} stream_dev_command_t;

typedef uint8_t stream_transfer_t;
/**
 * Structure to keep the statemachine state and used variables
 * eg. for reliable transfers ...
 */
typedef struct stream_trstate_t_ {
    uint8_t state;
} stream_trstate_t;
/**
 * Functions
 */
em_msg stream_init(void );
/**
 * Open a stream transfer
 * \param outHdl Handle of the device for writting the stream to
 * \param trfType Type of transfer
 * \return OK on success
 */
em_msg stream_open(dev_handle_t outHdl, stream_transfer_t trfType);

em_msg stream_ioctrl(dev_handle_t outHdl);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_INC_STREAM_H_ */
