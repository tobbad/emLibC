/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Tobias Badertscher
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * serial_io.c
 *
 * To use this printf support you  should configure the UARt with
 * DMA support and IRQ on DMA and UART should be allowed.
 *
 *  Created on: 12.05.2018
 *      Author: badi
 */
#undef USE_USB
#include "main.h"
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include "stm32l4xx_hal_conf.h"
#ifdef HAL_PCD_MODULE_ENABLED
#include "usbd_cdc_if.h"
#endif
#if defined(STM32F303xC)
#include "stm32f3xx.h"
#elif defined(STM32F407xx) || defined(STM32F401xE)
#include "stm32f4xx.h"
#elif defined(STM32H755xx) || defined(STM32H743xx)
#include "stm32h7xx.h"
#elif defined(STM32L476xx) || defined(STM32L432xx) || defined(STM32L412xx) || defined(STM32L475xx)
#include "stm32l4xx.h"
#else
#error Undefined platform
#endif
#include "_time.h"
#include "buffer.h"
#include "common.h"
#include "hal_port.h"
#include "main.h"
#include "mutex.h"
#include "serial.h"
#include "state.h"

static char rx_buf[RX_BUFFER_SIZE];
static char tx_buf[TX_BUFFER_SIZE];


typedef struct isio_s {
    UART_HandleTypeDef *uart;
    buffer_t       *buffer[SIO_RXTX_CNT];
    print_e        mode;
    int8_t         ready[SIO_RXTX_CNT];
    dev_handle_t   devh;
    state_t        state;
    bool           init;
} isio_t;

time_handle_t  shdl;
static isio_t isio;
static char *new = NULL;

void serial_set_mode(print_e mode, bool doReset);

em_msg serial_init(dev_handle_t devh, dev_type_e dev_type, void *dev) {
    if (isio.init) return EM_ERR;
    sio_t *init = dev;
    memset(&isio, 0, sizeof(isio_t));
    isio.uart = init->uart;
    isio.devh = dev_type;
    isio.buffer[SIO_RX] = buffer_new(init->buffer[SIO_RX]->size);
    isio.buffer[SIO_TX] = buffer_new(init->buffer[SIO_TX]->size);
    state_init(&isio.state);
    isio.mode = init->mode | USE_DMA_TX;
    memset(rx_buf, 0, RX_BUFFER_SIZE);
    memset(tx_buf, 0, TX_BUFFER_SIZE);
    serial_set_mode(init->mode, true);
    HAL_UARTEx_ReceiveToIdle_DMA(isio.uart, (uint8_t *)rx_buf, RX_BUFFER_SIZE);
    shdl = time_new();
    isio.init = true;
    // we could set the output buffer size to 0:
    // setbuf(stdout, NULL);
    // flush buffer:
    // fflush(stdout);
    return EM_OK;
}

em_msg serial_io_open(dev_handle_t devh, void *dev) {
    if (!isio.init)
        return EM_ERR;
    return serial_init(0, 0, dev);
}

void serial_set_mode(print_e mode, bool doReset) {
    isio.mode = mode | USE_DMA_RX;
    if (doReset) {
        time_reset(shdl);
    }
    time_set_mode(shdl, mode);
}

em_msg serial_write(dev_handle_t hdl, const uint8_t *buffer, int16_t cnt) {
    // clang-format off
    if (!isio.init) return EM_ERR;
    // clang-format on
    printf("%s", buffer);
    return EM_OK;
}

