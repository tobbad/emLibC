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
        wr_size = snprintf(&out[out_idx], ava_size, "0x%04X ", addr);
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
                out_idx += wr_size;
                ava_size -= wr_size;
                abuf_idx += sprintf(&ascii[abuf_idx], " ");
            }
            if (laddr%8 == 7)
            {
                wr_size = snprintf(&out[out_idx], ava_size, SP78);
                out_idx += wr_size;
                ava_size -= wr_size;
            }
        }
        if (write_asci)
        {
            wr_size = snprintf(&out[out_idx], ava_size, "%s", ascii);
            out_idx += wr_size;
            ava_size -= wr_size;
        }
        wr_size = snprintf(&out[out_idx], ava_size, "\n");
        out_idx += wr_size;
        ava_size -= wr_size;
    }

    return out_idx;
}

