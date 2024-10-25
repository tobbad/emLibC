/*
 * ringbuffer.c
 *
 *  Created on: Apr 26, 2020
 *      Author: badi
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"    
#include "device.h"
    
#include <string.h>

#include "ringbuffer.h"

const rbuf_t rbuf_clear = {
    .empty = true,
    .nxtRdIdx = 0,
    .nxtWrIdx = 0,
    .buffer = NULL,
    .buffer_size = 0,
};

static rbuf_t* rbuf_reg[RBUF_REGISTERS] = {0};

static rbuf_t* get_rbuf(rbuf_hdl_t hdl)
{
    if ((hdl<0) || (hdl>=RBUF_REGISTERS))
    {
        return NULL;
    }
    return rbuf_reg[hdl];
}

void rbuf_init(void)
{
    for (uint8_t i = 0;i<RBUF_REGISTERS;i++)
    {
        rbuf_reg[i] = NULL;
    }
}

rbuf_hdl_t rbuf_register(rbuf_t *rbuf)
{
    if (NULL == rbuf)
    {
        return -1;
    }
    if ( (NULL == rbuf->buffer) || (0 == rbuf->buffer_size))
    {
        return -1;
    }
    for (uint8_t i = 0;i<RBUF_REGISTERS;i++)
    {
        if ((NULL == rbuf_reg[i]) || (rbuf_reg[i] == rbuf))
        {
            rbuf_reg[i] = rbuf;
            return i;
        } 
    }
    return -1;
}

rbuf_hdl_t rbuf_deregister(rbuf_hdl_t hdl)
{
    rbuf_t *rbuf = get_rbuf(hdl);
    if (rbuf == NULL)
    {
        return hdl;
    }
    rbuf_reg[hdl] = NULL;
    return -1;
}

uint16_t rbuf_free(rbuf_hdl_t hdl)
{
	uint16_t count=0;
    rbuf_t *rbuf = get_rbuf(hdl);
    if (rbuf == NULL)
    {
        return count;
    }
    count = rbuf->buffer_size;
    if (!rbuf->empty)
    { 
        if (rbuf->nxtRdIdx < rbuf->nxtWrIdx)
        {
            count = (rbuf->nxtRdIdx+rbuf->buffer_size)-rbuf->nxtWrIdx;
        }
        else if (rbuf->nxtRdIdx == rbuf->nxtWrIdx)
        {
            count = 0;
        }
        else if (rbuf->nxtRdIdx > rbuf->nxtWrIdx)
        {
            count = rbuf->nxtRdIdx-rbuf->nxtWrIdx;
        }
    }
	return count;
}


elres_t rbuf_write_byte(rbuf_hdl_t hdl, uint8_t byte)
{
    elres_t res = EMLIB_ERROR;
    rbuf_t *rbuf = get_rbuf(hdl);
    if (rbuf == NULL)
    {
        return res;
    }
    if ((rbuf->empty) || (rbuf->nxtWrIdx != rbuf->nxtRdIdx))
    {
        rbuf->buffer[rbuf->nxtWrIdx] = byte;
        rbuf->nxtWrIdx = (rbuf->nxtWrIdx+1) % rbuf->buffer_size;
        /* ToDo protect by semaphore */
        rbuf->empty = false;
        res = EMLIB_OK;
    }
    return res;
}
elres_t rbuf_write_bytes(rbuf_hdl_t hdl, const uint8_t* bytes, uint16_t count){
    elres_t res = EMLIB_ERROR;
    rbuf_t *rbuf = get_rbuf(hdl);
    uint16_t free;
    if (rbuf == NULL)
    {
        return res;
    }
    free = rbuf_free(hdl);
    if (free>=count)
    {
        res = EMLIB_OK;
        for (uint16_t i = 0;i<count && (res == EMLIB_OK);i++)
        {
            res = rbuf_write_byte(hdl, bytes[i]);
        }
    }
    return res;
}


elres_t rbuf_read_byte(rbuf_hdl_t hdl, uint8_t *byte)
{
    elres_t res = EMLIB_ERROR;
    rbuf_t *rbuf = get_rbuf(hdl);
    if (rbuf == NULL)
    {
        return res;
    }
    if (!rbuf->empty)
    {
        *byte = rbuf->buffer[rbuf->nxtRdIdx];
        rbuf->nxtRdIdx = (rbuf->nxtRdIdx+1) % rbuf->buffer_size;
        if (rbuf->nxtRdIdx == rbuf->nxtWrIdx)
        {
            rbuf->empty = true;
        }
        res = EMLIB_OK;
    }

    return res;
}

elres_t rbuf_read_bytes(rbuf_hdl_t hdl, uint8_t *bytes, uint16_t *count){
    elres_t res = EMLIB_ERROR;
    rbuf_t *rbuf = get_rbuf(hdl);
    uint16_t available;
    if (rbuf == NULL)
    {
        return res;
    }
    available = rbuf->buffer_size-rbuf_free(hdl);
    if (!rbuf->empty)
    {
        *count = *count<available?*count:available;
        res = EMLIB_OK;
        for (uint16_t i = 0;i<*count && (res == EMLIB_OK);i++)
        {
            res = rbuf_read_byte(hdl, &bytes[i]);
        }
    }
    return res;
}

elres_t rbuf_get_device(rbuf_hdl_t hdl, device_t *device, dev_func_t dev_type) {
    elres_t res = EMLIB_ERROR;
    if (NULL==device) {
        if (DEV_NONE == dev_type) {
        		return res;
        }
    } else {
       res = EMLIB_OK;
       memset(device, 0, sizeof(device_t));
       if (dev_type & DEV_READ){
           device->read = rbuf_read_bytes;
       }
       if (dev_type & DEV_WRITE){
           device->write = rbuf_write_bytes;
       }
       device->user_data = (void*)&hdl;
    }
    return res;
}

#ifdef __cplusplus
}
#endif
