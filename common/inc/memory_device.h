/*
 * memory_device.h
 *
 *  Created on: 30.10.2020
 *      Author: badi
 */

#ifndef COMMON_TEST_MEMORY_DEVICE_H_
#define COMMON_TEST_MEMORY_DEVICE_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"
#include "device.h"

#define RCV_BUF_SIZE 4096
extern uint8_t rcv_buffer[];

extern buffer_t memory_device_buffer;
extern device_t memory_device;

elres_t memory_device_reset(buffer_t *buf);
elres_t memory_device_print(buffer_t *buf);
#ifdef __cplusplus
}
#endif

#endif /* COMMON_TEST_MEMORY_DEVICE_H_ */
