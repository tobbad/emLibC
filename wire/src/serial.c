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
#include "common.h"
#include "hal_port.h"
#include "main.h"
#include "stm32l4xx_hal_conf.h"
#include <assert.h>
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
#ifdef HAL_PCD_MODULE_ENABLED
#ifdef USE_TINY_USB
#include "tusb.h"
#else
#include "usbd_cdc_if.h"
#endif
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
// gi#include "rb_system.h"
#include "_time.h"
#include "common.h"
#include "hal_port.h"
#include "main.h"
#include "mutex.h"
#include "serial.h"
#include "state.h"

static char rx_buf[RX_BUFFER_SIZE];
static char tx_buf[TX_BUFFER_SIZE];
#define POOL_SIZE 10

time_handle_t srxhdl;
time_handle_t stxhdl;
isio_t isio;
static char *new = NULL;
static void serial_apply_change(clabel_u *lbl);

em_msg serial_init(dev_handle_t devh, dev_type_e dev_type, void *dev) {
    if (isio.init)
        return EM_ERR;
    sio_t *init = dev;
    memset(&isio, 0, sizeof(isio_t));
    isio.uart = init->uart;
    isio.devh = dev_type;
    isio.buffer[SIO_RX] = buffer_new_buffer_t(init->buffer[SIO_RX]);
    isio.buffer[SIO_TX] = buffer_new_buffer_t(init->buffer[SIO_TX]);
    state_init(&isio.state);
    // isio.pool =buffer_pool_new(POOL_SIZE, TX_BUFFER_SIZE, LINEAR);
    isio.cbuffer = NULL;
    isio.ser_overflow = 0;
    isio.cTxBytePerSecond = 0;
    isio.mode = init->mode;
    isio.usb_drop_cnt = 0;
    memset(rx_buf, 0, RX_BUFFER_SIZE);
    memset(tx_buf, 0, TX_BUFFER_SIZE);
    isio.init = true;
    serial_mode_set(isio.mode | USE_DMA_RX);
    srxhdl = time_new("srxhdl");
    stxhdl = time_new("stxhdl");
    HAL_UARTEx_ReceiveToIdle_DMA(isio.uart, (uint8_t *)rx_buf, RX_BUFFER_SIZE);
    // we could set the output buffer size to 0:
    // setbuf(stdout, NULL);
    // flush buffer:
    // fflush(stdout);
    return EM_OK;
}

print_e serial_mode_get() {
    if (!isio.init)
        return EM_ERR;
    return isio.mode;
};

void serial_mode_set(print_e mode) {
    if (!isio.init)
        return;
    isio.mode = mode;
}

uint32_t serial_get_byte_per_second() {
    uint32_t ret = isio.cTxBytePerSecond;
    ret = isio.cTxBytePerSecond = 0;
    return ret;
};

volatile int16_t _read(int32_t file, uint8_t *ptr, uint16_t len) {
    uint16_t rLen = EM_ERR;
    if (!isio.init)
        return rLen;
#ifdef HAL_PCD_MODULE_ENABLED
    if (urx_buffer.state == BUFFER_USED) {
        clabel_u *lbl = buffer_get_clabel(&urx_buffer);
        serial_apply_change(lbl);
        buffer_transfer(&urx_buffer, isio.buffer[SIO_RX]);
        time_stop(urxhdl, NULL);
        return isio.buffer[SIO_RX]->used;
    }
#endif
    if (isio.uart != NULL) {
        if (isio.mode & USE_DMA_RX) {
            memcpy(ptr, isio.buffer[SIO_RX]->mem, isio.buffer[SIO_RX]->used);
            rLen = strlen((char *)isio.buffer[SIO_RX]->mem);
        } else if (isio.buffer[SIO_RX]->mem == 0) {
            isio.buffer[SIO_RX]->state = BUFFER_USED;
            time_start(stxhdl, len, isio.buffer[SIO_RX]->mem);
            HAL_UART_Receive(isio.uart, isio.buffer[SIO_RX]->mem, len, HAL_MAX_DELAY);
            time_stop(stxhdl, isio.buffer[SIO_RX]->mem);
        } else {
            rLen = strlen((char *)isio.buffer[SIO_RX]->mem);
        }
    }
    return rLen;
}

