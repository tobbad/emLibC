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

static const device_t dev_reset={.open=NULL,.read=NULL, .write=NULL, .ioctrl=NULL, .close=NULL};

/*
 * \brief Check if at least the functions indicated by dev_type are set in the structure dev
 * \param dev Device structure holding the access functions
 * \param Bitmap indicateing the function which must exist
 */
elres_t device_check(const device_t * dev, dev_func_t dev_type) {
    bool is_ok=false;
    if (dev != NULL) {
        is_ok =          ((NULL != dev->open)  || ((dev_type&DEV_OPEN)==0));
        is_ok = is_ok && ((NULL != dev->read)  || ((dev_type&DEV_READ)==0));
        is_ok = is_ok && ((NULL != dev->write) || ((dev_type&DEV_WRITE)==0));
        is_ok = is_ok && ((NULL != dev->ioctrl)|| ((dev_type&DEV_IOCTRL)==0));
        is_ok = is_ok && ((NULL != dev->close) || ((dev_type&DEV_CLOSE)==0));
    }
    return is_ok?EMLIB_OK:EMLIB_ERROR;
}


elres_t device_free(device_t * dev) {
	elres_t res = EMLIB_ERROR;
    if (dev != NULL) {
    	*dev = dev_reset;
    	res = EMLIB_OK;
    }
    return res;
}

void device_print(const device_t * dev){
    if (dev != NULL) {
		printf("open  = %p\n",dev->open);
		printf("read  = %p\n",dev->read);
		printf("write = %p\n",dev->write);
		printf("ioctrl= %p\n",dev->ioctrl);
		printf("close = %p\n",dev->close);
    }
}

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

