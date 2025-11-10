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
static sio_t sio;
uint8_t serial_mode = 0;
buf_t rx_buffer;
static time_meas__t _time[MEAS_CNT];
static uint16_t time_idx=0;

sio_res_e serial_init(sio_t *init) {
    sio = *init;
    sio.ready[SIO_RX] = true;
    sio.ready[SIO_TX] = true;
    if (sio.buffer[SIO_RX] != NULL) {
        HAL_UART_Receive_IT(sio.uart, rx_buffer.buffer, 1);
    }
    for (uint8_t i=0;i<MEAS_CNT;i++){
        _time[i].start = -1;
        _time[i].stop = -1;
    }
    CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    return SIO_OK;
}

void serial_addMode(print_e mode) {
    serial_mode |= mode;
}

void serial_removeMode(print_e mode) {
    serial_mode = mode;

}

int _write(int32_t file, uint8_t *ptr, int32_t len) {
    HAL_StatusTypeDef status = HAL_OK;
    if ((sio.buffer_size[SIO_TX] == 0) || (sio.buffer[SIO_TX] == NULL)) {
        if (sio.uart != NULL) {
            sio.ready[SIO_TX] = false;
            sio.bytes_in_buffer[SIO_TX] = len;
            _time[time_idx].start = DWT->CYCCNT;
            //GpioPinToggle(&uart_toggle);
            status = HAL_UART_Transmit(sio.uart, ptr, len, UART_TIMEOUT_MS);
            sio.bytes_in_buffer[SIO_TX] = 0;
            sio.ready[SIO_TX] = true;
        } else {
            errno = EWOULDBLOCK;
            status = HAL_ERROR;
        }
    } else {
        if ((len > sio.buffer_size[SIO_TX]) || (sio.buffer[SIO_TX] == NULL)) {
            errno = EMSGSIZE;
            status = HAL_ERROR;
            return -1;
        } else if (sio.ready[SIO_TX]) {
            while (!ReadModify_write((bool *)&sio.ready[SIO_TX], -1)){}
            _time[time_idx].start = DWT->CYCCNT;
            memcpy(sio.buffer[SIO_TX], ptr, len);
            sio.bytes_in_buffer[SIO_TX] = len;
            //GpioPinToggle(&uart_toggle);
            status = HAL_UART_Transmit_DMA(sio.uart, sio.buffer[SIO_TX], len);
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
         while (!ReadModify_write((bool*)&sio.ready[SIO_TX], -1)){}
        _time[time_idx].stop = DWT->CYCCNT;
        _time[time_idx].duration_us =125*(_time[time_idx].stop-_time[time_idx].start)/10000;
        //GpioPinToggle(&uart_toggle);
        time_idx = (time_idx+1)%MEAS_CNT;
        if (time_idx==0){
            sio.bytes_in_buffer[SIO_TX] = 0;
        }
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
