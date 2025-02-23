/*
 * common.c
 *
 *  Created on: 24.03.2017
 *      Author: badi
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include "common.h"

uint16_t to_hex(char *out, uint16_t out_size, uint8_t *buffer, uint16_t buffer_size, bool write_asci)
{
    // hex data (without addr/asci) contains in maximum:
    // 2*16(databytes)+15(normal spaces)+1 (extraspace byte  7->8)
    // = 48 + '\0' = 56 characters
    static const char SP78[] = "  ";
    static const char INTERCHARSP[] = " ";
    static const uint8_t addr_inc = 16;
    char ascii[17];
    uint8_t abuf_idx = 0;
    uint16_t out_idx=0;
    uint16_t ava_size=out_size;
    int wr_size;
    for (uint32_t addr=0;addr<buffer_size;addr+=addr_inc)
    {
        wr_size = snprintf(&out[out_idx], ava_size, "0x%04X ",(unsigned int) addr);
        out_idx += wr_size;
        ava_size -= wr_size;
        /*
         * Write data
         */
        uint16_t end_addr = addr+addr_inc;
        abuf_idx = 0;
        ascii[abuf_idx] = '\0';
        for (uint32_t laddr=addr; laddr<end_addr; laddr+=1)
        {
            if (laddr < buffer_size)
            {
                wr_size = snprintf(&out[out_idx], ava_size, "%02X%s", buffer[laddr], INTERCHARSP);
                wr_size = MIN(wr_size, ava_size);
                out_idx += wr_size;
                ava_size -= wr_size;
                if (isprint(buffer[laddr])>0)
                {
                    abuf_idx += sprintf(&ascii[abuf_idx], "%c", buffer[laddr]);
                }
                else
                {
                    abuf_idx += sprintf(&ascii[abuf_idx], ".");
                }
            } else {
                wr_size = snprintf(&out[out_idx], ava_size, "  %s",INTERCHARSP);
                wr_size = MIN(wr_size, ava_size);
                out_idx += wr_size;
                ava_size -= wr_size;
                abuf_idx += sprintf(&ascii[abuf_idx], " ");
            }
            if (laddr%8 == 7)
            {
                wr_size = snprintf(&out[out_idx], ava_size, SP78);
                wr_size = MIN(wr_size, ava_size);
                out_idx += wr_size;
                ava_size -= wr_size;
            }
        }
        if (write_asci)
        {
            ascii[abuf_idx] = '\0';
            wr_size = snprintf(&out[out_idx], ava_size, "%s", ascii);
            wr_size = MIN(wr_size, ava_size);
            out_idx += wr_size;
            ava_size -= wr_size;
        }
        wr_size = snprintf(&out[out_idx], ava_size, "\n");
        wr_size = MIN(wr_size, ava_size);
        out_idx += wr_size;
        ava_size -= wr_size;
    }

    return out_idx;
}

/*
//                                      16   12   5
// this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
// This works out to be 0x1021, but the way the algorithm works
// lets us use 0x8408 (the reverse of the bit pattern).  The high
// bit is always assumed to be set, thus we only use 16 bits to
// represent the 17 bit value.
 * From
 * http://www8.cs.umu.se/~isak/snippets/crc-16.c
 *
 * In python crcmod this is the predefined x-25 crc Calculation:
 * Name     Polynomial  Reversed?   Init-value  XOR-out     Check
 * x-25     0x11021     True        0x0000      0xFFFF      0x906E
*/
uint16_t common_crc16(uint8_t *data_p, uint16_t length)
{
    const uint16_t CRC_POLY = 0x8408;
    uint8_t i;
    uint32_t data;
    uint32_t crc = 0xffff;

    if (length == 0) {
        return (~crc);
    }

    do {
        for (i=0, data=(unsigned int)0xff & *data_p++;
             i < 8;
             i++, data >>= 1) {
              if ((crc & 0x0001) ^ (data & 0x0001)) {
                    crc = (crc >> 1) ^ CRC_POLY;
              } else {
                  crc >>= 1;
              }
        }
    } while (--length);

    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xff);

    return (crc);
}

void PrintBuffer(uint8_t *buffer, uint8_t size, char *header) {
    if (header!=NULL){
        printf("Print %s msg size %d;"NL, header, size);
    }
    printf("Head"NL);
    for (uint8_t i = 0; i < size; i++) {
        printf(" %2d = 0x%02x"NL, i, buffer[i]);
    }
}
