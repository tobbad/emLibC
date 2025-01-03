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
 * serial_io.c
 *
 * To use this printf support you  should configure the UARt with
 * DMA support and IRQ on DMA and UART should be allowed.
 *
 *  Created on: 12.05.2018
 *      Author: badi
 */
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#if defined(STM32F303xC)
#include "stm32f3xx.h"
#elif defined(STM32F407xx) ||defined(STM32F401xE)
#include "stm32f4xx.h"
#elif defined(STM32H755xx) || defined(STM32H743xx)
#include "stm32h7xx.h"
#elif defined(STM32L476xx) || defined(STM32L432xx) || defined(STM32L412xx) || defined(STM32L475xx)
#include "stm32l4xx.h"
#else
#error Undefined platform
#endif
#include "serial.h"


static sio_t sio;
uint8_t serial_mode=0;

sio_res_e serial_init(sio_t* init)
{
	sio = *init;
	sio.ready[SIO_RX] = true;
	sio.ready[SIO_TX] = true;
	if ( (sio.buffer_size[SIO_RX]>0) || (sio.buffer_size[SIO_TX]>0))
	{
	    // Buffered Output is not supported
	    return SIO_ERROR;
	}

	return SIO_OK;
}

void serial_addMode(print_e mode){
	serial_mode |=mode;
}

void serial_removeMode(print_e mode){
	serial_mode =mode;

}

int _write(int32_t file, uint8_t *ptr, int32_t len)
{
	HAL_StatusTypeDef status;
	if ((sio.buffer_size[SIO_TX]==0) || (sio.buffer[SIO_TX]==NULL))
	{
		if (sio.uart != NULL)
		{
			sio.ready[SIO_TX] = false;
			sio.bytes_in_buffer[SIO_TX]=len;
			status = HAL_UART_Transmit(sio.uart, ptr, len, UART_TIMEOUT_MS);
			sio.bytes_in_buffer[SIO_TX]=0;
			sio.ready[SIO_TX] = true;
		}
		else
		{
			errno = EWOULDBLOCK;
			status = HAL_ERROR;
		}
	}
	else
	{
		if ( (len > sio.buffer_size[SIO_TX]) || (sio.buffer[SIO_TX] == NULL))
		{
			errno = EMSGSIZE;
			status = HAL_ERROR;
		}
		else if (sio.ready[SIO_TX])
		{
			errno = EWOULDBLOCK;
			status = HAL_ERROR;
		}
		else
		{
			memcpy(sio.buffer[SIO_TX], ptr, len);
			sio.ready[SIO_TX] = false;
			sio.bytes_in_buffer[SIO_TX] = len;
			status = HAL_UART_Transmit_DMA(sio.uart, sio.buffer[SIO_TX], len);
		}
	}
	return (status==HAL_OK)?len:-1;
}


int _read(int32_t file, uint8_t *ptr, int32_t len)
{
	HAL_StatusTypeDef status;
	sio.bytes_in_buffer[SIO_RX]=-1;

	if ((sio.buffer_size[SIO_RX]==0) || (sio.buffer[SIO_RX]==NULL))
	{
		if (sio.uart != NULL)
		{
			char buf[]= "abcdefgh";
			sio.ready[SIO_RX] = false;
			if (1==1)
			{
				status = HAL_UART_Receive(sio.uart, ptr, len, HAL_MAX_DELAY);
				if (status == HAL_OK)	{
					sio.bytes_in_buffer[SIO_RX] = len;
				}
				else if ((status == HAL_ERROR) || (status == HAL_TIMEOUT))
				{
					sio.bytes_in_buffer[SIO_RX] = -1;
				}
				else /* Not ready */
				{
					sio.bytes_in_buffer[SIO_RX] = 0;
				}
			}
			else
			{
				static uint8_t i=0;
				if (i%2==0)
				{
					memcpy(ptr, buf, strlen(buf));
					sio.bytes_in_buffer[SIO_RX] = (int32_t)strlen(buf);
				}
				else
				{
					sio.bytes_in_buffer[SIO_RX] = 0;
				}
				++i;
			}
			sio.ready[SIO_RX] = true;
		}
		else
		{
			errno = EWOULDBLOCK;
			sio.bytes_in_buffer[SIO_RX]=-1;
		}
	}
	return sio.bytes_in_buffer[SIO_RX];
}

int __io_getchar(void)
{
    uint8_t ch = 0;
    // Clear the Overrun flag just before receiving the first character
    __HAL_UART_CLEAR_OREFLAG(sio.uart);

    HAL_UART_Receive(sio.uart, (uint8_t *)&ch, 1, 0xFFFF);
    //HAL_UART_Transmit(sio.uart), (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}
int __io_putchar(char ch)
{
    // Clear the Overrun flag just before receiving the first character
    __HAL_UART_CLEAR_OREFLAG(sio.uart);

    HAL_UART_Transmit(sio.uart, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

