/*
 * device.h
 *
 *  Created on: 22.05.2020
 *      Author: badi
 */

#ifndef COMMON_INC_DEVICE_H_
#define COMMON_INC_DEVICE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"
typedef enum { XSCAN, EIGHTKEY, TERMINAL, DEV_TYPE_NA } dev_type_e;

#include "common.h"
#define DEV_HANDLE_NOTDEFINED -1
#define DEVICE_CNT 5
typedef int8_t
    dev_handle_t; /**< A value >=0 is OK, otherwise this is invalid */

typedef enum {
  DEV_CMD_RESET_DEVICE = 0,
  DEV_CMD_LAST,
  DEV_NOCMD = 0x7FFF
} dev_command_t;

typedef enum {
  DEV_READY_FOR_WRITE = 1 << 0,
  DEV_READY_FOR_READ = 1 << 1,
} device_state_t;

typedef struct device_s {
  void *user_data;
  em_msg (*open)(dev_handle_t hdl, void *user_data);
  em_msg (*read)(dev_handle_t hdl, uint8_t *buffer, int16_t *cnt);
  em_msg (*write)(dev_handle_t hdl, const uint8_t *buffer, int16_t cnt);
  em_msg (*ioctrl)(dev_handle_t hdl, dev_command_t cmd, int16_t value);
  em_msg (*close)(dev_handle_t hdl);
  em_msg (*ready_cb)(device_state_t state);
  uint8_t dev_type;
} device_t;

extern const device_t DEVICE_RESET;

typedef enum {
  DEV_NONE = 0x00,
  DEV_OPEN = 0x01,
  DEV_READ = 0x02,
  DEV_LREAD = 0x04,
  DEV_WRITE = 0x04,
  DEV_LWRITE = 0x08,
  DEV_IOCTRL = 0x10,
  DEV_CLOSE = 0x20,
  DEV_DRCB = 0x40, /* Data ready callback */
  DEV_ALL = DEV_DRCB << 1,
} dev_func_t;

dev_handle_t device_init(device_t *dev, void *user_data);
em_msg device_check(dev_handle_t hdl, dev_func_t dev_type);
em_msg device_read(dev_handle_t hdl, uint8_t *buffer, int16_t *cnt);
em_msg device_write(dev_handle_t hdl, const uint8_t *buffer, int16_t cnt);
em_msg device_reset(dev_handle_t hdl);
void device_print(dev_handle_t hdl);

#ifdef __cplusplus
}
#endif
#endif /* COMMON_INC_DEVICE_H_ */
