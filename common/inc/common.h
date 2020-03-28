/*
 * common.h
 *
 *  Created on: 09.12.2015
 *      Author: badi
 */

#ifndef INC_EMLIB_COMMON_H_
#define INC_EMLIB_COMMON_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#ifdef UNIT_TEST
#define STATIC
//#include "hal_defines.h"
#else
#define STATIC static
#endif

typedef enum {
    EMLIB_ERROR = -1,
    EMLIB_OK = 0,
} elres_t;

typedef enum {
    DEV_RESET_DEVICE = 0,
    DEV_SET_CURSOR,
    DEV_LAST,
    DEV_NOCMD = 0x7FFF
} dev_command_t;

#define DEV_HANDLE_NOTDEFINED   -1

typedef int8_t dev_handle;  /**< A value >=0 is OK, otherwise this is invalid */

typedef enum {
	DEV_NONE = 0x00,
	DEV_OPEN = 0x01,
	DEV_READ = 0x02,
	DEV_WRITE= 0x04,
	DEV_IOCTRL=0x08,
	DEV_CLOSE= 0x10,
	DEV_ALL  = 0x1F,
} dev_func_t;


typedef struct device_s {
    dev_handle (*open)(void);
    elres_t (*read)(uint8_t *buffer, uint16_t cnt);
    elres_t (*write)(uint8_t *buffer, uint16_t cnt);
    elres_t (*ioctrl)(dev_command_t cmd, uint16_t value);
    elres_t (*close)(dev_handle hdl);
} device_t;


elres_t device_check(const device_t * dev, dev_func_t dev_type);
elres_t device_free(device_t * dev);
void device_print(const device_t * dev);


#ifdef __cplusplus
}
#endif
#endif /* INC_EMLIB_COMMON_H_ */
