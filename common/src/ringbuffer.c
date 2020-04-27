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

uint16_t rbuf_size(rbuf_hdl_t hdl)
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
            count = rbuf->nxtWrIdx-rbuf->nxtRdIdx;
        }
        else if (rbuf->nxtRdIdx > rbuf->nxtWrIdx)
        {
            count = (rbuf->buffer_size+rbuf->nxtWrIdx)-rbuf->nxtRdIdx;
        }
    }
	return count;
}


elres_t rbuf_write_byte(rbuf_hdl_t hdl, uint8_t byte)
{
    rbuf_t *rbuf = get_rbuf(hdl);
    if (rbuf == NULL)
    {
        return EMLIB_ERROR;
    }
    
   
    return EMLIB_OK;
}

#ifdef __cplusplus
}
#endif
