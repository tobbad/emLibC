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
#include "usbd_cdc_if.h"
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
#include "common.h"
#include "serial.h"
#include "state.h"
#include "keyboard.h"
#include "mutex.h"
#include "_time.h"
#include "buffer.h"
#include "stm32l4xx_ll_usart.h"

static char rx_buf[RX_BUFFER_SIZE];
static char tx_buf[TX_BUFFER_SIZE];

typedef struct isio_s{
	UART_HandleTypeDef * uart;
	PCD_HandleTypeDef *pcd;
	buffer_t *buffer[SIO_RXTX_CNT];
	print_e mode;
	int8_t ready[SIO_RXTX_CNT];
	dev_handle_t devh;
	state_t state;
	bool init;
} isio_t;

static isio_t isio;
static char  *new = NULL;

void serial_set_mode(print_e mode, bool doReset );

void serial_init(dev_handle_t devh, dev_type_e dev_type, void *dev) {
	sio_t *init = dev;
	isio.uart = init->uart;
	isio.pcd = init->pcd;
	isio.buffer[SIO_RX]= buffer_new(init->buffer[SIO_RX].size);
	isio.buffer[SIO_TX]= buffer_new(init->buffer[SIO_TX].size);
    state_init(&isio.state);
    isio.mode = init->mode|USE_DMA_RX;
    memset(rx_buf, 0, RX_BUFFER_SIZE);
    memset(tx_buf, 0, TX_BUFFER_SIZE);
    serial_set_mode(init->mode, true);
    HAL_UARTEx_ReceiveToIdle_DMA(isio.uart, (uint8_t*)rx_buf, RX_BUFFER_SIZE);
    isio.init = true;
	// we could set the output buffer size to 0:
	// setbuf(stdout, NULL);
	// flush buffer:
	// fflush(stdout);
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
    if ((isio.buffer[SIO_TX]->mem != NULL)) {
         if (isio.mode&TIMESTAMP){
        	 uint32_t tick=HAL_GetTick();
             len = sprintf((char*)isio.buffer[SIO_TX]->mem, "%010ld: ", tick);
          }
         if (isio.mode&GAP_DETECT){
             len += sprintf((char*)&isio.buffer[SIO_TX]->mem[len], " %x ", idx);
             idx =(idx+1)%USE_DMA_TX;
         }
         txLen = MIN(TX_BUFFER_SIZE-len-1, txLen);
         memcpy(&isio.buffer[SIO_TX]->mem[len], ptr, txLen);
         len+=txLen;
         isio.buffer[SIO_TX]->mem[len]=0;
         ptr =isio.buffer[SIO_TX]->mem;
     }else{
         if (isio.mode&TIMESTAMP){
        	 tick = HAL_GetTick();
             len = sprintf((char*)&tx_buf, "%010ld: ", tick);
        	 ltick = tick;
          }
         if (isio.mode&GAP_DETECT){
             len += sprintf((char*)&tx_buf, " %x ", idx);
             idx =(idx+1)%USE_DMA_TX;
         }
         memcpy(&tx_buf, ptr, txLen);
         len+=txLen;
         ptr =(uint8_t*)tx_buf;
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
			HAL_UART_Transmit(isio.uart, ptr, len, UART_TIMEOUT_MS);
			time_end_tx();
			isio.ready[SIO_TX] = true;
		 }
	 } else if (isio.pcd!=NULL){
		 CDC_Transmit_FS(ptr, len);
	 } else{
		 printf("No Uart or USB is given"NL);
	 }
    return len;
}

int16_t _read(int32_t file, uint8_t *ptr, int32_t len) {
    uint16_t rLen=0;
    if (!isio.init) return -1;
    if (isio.uart != NULL) {
    	if(isio.mode&USE_DMA_RX){
			rLen= strlen(isio.buffer[SIO_RX]->mem);
			if ((rLen>0)&&(rLen<CMD_LEN)){
				memcpy(ptr, isio.state.clabel.str, rLen);
			} else{
				isio.state.clabel.cmd =ZERO4;
			}
		} else  if (isio.buffer[SIO_RX]->mem == 0) {
            isio.buffer[SIO_RX]->ready = false;
            HAL_UART_Receive(isio.uart, isio.buffer[SIO_RX]->mem, len, HAL_MAX_DELAY);
        }
    }

    return rLen;
}

// If Result is negativ -value is the number, which was entered (0...127)
// If the result >0: 1 alpha Higher case char where entered
int16_t serial_scan(dev_handle_t dev){
	if (!isio.init) return -1;
	return _read(0, isio.buffer[SIO_RX]->mem, LINE_LENGTH);
};
void serial_reset(dev_handle_t dev, bool hard){
	memset(rx_buf, 0, LINE_LENGTH);
};

void serial_state(dev_handle_t dev, state_t *ret){
	if (!isio.init) return ;
	uint16_t len= strlen(isio.state.clabel.str);
	if (len>0){
		uint8_t ctype = clable2type(isio.state.clabel.str);
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
	char *str=NULL;
	clabel_u lbl ={.cmd=0};
	uint8_t len=0;
	uint8_t i;
	if (new!=NULL){
		str=new;
		len = strlen(str);
		len = MIN(len, CMD_LEN);
		for ( i=0;i<len;i++){
			lbl.str[i]=new[i];
		}
		lbl.str[i]=0;
		new=NULL;
	}
	if (strlen(str)==0)return -1;
	uint8_t ctype = clable2type(&lbl);
	if  (ctype==ISNUM) {
		*key = (char*)&str[0];
		return lbl.cmd;
	}
	if (ctype == ISASCISTR) {
		*key = (char*)&str[0];
		return -1;
	}
	return -1;
}


/**
  * @brief  User implementation of the Reception Event Callback pRxBuffPtr
  *         (Rx event notification called after use of advanced reception service).
  * @param  huart UART handle
  * @param  Size  Number of data available in application reception buffer (indicates a position in
  *               reception buffer until which, data are available)
  * @retval None
  */
void  HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size){
	if (huart->Instance == USART2){
		uint8_t idx=0;
		memset(isio.buffer[SIO_RX]->mem, 0, isio.buffer[SIO_RX]->size);
		isio.buffer[SIO_RX]->pl = isio.buffer[SIO_RX]->mem;
		for (uint8_t i=0;i<size;i++){
			char ch = rx_buf[idx++];
			*isio.buffer[SIO_RX]->pl =ch;
			isio.buffer[SIO_RX]->pl++;
			if (idx==RX_BUFFER_SIZE){
				assert("too long");
			}
			if (isio.buffer[SIO_RX]->pl ==isio.buffer[SIO_RX]->mem+isio.buffer[SIO_RX]->size-1){
				isio.buffer[SIO_RX]->pl = isio.buffer[SIO_RX]->mem;
			}
		}
		*isio.buffer[SIO_RX]->pl =0; // End string
		new = (char*)isio.buffer[SIO_RX]->mem;
		HAL_UARTEx_ReceiveToIdle_DMA(isio.uart,(uint8_t*) rx_buf, RX_BUFFER_SIZE);
	}
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /* Set transmission flag: transfer complete */
  time_end_tx();
  isio.buffer[SIO_RX]->ready=true;
}

