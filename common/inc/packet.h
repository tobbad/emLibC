/*
 * packet.h
 *
 *  Packets according to the HOST controller interface Three wire transport layer
 *
 *  Created on: 14.08.2020
 *      Author: badi
 */

#ifndef COMMON_INC_PACKET_H_
#define COMMON_INC_PACKET_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "common.h"

typedef struct packet_bitfield_s {
    uint8_t  seq_nr :3;
    uint8_t  ack_nr :3;
    bool     is_checked: 1; /* Data integrity check present 16 bit CCITT-CRC after payload */
    bool     is_reliable :1; /* Seq_nr is checked and packet is resent if not acknowledged */
    uint8_t  type :4;
    uint16_t length : 12; /* Length of the payload following this header without Integrity checks tail */
    uint8_t  chk_sum : 8; /* Header Checksum */
} packet_bitfield_t;

typedef struct packet_header_t_ {
    union {
        uint32_t head;
        packet_bitfield_t pkt;
    };
} packet_header_t;

typedef enum {
    PACKET_ACK = 0,
    PACKET_CH0,
    PACKET_CH1,
    PACKET_CH2,
    PACKET_CH3,
    PACKET_CH4,
    PACKET_CH5,
    PACKET_CH6,
    PACKET_CH7,
    PACKET_CH8,
    PACKET_CH9,
    PACKET_CH10,
    PACKET_CH11,
    PACKET_CH12,
    PACKET_CH13,
    PACKET_CH14,
    PACKET_LINK_CONTROL = 15,
} packet_t;
/**
 * Structure to keep the statemachine state and used variables
 * eg. for reliable transfers ...
 */
typedef struct packet_transfer_t_ {

} packet_transfer_t;
/**
 * Functions
 */
elres_t packet_init(void);
#ifdef __cplusplus
}
#endif

#endif /* COMMON_INC_PACKET_H_ */
