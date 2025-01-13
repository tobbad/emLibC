/*
 * packet.h
 *
 *  Created on: 30.10.2020
 *      Author: badi
 *
 * Module to take care of a packet transfer:
 * - On not reliable packets without check return immediatly after write
 * - Rest TBD later
 */

#ifndef COMMON_INC_PACKET_H_
#define COMMON_INC_PACKET_H_
#ifdef __cplusplus
extern "C"
{
#endif

#define PACKET_DEV_COUNT 10
#define PACKET_MAX_LENGTH 4096

typedef struct __attribute__((__packed__)) packet_bitfield_s {
	/* Lowest position bits start here */
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
        uint32_t u32;
        uint8_t  u8[4];
        packet_bitfield_t bf;
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
    PACKET_LINK_CONTROL = 15,
} packet_t;

typedef enum {
    PACKET_NOT_RELIABLE=0,
    PACKET_IS_RELIABLE=1<<4,
} packet_reliable_t;

typedef enum {
    PACKET_NOT_CHECKED =0,
    PACKET_IS_CHECKED =1<<5,
} packet_integrity_e;

typedef uint16_t packet_tail_t;

#define PACKET_TAIL_SIZE sizeof(packet_tail_t)
#define PACKET_HEADER_SIZE sizeof(packet_header_t)

em_msg packet_init(void);
dev_handle_t packet_open(packet_t type, packet_reliable_t reliable, packet_integrity_e checked);
em_msg packet_write(dev_handle_t pktHdl, buffer_t *data);
em_msg packet_close(dev_handle_t pktHdl);

void packet_head_print(packet_header_t header);


#ifdef __cplusplus
}
#endif
#endif /* COMMON_INC_PACKET_H_ */
