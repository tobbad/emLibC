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

//#include "main.h"
#include "mutex.h"
#include "serial.h"
#include "_time.h"
static sio_t sio;
static buf_t rx_buffer;
static buf_t tx_buffer;

sio_res_e serial_init(sio_t *init) {
    sio = *init;
    sio.ready[SIO_RX] = true;
    sio.ready[SIO_TX] = true;
    if (sio.buffer[SIO_RX] != NULL) {
        HAL_UART_Receive_IT(sio.uart, (uint8_t*)rx_buffer.buffer, 1);
    }
    time_init();
    time_set_mode(sio.mode);
    return SIO_OK;
}

void serial_set_mode(print_e mode, bool doReset ) {
    sio.mode = mode;
    if (doReset){
    	time_reset();
    }
    time_set_mode(sio.mode);

}

int _write(int32_t file, uint8_t *ptr, int32_t txLen) {
    uint16_t len=0;
    uint8_t idx=0;
    if ((sio.buffer_size[SIO_TX] != 0) && (sio.buffer[SIO_TX] != NULL)) {
        sio.ready[SIO_TX] = false;
         sio.bytes_in_buffer[SIO_TX] = len;
         if (sio.mode&TIMESTAMP){
             len = sprintf(sio.buffer[SIO_TX], "%010ld: ", HAL_GetTick());
          }
         if (sio.mode&GAP_DETECT){
             len += sprintf(&sio.buffer[SIO_TX][len], " %x ", idx);
             idx =(idx+1)%USE_DMA;
         }
         memcpy(&sio.buffer[SIO_TX][len], ptr, txLen);
         len+=txLen;
         ptr =(uint8_t*) sio.buffer[SIO_TX];
     }else{
         if (sio.mode&TIMESTAMP){
             len = sprintf(tx_buffer.buffer, "%010ld: ", HAL_GetTick());
          }
         if (sio.mode&GAP_DETECT){
             len += sprintf(&tx_buffer.buffer[len], " %x ", idx);
             idx =(idx+1)%USE_DMA;
         }
         memcpy(&tx_buffer.buffer[len], ptr, txLen);
         len+=txLen;
         ptr =(uint8_t*)tx_buffer.buffer;
     }
	 if (sio.uart != NULL) {
		 if (sio.mode&USE_DMA){
			 while (!ReadModify_write(&sio.ready[SIO_TX], -1)){}
			 time_start(len);
			 HAL_UART_Transmit_DMA(sio.uart, (uint8_t*)ptr, len);
			 time_end_su();
		 } else{
			time_start(len);
			HAL_UART_Transmit(sio.uart, (uint8_t*)ptr, len, UART_TIMEOUT_MS);
			time_end_tx();
			sio.bytes_in_buffer[SIO_TX] = 0;
			sio.ready[SIO_TX] = true;
		 }
	} else {
		errno = EWOULDBLOCK;
		len  = EM_ERR;
	}
    return len;
}

int _read(int32_t file, uint8_t *ptr, int32_t len) {
    HAL_StatusTypeDef status;
    sio.bytes_in_buffer[SIO_RX] = -1;

    if (sio.uart != NULL) {
        if ((sio.buffer_size[SIO_RX] != 0) || (sio.buffer[SIO_RX] != NULL)) {
            sio.ready[SIO_RX] = false;
            status = HAL_UART_Receive(sio.uart, ptr, len, HAL_MAX_DELAY);
            if (status == HAL_OK) {
                sio.bytes_in_buffer[SIO_RX] = len;
            } else if ((status == HAL_ERROR) || (status == HAL_TIMEOUT)) {
                sio.bytes_in_buffer[SIO_RX] = -1;
            } else  { //HAL_BUSY
                //Not ready
                sio.bytes_in_buffer[SIO_RX] = 0;
            }
        } else {// Receive text by IT HAL_UART_Receive_IT
            // Buffer with size larger than 0 is provided
        }
    }
    return sio.bytes_in_buffer[SIO_RX];
}

int __io_getchar(void) {
    uint8_t ch = 0;
    // Clear the Overrun flag just before receiving the first character
    __HAL_UART_CLEAR_OREFLAG(sio.uart);

    HAL_UART_Receive(sio.uart, (uint8_t*) &ch, 1, 0xFFFF);
    //HAL_UART_Transmit(sio.uart), (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

int __io_putchar(char ch) {
    // Clear the Overrun flag just before receiving the first character
    __HAL_UART_CLEAR_OREFLAG(sio.uart);

    HAL_UART_Transmit(sio.uart, (uint8_t*) &ch, 1, 0xFFFF);
    return ch;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == sio.uart) {
        /* Set transmission flag: transfer complete */
         while (!ReadModify_write(&sio.ready[SIO_TX], -1)){}
         time_end_tx();
        //GpioPinToggle(&uart_toggle);
        sio.bytes_in_buffer[SIO_TX] = 0;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == sio.uart) {
        /* Set transmission flag: transfer complete */
        sio.ready[SIO_RX] = true;
        sio.bytes_in_buffer[SIO_RX] = 1;
    }
}
