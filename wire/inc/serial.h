/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Tobias Badertscher
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * serial_io.h
 *
 *  Created on: 12.05.2018
 *      Author: badi
 */

#ifndef INC_SERIAL_IO_H_
#define INC_SERIAL_IO_H_
#include "common.h"
#define UART_TIMEOUT_MS 100
typedef enum {SIO_ERROR=-1, SIO_OK=0, } sio_res_e;
typedef enum {SIO_RX=0, SIO_TX, SIO_RXTX_CNT} sio_channel_e;
typedef enum {
	RAW = 0,
	TIMESTAMP = 1,
	GAP_DETECT = 2,
    ONE_SHOT = 4,
    USE_DMA = 8,
} print_e;


typedef struct _sio_t{
	UART_HandleTypeDef * uart;
	uint8_t	 ready[SIO_RXTX_CNT];	/* Internal use only */
	uint16_t buffer_size[SIO_RXTX_CNT];
	int16_t  bytes_in_buffer[SIO_RXTX_CNT];
	int16_t  size[SIO_RXTX_CNT];
	char *buffer[SIO_RXTX_CNT];
	print_e mode;
} sio_t;

typedef struct buf_s{
	char buffer[LINE_LENGTH];
	bool isValid;
} buf_t;

sio_res_e serial_init(sio_t *init);
void  serial_set_mode(print_e mode, bool doReset);
int	logf_debug(const char *__restrict, ...) _ATTRIBUTE ((__format__ (__printf__, 1, 2)));
#endif /* INC_SERIAL_IO_H_ */
