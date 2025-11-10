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

#include "mutex.h"
#include "serial.h"
#define MEAS_CNT 10
typedef struct time_meas_s{
    uint32_t start;
    uint32_t stop;
    uint32_t duration_us;
}time_meas__t;
static mSio_t mSio;
uint8_t serial_mode = 0;
buf_t rx_buffer;
static time_meas__t _time[MEAS_CNT];
static uint16_t time_idx=0;

sio_res_e mserial_init(mSio_t *init) {
    mSio = *init;
    mSio.ready[SIO_RX] = true;
    mSio.ready[SIO_tX] = true;
    if (mSio.buffer[SIO_RX] != NULL) {
        HAL_UART_Receive_IT(mSio.uart, rx_buffer.buffer, 1);
    }
    for (uint8_t i=0;i<MEAS_CNT;i++){
        _time[i].start = -1;
        _time[i].stop = -1;
    }
    CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    return SIO_OK;
}

void mserial_addMode(print_e mode) {
    serial_mode |= mode;
}

void mserial_removeMode(print_e mode) {
    serial_mode = mode;

}

int _write(int32_t file, uint8_t *ptr, int32_t len) {
    HAL_StatusTypeDef status = HAL_OK;
    if ((mSio.buffer_size[SIO_TX] == 0) || (mSio.buffer[SIO_TX] == NULL)) {
        if (mSio.uart != NULL) {
            mSio.ready[mSio_tX] = false;
            mSio.bytes_in_buffer[SIO_TX] = len;
            _time[time_idx].start = DWT->CYCCNT;
            //GpioPinToggle(&uart_toggle);
            status = HAL_UART_Transmit(mSio.uart, ptr, len, UART_TIMEOUT_MS);
            mSio.bytes_in_buffer[SIO_TX] = 0;
            mSio.ready[SIO_TX] = true;
        } else {
            errno = EWOULDBLOCK;
            status = HAL_ERROR;
        }
    } else {
        if ((len > mSio.buffer_size[SIO_TX]) || (mSio.buffer[SIO_TX] == NULL)) {
            errno = EMSGSIZE;
            status = HAL_ERROR;
            return -1;
        } else if (mSio.ready[SIO_TX]) {
            while (!ReadModify_write((bool *)&mSio.ready[mSio_tX], -1)){}
            _time[time_idx].start = DWT->CYCCNT;
            memcpy(mSio.buffer[SIO_TX], ptr, len);
            mSio.bytes_in_buffer[SIO_TX] = len;
            //GpioPinToggle(&uart_toggle);
            status = HAL_UART_Transmit_DMA(mSio.uart, mSio.buffer[mSio_tX], len);
        } else {
            errno = EWOULDBLOCK;
            status = HAL_ERROR;
            return -1;
        }
    }
    return len;
}

int _read(int32_t file, uint8_t *ptr, int32_t len) {
    HAL_StatusTypeDef status;
    mSio.bytes_in_buffer[SIO_RX] = -1;

    if (mSio.uart != NULL) {
        if ((mSio.buffer_size[SIO_RX] != 0) || (mSio.buffer[SIO_RX] != NULL)) {
            mSio.ready[SIO_RX] = false;
            status = HAL_UART_Receive(mSio.uart, ptr, len, HAL_MAX_DELAY);
            if (status == HAL_OK) {
                mSio.bytes_in_buffer[SIO_RX] = len;
            } else if ((status == HAL_ERROR) || (status == HAL_TIMEOUT)) {
                mSio.bytes_in_buffer[SIO_RX] = -1;
            } else  { //HAL_BUSY
                //Not ready
                mSio.bytes_in_buffer[SIO_RX] = 0;
            }
        } else {// Receive text by IT HAL_UART_Receive_IT
            // Buffer with size larger than 0 is provided

        }
    }
    return mSio.bytes_in_buffer[SIO_RX];
}

int __io_getchar(void) {
    uint8_t ch = 0;
    // Clear the Overrun flag just before receiving the first character
    __HAL_UART_CLEAR_OREFLAG(mSio.uart);

    HAL_UART_Receive(mSio.uart, (uint8_t*) &ch, 1, 0xFFFF);
    //HAL_UART_Transmit(mSio.uart), (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

int __io_putchar(char ch) {
    // Clear the Overrun flag just before receiving the first character
    __HAL_UART_CLEAR_OREFLAG(mSio.uart);

    HAL_UART_Transmit(mSio.uart, (uint8_t*) &ch, 1, 0xFFFF);
    return ch;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == mSio.uart) {
        /* Set transmision flag: transfer complete */
         while (!ReadModify_write((bool*)&mSio.ready[mSio_tX], -1)){}
        _time[time_idx].stop = DWT->CYCCNT;
        _time[time_idx].duration_us =125*(_time[time_idx].stop-_time[time_idx].start)/10000;
        //GpioPinToggle(&uart_toggle);
        time_idx = (time_idx+1)%MEAS_CNT;
        if (time_idx==0){
            mSio.bytes_in_buffer[mSio_tX] = 0;
        }
        mSio.bytes_in_buffer[mSio_tX] = 0;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == mSio.uart) {
        /* Set transmission flag: transfer complete */
        mSio.ready[SIO_RX] = true;
        mSio.bytes_in_buffer[SIO_RX] = 1;
    }
}
