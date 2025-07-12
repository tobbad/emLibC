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
#undef  USE_USB
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
#include "main.h"
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
#include "state.h"
#include "mkeyboard.h"
#include "mutex.h"
#include "_time.h"
#include "buffer.h"

buffer_t *tx_buffer;
buffer_t *rx_buffer;


typedef struct isio_s{
	UART_HandleTypeDef * uart;
	buffer_t *buffer[SIO_RXTX_CNT];
	print_e mode;
	int8_t ready[SIO_RXTX_CNT];
	dev_handle_t devh;
	buffer_t *cstr;
	state_t state;
	bool init;
} isio_t;

static isio_t isio;

void serial_set_mode(print_e mode, bool doReset );

void serial_init(dev_handle_t devh, dev_type_e dev_type, void *dev) {
	sio_t *init = dev;
	isio.uart = init->uart;
	isio.buffer[SIO_RX]= buffer_new(init->buffer[SIO_RX].size);
	isio.buffer[SIO_TX]= buffer_new(init->buffer[SIO_TX].size);
	isio.cstr = buffer_new( RX_BUFFER_SIZE);
	rx_buffer= buffer_new(RX_BUFFER_SIZE);
	tx_buffer = buffer_new(RX_BUFFER_SIZE);
    state_init(&isio.state);
    isio.mode = init->mode|USE_DMA_RX;
    serial_set_mode(init->mode, true);
    HAL_UARTEx_ReceiveToIdle_DMA(isio.uart, rx_buffer->mem, rx_buffer->size);
    isio.init = true;
	return;
}

void serial_set_mode(print_e mode, bool doReset ) {
    isio.mode = mode|USE_DMA_RX;
    time_init();
    if (doReset){
    	time_reset();
    }
    time_set_mode(mode);

}


int _write(int32_t file, uint8_t *ptr, int32_t txLen) {
    uint16_t len=0;
    uint8_t idx=0;
    static uint32_t ltick=0;
    if (!isio.init) return -1;
    uint32_t tick=0;
    if (!isio.init) return-1;
    if ((isio.buffer[SIO_TX]->mem != NULL)) {
         if (isio.mode&TIMESTAMP){
        	 uint32_t tick=HAL_GetTick();
             len = sprintf((char*)isio.buffer[SIO_TX]->mem, "%010ld: ", tick);
          }
         if (isio.mode&GAP_DETECT){
             len += sprintf((char*)&isio.buffer[SIO_TX]->mem[len], " %x ", idx);
             idx =(idx+1)%USE_DMA_TX;
         }
         memcpy(&isio.buffer[SIO_TX]->mem[len], ptr, txLen);
         len+=txLen;
         isio.buffer[SIO_TX]->pl=&isio.buffer[SIO_TX]->mem[len];
         ptr =isio.buffer[SIO_TX]->mem;
     }else{
         if (isio.mode&TIMESTAMP){
        	 tick = HAL_GetTick();
             len = sprintf((char*)&tx_buffer->mem, "%010ld: ", tick);
        	 ltick = tick;
          }
         if (isio.mode&GAP_DETECT){
             len += sprintf((char*)&isio.buffer[SIO_TX]->mem[len], " %x ", idx);
             idx =(idx+1)%USE_DMA_TX;
         }
         memcpy(&tx_buffer->mem[len], ptr, txLen);
         len+=txLen;
         isio.buffer[SIO_TX]->pl = &isio.buffer[SIO_TX]->mem[len];
         ptr =tx_buffer->mem;
     }
	 if (isio.uart != NULL) {
		 if (isio.mode&USE_DMA_TX){
			 while (!ReadModify_write(&isio.buffer[SIO_TX]->ready, -1)){}
			 time_start(len, ptr);
			 isio.buffer[SIO_RX]->ready=false;
			 HAL_UART_Transmit_DMA(isio.uart, (uint8_t*)ptr, len);
			 time_end_su();
		 } else{
			time_start(len, ptr);
			HAL_UART_Transmit(isio.uart, (uint8_t*)ptr, len, UART_TIMEOUT_MS);
			time_end_tx();
			isio.ready[SIO_TX] = true;
		 }
	} else {
		errno = EWOULDBLOCK;
		len  = EM_ERR;
	}
    return len;
}