int _write(int32_t file, uint8_t *ptr, int32_t txLen) {
    // clang-format off
    if (!isio.init) return EM_ERR;
    // clang-format on
    uint16_t len = 0;
    uint8_t idx = 0;
    txLen = MIN(txLen, TX_BUFFER_SIZE - 3);
    // clang-format off
    if (!isio.init) return -1;
    // clang-format on
    uint32_t tick = 0;
    if (isio.buffer[SIO_TX]->mem != NULL) {
        if (isio.mode & TIMESTAMP) {
            uint32_t tick = HAL_GetTick();
            len = sprintf((char *)isio.buffer[SIO_TX]->mem, "%010ld: ", tick);
        }
        if (isio.mode & GAP_DETECT) {
            len += sprintf((char *)&isio.buffer[SIO_TX]->mem[len], " %x ", idx);
            idx = (idx + 1) % USE_DMA_TX;
        }
        memcpy(&isio.buffer[SIO_TX]->mem[len], ptr, txLen);
        len += txLen;
        isio.buffer[SIO_TX]->mem[len] = 0;
        isio.buffer[SIO_TX]->used = len;
        ptr = isio.buffer[SIO_TX]->mem;
    } else {
        if (isio.mode & TIMESTAMP) {
            tick = HAL_GetTick();
            len = sprintf((char *)&tx_buf, "%010ld: ", tick);
        }
        if (isio.mode & GAP_DETECT) {
            len += sprintf((char *)&tx_buf, " %x ", idx);
            idx = (idx + 1) % USE_DMA_TX;
        }
        memcpy(&tx_buf[len], ptr, txLen);
        len += txLen;
        ptr = (uint8_t *)tx_buf;
    }
    if (isio.uart != NULL) {
        if (isio.mode & USE_DMA_TX) {
            while (!ReadModify_write((int8_t *)&isio.buffer[SIO_TX]->state, 1)) {
            };
            time_start(shdl, len, ptr);
            isio.buffer[SIO_RX]->state = BUFFER_USED;
            HAL_UART_Transmit_DMA(isio.uart, (uint8_t *)ptr, len);
            time_end_su(shdl);
        } else {
            time_start(shdl, len, ptr);
            HAL_UART_Transmit(isio.uart, ptr, len, UART_TIMEOUT_MS);
            time_end_tx(shdl);
            isio.ready[SIO_TX] = true;
#ifdef HAL_PCD_MODULE_ENABLED
            if (isio.mode & USE_USB) {
                CDC_Transmit_FS(ptr, len);
            }
#endif
        }
    } else {
        printf("No UART or USB is given" NL);
    }
    return len;
}

em_msg serial_read(dev_handle_t hdl, uint8_t *buffer, int16_t *cnt) {
    if (!isio.init)
        return EM_ERR;
    em_msg res = EM_OK;
    int16_t _cnt = *cnt;
    int16_t rLen = fread(buffer, (size_t)1, (size_t)_cnt, stdin);
    if (rLen < 0) {
        res = EM_ERR;
    }
    return res;
}
int16_t _read(int32_t file, uint8_t *ptr, uint16_t len) {
    uint16_t rLen;
    if (!isio.init)
        return EM_ERR;
#ifdef HAL_PCD_MODULE_ENABLED
    if (urx_buffer.state == BUFFER_READY) {
        uint16_t msize = MIN(len, urx_buffer.size);
        buffer_get(&urx_buffer, ptr, &msize);
        if (msize != len) {
            printf("Only transfer %d of %d" NL, msize, len);
        }
        return msize;
    }
#endif
    if (isio.uart != NULL) {
        if (isio.mode & USE_DMA_RX) {
            rLen = strlen((char *)isio.buffer[SIO_RX]->mem);
        } else if (isio.buffer[SIO_RX]->mem == 0) {
            isio.buffer[SIO_RX]->state = BUFFER_USED;
            HAL_UART_Receive(isio.uart, isio.buffer[SIO_RX]->mem, len, HAL_MAX_DELAY);
        }
    }
    return rLen;
}

// If the result >0: number of char in buffers
// EM_ERR: Serial was not initialized
int16_t serial_scan(dev_handle_t dev) {
    if (!isio.init)
        return EM_ERR;
    return _read(0, isio.buffer[SIO_RX]->mem, RX_BUFFER_SIZE);
};

void serial_reset(dev_handle_t dev) {
    buffer_t *buf = isio.buffer[SIO_RX];
    buffer_reset(buf);
    buf = isio.buffer[SIO_TX];
    buffer_reset(buf);
    state_reset(&isio.state);
    memset(rx_buf, 0, RX_BUFFER_SIZE);
    memset(tx_buf, 0, TX_BUFFER_SIZE);
};

void serial_set_state(dev_handle_t dev, const state_t *state) { state_set_state(state, &isio.state); };

void serial_apply_change(void) {
    if (!isio.init)
        return;
    uint16_t len = strlen(isio.state.clabel.str);
    for (uint8_t i = 0; i < CMD_LEN; i++) {
        if (isalpha((int)isio.state.clabel.str[i])) {
            isio.state.clabel.str[i] &= 0xdf;
        }
    }
    if (len > 0) {
        uint8_t ctype = clable2type(&isio.state.clabel);
        if (ctype == ISNUM) {
            if (!state_get_dirty(&isio.state)) {
                uint8_t nr = clabel2uint8(&isio.state.clabel);
                state_propagate_by_idx(&isio.state, nr);
            }
        }
    }
};

