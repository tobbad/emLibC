/*
 * ringbuffer.c
 *
 *  Created on: Apr 26, 2020
 *      Author: badi
 */

#include "ringbuffer.h"

static uin16_t free_space(rbuf_t *rbuf)
{
	uint16_t count=0;
	if (! rbuf->empty)
	{
		if (rbuf->nxtRdIdx < rbuf->nxtWrIdx)
		{
			count = rbuf->nxtWrIdx-rbuf->nxtRdIdx;
		}
		else if (rbuf->nxtRdIdx > rbuf->nxtWrIdx)
		{
			count = (BDEV_BUFFERSIZE+rbuf->nxtWrIdx)-rbuf->nxtRdIdx;
		}
		else
		{
			count = BDEV_BUFFERSIZE;
		}
	}
	return count;
}