int16_t _read(int32_t file, uint8_t *ptr, int32_t len) {
    HAL_StatusTypeDef status;
    uint16_t rLen=0;
    if (!isio.init) return -1;
    if (isio.uart != NULL) {
    	if(isio.mode&USE_DMA_RX){
			rLen= strlen((char*)isio.cstr->pl);
			memcpy(ptr, isio.cstr->mem, rLen+1);
			if (rLen>0){
				isio.cstr->pl=isio.cstr->mem;
				printf("Received %s"NL, ptr);
			}
		} else  if (isio.buffer[SIO_RX]->mem == 0) {
            isio.buffer[SIO_RX]->ready = false;
            status = HAL_UART_Receive(isio.uart, isio.buffer[SIO_RX]->mem, len, HAL_MAX_DELAY);
        }
    }

    return rLen;
}

// If Result is negativ -value is the number, which was entered (0...127)
// If the result >0: 1 alpha Higher case char where entered
int16_t serial_scan(dev_handle_t dev){
	if (!isio.init) return -1;
	return _read(0, rx_buffer->mem, LINE_LENGTH);
};
void serial_reset(dev_handle_t dev, bool hard){};

void serial_state(dev_handle_t dev, state_t *ret){
	if (!isio.init) return ;
	uint16_t len= strlen((char*)isio.cstr->pl);
	if (len>0){
		len = MIN(len,CMD_LEN);
		memcpy(isio.state.clabel.str, isio.cstr->pl, len);
		uint8_t ctype = clable2type(&isio.state.clabel);
		if (ctype==ISNUM){
			state_propagate_index(&isio.state, isio.state.clabel.cmd);
		}
	}
};
bool serial_isdirty(dev_handle_t dev){return isio.state.dirty;};
void serial_undirty(dev_handle_t dev){state_undirty(&isio.state);};
kybd_t serial_dev = {
	.init = &serial_init,
	.scan = &serial_scan,
	.reset= &serial_reset,
	.state = &serial_state,
    .isdirty = &serial_isdirty,
    .undirty = &serial_undirty,
	.dev_type = TERMINAL,
	.cnt = 8,
	.first = 1,
};


int8_t serial_waitForNumber(char **key) {
	uint8_t ch = 0xFF;
	bool stay = true;
	int16_t idx = 0;
	while ((ch == 0xff) && (stay)) {
		ch=isio.cstr->pl[0];
		if (ch!=0){
			if (((ch >= '0') && (ch < '9')) || (ch == 'R') || (ch == 'r')
					|| (ch == '+') || (ch == '-')) {
				rx_buffer->mem[idx++] = ch;
				if ((ch == 'R') || (ch == 'r')) {
					stay = false;
				}
				if (idx >= 1) {
					stay = false;
				}
				ch = 0xFF;
			}
		} else{
			HAL_Delay(5);
		}
	}
	char *stopstring = NULL;
	if ((rx_buffer->mem[0] == 'R') || (rx_buffer->mem[0] == 'r')) {
		*key = (char*)&rx_buffer->mem[0];
		return -1;
	}
	long long int res = strtol((char*) &rx_buffer->mem, &stopstring, 10);
	if (strlen(stopstring) == 0) {
		*key = (char*)&rx_buffer->mem[0];
		return res;
	}
	return -1;
}


/**
  * @brief  User implementation of the Reception Event Callback
  *         (Rx event notification called after use of advanced reception service).
  * @param  huart UART handle
  * @param  Size  Number of data available in application reception buffer (indicates a position in
  *               reception buffer until which, data are available)
  * @retval None
  */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size){
	isio.cstr->pl = isio.cstr->mem;
	for (uint16_t i=0;i<size;i++){
		char ch = *rx_buffer->pl;
		*isio.cstr->pl = ch;
		isio.cstr->pl++;
		isio.buffer[SIO_RX]->pl++;
		if (rx_buffer->pl == rx_buffer->mem+rx_buffer->size-1){
			rx_buffer->pl = rx_buffer->mem;
		}
		if (isio.cstr->pl==isio.cstr->mem+isio.cstr->size-1){
			isio.cstr->pl = isio.cstr->mem;
		}
	}
	rx_buffer->pl = rx_buffer->mem;
	*rx_buffer->pl =0;
	*isio.cstr->pl=0;

}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /* Set transmission flag: transfer complete */
  time_end_tx();
  isio.buffer[SIO_RX]->ready=true;
}

