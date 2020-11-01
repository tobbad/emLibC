/*
 * packet.c
 *
 *  Created on: 30.10.2020
 *      Author: badi
 */
#include <string.h>
#include "common.h"
#include "device.h"
#include "packet.h"


typedef struct packet_desc_t_ {
    device_t * out_dev;
    packet_header_t header;
    packet_tail_t tail;
} packet_desc_t;

static packet_desc_t pkt_dev[PACKET_DEV_COUNT];

static bool packet_hdl_check(dev_handle_t pktHdl) {
    if ( (DEV_HANDLE_NOTDEFINED!=pktHdl)
         && (pktHdl<PACKET_DEV_COUNT)
         && (NULL!=pkt_dev[pktHdl].out_dev)) {
        return true;
    } else {
        return false;
    }
}


elres_t packet_init(void) {
    for (uint8_t dev=0;dev<PACKET_DEV_COUNT; dev++)
    {
        pkt_dev[dev].out_dev = NULL;
    }
    return EMLIB_OK;
}



dev_handle_t packet_open(device_t* outDev, packet_t type, packet_reliable_t reliable, packet_integrity_e checked) {
    dev_handle_t ret_dev = DEV_HANDLE_NOTDEFINED;

    bool dev_ok = device_check(outDev, DEV_WRITE)==EMLIB_OK?true:false;

    if ( dev_ok && (type>PACKET_ACK) && (type<PACKET_LINK_CONTROL) ) {
        for (uint8_t dev=0;dev<PACKET_DEV_COUNT; dev++)
        {
            if (NULL == pkt_dev[dev].out_dev){
                pkt_dev[dev].out_dev = outDev;
                pkt_dev[dev].header.bf.type = type;
                pkt_dev[dev].header.bf.is_reliable = (reliable!=0);
                pkt_dev[dev].header.bf.is_checked = (checked!=0);
                ret_dev = dev;
                break;
            }
        }
    }

    return ret_dev;
}

elres_t packet_write(dev_handle_t pktHdl, buffer_t *data) {
    elres_t res = EMLIB_ERROR;

    if (packet_hdl_check(pktHdl)) {
        if ((NULL != data->mem) && (NULL != data->pl)) {
            uint16_t pl_offset = data->pl - data->mem;
            printf("Payload offset is %d/used = %d\n", pl_offset, data->used);
            if (pl_offset >= PACKET_HEADER_SIZE) {
                if (data->used <= PACKET_MAX_LENGTH) {
                    int16_t chk_sum=0;
                    uint16_t trSize = data->used + PACKET_HEADER_SIZE;
                    uint8_t * pktHead = &data->pl[-PACKET_HEADER_SIZE];
                    pkt_dev[pktHdl].header.bf.length = data->used;
                    res = EMLIB_OK;
                    /*
                     * Calculate header checksum
                     */
                    for (uint8_t i=0;i<3;i++) {
                        chk_sum = (chk_sum + pkt_dev[pktHdl].header.u8[i]) % 256;
                    }
                    chk_sum = 255-chk_sum;
                    pkt_dev[pktHdl].header.bf.chk_sum = chk_sum;
                    /*
                     * Calculate crc of payload if required
                     */
                    if (pkt_dev[pktHdl].header.bf.is_checked) {
                        if (trSize+2<=data->size) {
                            /* We have spare palace for CRC */
                            uint16_t crc16 = common_crc16(&data->pl[0], data->used);
                            trSize += 2;
                            memcpy(&data->pl[data->used], &crc16, sizeof(crc16));
                        } else {
                            res = EMLIB_ERROR;
                            printf("No space for trailing CRC (used+head= %d, size = %d)\n", trSize, data->size);
                        }
                    }
                    /*
                     * Set Header
                     */
                    if (EMLIB_OK == res) {
                        memcpy(pktHead, &pkt_dev[pktHdl].header.u32, PACKET_HEADER_SIZE);
                        device_write(pkt_dev[pktHdl].out_dev, pktHead, trSize);
                    } else {
                        printf("Do NOT write to output\n");
                    }
                } else {
                    printf("Given packet to large\n");
                }
            } else {
                printf("No place to introduce head\n");
            }
        } else {
            printf("Mem or payload is NULL!\n");
        }
    } else {
        printf("Packet check is not OK\n");
    }

    return res;
}

elres_t packet_close(dev_handle_t pktHdl) {
    elres_t res = EMLIB_ERROR;

    if (packet_hdl_check(pktHdl)) {
        pkt_dev[pktHdl].out_dev = NULL;
        res = EMLIB_OK;
    }

    return res;
}

void packet_head_print(packet_header_t header){
    printf("seq_nr      : %d\n", header.bf.seq_nr     );
    printf("ack_nr      : %d\n", header.bf.ack_nr     );
    printf("is_checked  : %d\n", header.bf.is_checked );
    printf("is_reliable : %d\n", header.bf.is_reliable);
    printf("type        : %d\n", header.bf.type       );
    printf("length      : %d\n", header.bf.length     );
    printf("chk_sum     : %d\n", header.bf.chk_sum    );
}

