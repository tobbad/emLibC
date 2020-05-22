/*
 * device.h
 *
 *  Created on: 22.05.2020
 *      Author: badi
 */

#ifndef COMMON_INC_DEVICE_H_
#define COMMON_INC_DEVICE_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include "common.h"

#define DEV_HANDLE_NOTDEFINED   -1

typedef int8_t dev_handle;  /**< A value >=0 is OK, otherwise this is invalid */

typedef enum {
    DEV_RESET_DEVICE = 0,
    DEV_SET_CURSOR,
    DEV_LAST,
    DEV_NOCMD = 0x7FFF
} dev_command_t;

typedef struct device_s {
    void * user_data;
    dev_handle (*open)(void * user_data);
    elres_t (*read)(void * user_data, uint8_t *buffer, uint16_t cnt);
    elres_t (*write)(void * user_data, const uint8_t *buffer, uint16_t cnt);
    elres_t (*ioctrl)(void * user_data, dev_command_t cmd, uint16_t value);
    elres_t (*close)(void * user_data, dev_handle hdl);
} device_t;

extern const device_t DEVICE_RESET;

typedef enum {
    DEV_NONE = 0x00,
    DEV_OPEN = 0x01,
    DEV_READ = 0x02,
    DEV_WRITE= 0x04,
    DEV_IOCTRL=0x08,
    DEV_CLOSE= 0x10,
    DEV_ALL  = 0x1F,
} dev_func_t;


elres_t device_check(const device_t * dev, dev_func_t dev_type);
elres_t device_write(device_t * dev, const uint8_t *buffer, uint16_t cnt);
elres_t device_reset(device_t * dev);
void device_print(const device_t * dev);


#ifdef __cplusplus
}
#endif
#endif /* COMMON_INC_DEVICE_H_ */