int _write(int32_t file, uint8_t *ptr, int32_t txLen) {
    // clang-format off
    if (!isio.init) return EM_ERR;
    // clang-format on
    if (in_interrupt()) {
        // Save ptr, len
        return 0;
    }

    int16_t len = 0;
    static uint8_t gap_idx = 0;
    uint32_t tick = 0;
    if (isio.buffer[SIO_TX]->mem != NULL) {
        buffer_reset(isio.buffer[SIO_TX]);
        if (isio.mode & TIMESTAMP) {
            uint32_t tick = HAL_GetTick();
            len = sprintf((char *)isio.buffer[SIO_TX]->mem, "%010ld: ", tick);
        }
        if (isio.mode & GAP_DETECT) {
            len += sprintf((char *)&isio.buffer[SIO_TX]->mem[len], " %x ", gap_idx);
            gap_idx = (gap_idx + 1) % USE_DMA_TX;
        }
        uint16_t txLenAct = MIN(isio.buffer[SIO_TX]->size - len - TRUCT_NL_LEN, txLen);
        memcpy(&isio.buffer[SIO_TX]->mem[len], ptr, txLenAct);
        len += txLenAct;
        if (txLen != txLenAct) {
            // Trucate line
            len += TRUCT_NL_LEN;
            isio.ser_overflow++;
            memcpy(&isio.buffer[SIO_TX]->mem[isio.buffer[SIO_TX]->size - TRUCT_NL_LEN], &TRUNCT_NL, TRUCT_NL_LEN);
        }
        isio.buffer[SIO_TX]->mem[len] = 0;
        isio.buffer[SIO_TX]->used = len;
        ptr = isio.buffer[SIO_TX]->mem;
    } else {
        if (isio.mode & TIMESTAMP) {
            tick = HAL_GetTick();
            len = sprintf((char *)&tx_buf, "%010ld: ", tick);
        }
        if (isio.mode & GAP_DETECT) {
            gap_idx += sprintf((char *)&tx_buf, " %x ", gap_idx);
            gap_idx = (gap_idx + 1) % USE_DMA_TX;
        }
        uint16_t txLenAct = MIN(TX_BUFFER_SIZE - len - TRUCT_NL_LEN, txLen);
        memcpy(&isio.buffer[SIO_TX]->mem[len], ptr, txLenAct);
        len += txLenAct;
        if (txLen != txLenAct) {
            // Add Line end at last position
            isio.ser_overflow++;
            len += TRUCT_NL_LEN;
            memcpy(&isio.buffer[SIO_TX]->mem[isio.buffer[SIO_TX]->size - TRUCT_NL_LEN], &TRUNCT_NL, TRUCT_NL_LEN);
        }
        ptr = (uint8_t *)tx_buf;
    }
    if (isio.mode & MEASURE_BYTE_PER_SECONDS) {
        time_auto(stxhdl, len, ptr);
        return len;
    }
#ifdef HAL_PCD_MODULE_ENABLED
    if (isio.mode & USE_USB) {
        time_start(utxhdl, len, ptr);
#ifdef USE_TINY_USB
        bool con = tud_cdc_connected();
        if (con) {
            uint8_t written = tud_cdc_write(ptr, len);
            time_stop_su(utxhdl);
            if (written != len) {
                isio.usb_drop_cnt += len;
                time_stop(utxhdl, NULL);
            }
        } else {
            isio.usb_drop_cnt += len;
        }
        tud_cdc_write_flush();
        tud_task();
#else
        CDC_Transmit_FS(ptr, len);
#endif
    }
#endif

    if (isio.uart != NULL) {
        if (isio.mode | (USE_UART | RAW)) {
            time_start(stxhdl, len, ptr);
            HAL_UART_Transmit(isio.uart, ptr, len, UART_TIMEOUT_MS);
            time_stop(stxhdl, NULL);
            isio.ready[SIO_TX] = true;
            return len;
        }
        if (isio.mode & USE_DMA_TX) { // does not work, needs a too large buffer
            if (isio.cbuffer != NULL) {
                printf("UART TX overflow" NL);
                isio.mode ^= USE_DMA_TX;
                return len;
            }
            isio.cbuffer = buffer_pool_get(isio.pool);
            while (!ReadModify_write((int8_t *)&isio.cbuffer->state, 1)) {
            };
            time_start(stxhdl, len, ptr);
            buffer_set(isio.cbuffer, ptr, len);
            HAL_UART_Transmit_DMA(isio.uart, isio.cbuffer->mem, len);
            time_stop_su(stxhdl);
        }
    } else {
        printf("No UART or USB is given" NL);
    }
    return txLen;
}

