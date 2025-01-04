/*
 * device.c
 *
 *  Created on: 22.05.2020
 *      Author: badi
 */
#include "common.h"
#include "device.h"
#include <string.h>

const device_t DEVICE_RESET={
    .user_data=NULL,
    .open=NULL,
    .read=NULL,
    .write=NULL,
    .ioctrl=NULL,
    .close=NULL,
    .ready_cb = NULL,
};
static device_t *my_devicesp[DEVICE_CNT];


static uint8_t device_find_dev(const device_t *dev) {
	for (uint8_t i = 0; i < DEVICE_CNT; i++) {
		if (my_devicesp[i] == dev) {
			return i + 1;
		}
	}
	for (uint8_t i = 0; i < DEVICE_CNT; i++) {
		if (my_devicesp[i] == NULL) {
			return i + 1;
		}
	}
	return 0;
};

em_msg device_init(device_t * dev, void * user_data) {
	int8_t dev_nr=0;
	if (dev != NULL) {
		dev_nr = device_find_dev(dev);
		if (dev_nr > 0) {
			my_devicesp[dev_nr] = dev;
			dev->open(dev_nr, user_data);
		}
	} else {
		printf("%010ld: Cannot find device"NL, HAL_GetTick());
	}
	return dev_nr;
}

/*
 * \brief Check if at least the functions indicated by dev_type are set in the structure dev
 * \param dev Device structure holding the access functions
 * \param Bitmap indicating the function which must exist
 */
em_msg device_check(dev_handle_t hdl, dev_func_t dev_type) {
    bool is_ok=false;
    device_t *dev = my_devicesp[hdl];
    if (dev != NULL) {
        is_ok =          ((NULL != dev->open)  || ((dev_type&DEV_OPEN)==0));
        is_ok = is_ok && ((NULL != dev->read)  || ((dev_type&DEV_READ)==0));
        is_ok = is_ok && ((NULL != dev->write) || ((dev_type&DEV_WRITE)==0));
        is_ok = is_ok && ((NULL != dev->ioctrl)|| ((dev_type&DEV_IOCTRL)==0));
        is_ok = is_ok && ((NULL != dev->close) || ((dev_type&DEV_CLOSE)==0));
        is_ok = is_ok && ((NULL != dev->ready_cb) || ((dev_type&DEV_DRCB)==0));
    }
    return is_ok?EM_OK:EM_ERR;
}


em_msg device_reset(dev_handle_t hdl) {
    em_msg res = EM_ERR;
    device_t *dev = my_devicesp[hdl];
    if (dev != NULL) {
        *dev = DEVICE_RESET;
        res = EM_OK;
    }
    return res;
}
/**
 * Write data to the device
 * @param dev device to write to
 * @param buffer data to write
 * @param cnt Count of bytes to write
 * @return EM_ERR ie NULL device given or write function is NULL
 */
em_msg device_write(dev_handle_t hdl, const uint8_t *buffer, uint16_t cnt){
    device_t *dev = my_devicesp[hdl];
    em_msg res = EM_ERR;
    if ((dev != NULL) && (NULL != dev->write)) {
        res = dev->write(hdl, buffer, cnt);
    }
    return res;
}

em_msg device_read(dev_handle_t hdl, uint8_t *buffer, uint16_t *cnt){
    em_msg res = EM_ERR;
    device_t *dev = my_devicesp[hdl];
    if ((dev != NULL) && (NULL != dev->read)) {
        res = dev->read(hdl, buffer, cnt);
    }
    return res;
}



void device_print(dev_handle_t hdl){
    device_t *dev = my_devicesp[hdl];
    if (dev != NULL) {
        printf("open     =  %p"NL, dev->open);
        printf("read     =  %p"NL, dev->read);
        printf("write    =  %p"NL, dev->write);
        printf("ioctrl   =  %p"NL, dev->ioctrl);
        printf("close    =  %p"NL, dev->close);
        printf("udata    =  %p"NL, dev->user_data);
        printf("dev_type =  %d"NL, dev->dev_type);
    }
}