void serial_get_state(dev_handle_t dev, state_t *ret) {
    if (!isio.init)
        return;
    isio.state.first = ret->first;
    isio.state.cnt = ret->cnt;
    memcpy(ret, &isio.state, sizeof(state_t));
};

em_msg serial_diff(dev_handle_t dev, state_t *ref, state_t *diff) {
    if (!isio.init)
        return EM_ERR;
    return state_diff(ref, &isio.state, diff);
};

em_msg serial_add(dev_handle_t dev, state_t *add) {
    if (!isio.init)
        return EM_ERR;
    return state_add(&isio.state, add);
};

bool serial_isdirty(dev_handle_t dev) {
    if (!isio.init)
        return EM_ERR;
    return isio.state.dirty;
};

void serial_undirty(dev_handle_t dev) {
    if (!isio.init)
        return;
    state_set_undirty(&isio.state);
};

kybd_t serial_dev = {
    .init = &serial_init,
    .scan = &serial_scan,
    .reset = &serial_reset,
    .set_state = &serial_set_state,
    .state = &serial_get_state,
    .diff = &serial_diff,
    .add = &serial_add,
    .isdirty = &serial_isdirty,
    .undirty = &serial_undirty,
    .dev_type = TERMINAL,
    .cnt = 8,
    .first = 1,
};

device_t serial_io = {
    .open = &serial_io_open,
    .read = &serial_read,
    .write = &serial_write,
    .ioctrl = NULL,
    .close = NULL,
    .ready_cb = NULL,
    .dev_type = DEV_OPEN | DEV_READ | DEV_WRITE,
};

int8_t serial_waitForNumber(char **key) {
    if (!isio.init)
        return EM_ERR;
    char *str = NULL;
    clabel_u lbl = {.cmd = 0};
    uint8_t len = 0;
    uint8_t i;
    if (new != NULL) {
        str = new;
        len = strlen(str);
        len = MIN(len, CMD_LEN);
        for (i = 0; i < len; i++) {
            lbl.str[i] = new[i];
        }
        lbl.str[i] = 0;
        new = NULL;
    }
    if (strlen(str) == 0)
        return -1;
    uint8_t ctype = clable2type(&lbl);
    if (ctype == ISNUM) {
        *key = (char *)&str[0];
        return clabel2uint8(&lbl);
    }
    if (ctype == ISASCISTR) {
        *key = (char *)&str[0];
        return -1;
    }
    return -1;
}

/**
 * @brief  User implementation of the Reception Event Callback pRxBuffPtr
 *         (Rx event notification called after use of advanced reception
 * service).
 * @param  huart UART handle
 * @param  Size  Number of data available in application reception buffer
 * (indicates a position in reception buffer until which, data are available)
 * @retval None
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    if (huart == isio.uart) {
        uint8_t idx = 0;
        memset(isio.buffer[SIO_RX]->mem, 0, isio.buffer[SIO_RX]->size);
        isio.buffer[SIO_RX]->pl = isio.buffer[SIO_RX]->mem;
        for (uint8_t i = 0; i < size; i++) {
            char ch = rx_buf[idx++];
            *isio.buffer[SIO_RX]->pl = ch;
            isio.buffer[SIO_RX]->pl++;
            if (idx == RX_BUFFER_SIZE) {
                assert("too long");
            }
            if (isio.buffer[SIO_RX]->pl == isio.buffer[SIO_RX]->mem + isio.buffer[SIO_RX]->size - 1) {
                isio.buffer[SIO_RX]->pl = isio.buffer[SIO_RX]->mem;
            }
        }
        *isio.buffer[SIO_RX]->pl = 0; // End string
        isio.buffer[SIO_RX]->state = BUFFER_USED;
        new = (char *)isio.buffer[SIO_RX]->mem;
        memset((uint8_t *)&isio.state.clabel, 0, CMD_LEN);
        memcpy((uint8_t *)&isio.state.clabel, new, strlen(new));
        serial_apply_change();
        HAL_UARTEx_ReceiveToIdle_DMA(isio.uart, (uint8_t *)rx_buf, RX_BUFFER_SIZE);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle) {
    /* Set transmission flag: transfer complete */
    time_end_tx(shdl);
    memset(isio.buffer[SIO_RX]->mem, 0, isio.buffer[SIO_RX]->size);
    isio.buffer[SIO_RX]->state = BUFFER_READY;
#ifdef HAL_PCD_MODULE_ENABLED
    // USBD_CDC_ReceivePacket(&hUsbDeviceFS);
#endif
}