static em_msg serial_io_open(dev_handle_t devh, void *dev) {
    if (!isio.init)
        return EM_ERR;
    return serial_init(0, 0, dev);
}

static em_msg serial_read(dev_handle_t hdl, uint8_t *buffer, int16_t *cnt) {
    if (!isio.init)
        return EM_ERR;
    if (*cnt < 0)
        return EM_ERR;
    em_msg res = EM_OK;
    int16_t _cnt = MIN(*cnt, isio.buffer[SIO_RX]->used);
    buffer = memcpy(buffer, isio.buffer[SIO_RX]->mem, _cnt);
    *cnt = _cnt;
    return res;
}

static em_msg serial_write(dev_handle_t hdl, const uint8_t *buffer, int16_t cnt) {
    // clang-format off
    if (!isio.init) return EM_ERR;
    // clang-format on
    printf("%s", buffer);
    return EM_OK;
}

device_t serial_io = {
    .open = &serial_io_open,
    .read = &serial_read,
    .write = &serial_write,
    .ioctrl = NULL,
    .close = NULL,
    .ready_cb = NULL,
    .dev_type = DEV_OPEN | DEV_READ | DEV_WRITE,
};

// If the result >0: number of char in buffers
// EM_ERR: Serial was not initialized
static int16_t serial_scan(dev_handle_t dev) {
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
#ifdef HAL_PCD_MODULE_ENABLED
    buffer_clear(&urx_buffer);
#endif
    memset(rx_buf, 0, RX_BUFFER_SIZE);
    memset(tx_buf, 0, TX_BUFFER_SIZE);
};

static void serial_set_state(dev_handle_t dev, const state_t *state) { state_set_state(state, &isio.state); };

static void serial_get_state(dev_handle_t dev, state_t *ret) {
    if (!isio.init)
        return;
    isio.state.first = ret->first;
    isio.state.cnt = ret->cnt;
    memcpy(ret, &isio.state, sizeof(state_t));
};

static void serial_apply_change(clabel_u *lbl) {
    if (!isio.init)
        return;
    uint16_t len = strlen(lbl->str);
    if (len > 0) {
        type_e ctype = clable2type(lbl);
        if (ctype == hexnum) {
            if (!state_get_dirty(&isio.state)) {
                char *stopstring = NULL;
                uint8_t res = strtol(lbl->str, &stopstring, 10);
                if (strlen(stopstring) == 0) {
                    state_propagate_by_idx(&isio.state, res);
                }
                isio.state.clabel.cmd = lbl->cmd;
            }
        }
    }
};

static em_msg serial_diff(dev_handle_t dev, state_t *ref, state_t *diff) {
    if (!isio.init)
        return EM_ERR;
    return state_diff(ref, &isio.state, diff);
};

static em_msg serial_add(dev_handle_t dev, state_t *add) {
    if (!isio.init)
        return EM_ERR;
    return state_add(&isio.state, add);
};

static bool serial_isdirty(dev_handle_t dev) {
    if (!isio.init)
        return EM_ERR;
    return isio.state.dirty;
};

static void serial_undirty(dev_handle_t dev) {
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

int8_t serial_waitForNumber(char **key) {
    if (!isio.init)
        return EM_ERR;
    char *str = NULL;
    if (urx_buffer.state == BUFFER_USED) {
        *key = (char *)&urx_buffer.mem[0];
        return str2uint((char *)&urx_buffer.mem[0]);
    }

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
    if (ctype == hexnum) {
        *key = (char *)&str[0];
        return clabel2uint(&lbl);
    }
    if (ctype == ascii) {
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
        isio.state.clabel.cmd = 0;
        memcpy((uint8_t *)&isio.state.clabel, new, strlen(new));
        serial_apply_change(&isio.state.clabel);
        HAL_UARTEx_ReceiveToIdle_DMA(isio.uart, (uint8_t *)rx_buf, RX_BUFFER_SIZE);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle) {
    /* Set transmission flag: transfer complete */
    isio.cbuffer->state = BUFFER_READY;
    // buffer_pool_return(isio.pool, isio.cbuffer);
    isio.cbuffer = NULL;
    time_stop(stxhdl, NULL);
}
