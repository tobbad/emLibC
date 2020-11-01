/*
 * device.c
 *
 *  Created on: 22.05.2020
 *      Author: badi
 */
#include "device.h"
#include <string.h>
const device_t DEVICE_RESET={
    .open=NULL,
    .read=NULL,
    .write=NULL,
    .ioctrl=NULL,
    .close=NULL,
    .ready_cb = NULL,
    .user_data=NULL,
};

/*
 * \brief Check if at least the functions indicated by dev_type are set in the structure dev
 * \param dev Device structure holding the access functions
 * \param Bitmap indicating the function which must exist
 */
elres_t device_check(const device_t * dev, dev_func_t dev_type) {
    bool is_ok=false;
    if (dev != NULL) {
        is_ok =          ((NULL != dev->open)  || ((dev_type&DEV_OPEN)==0));
        is_ok = is_ok && ((NULL != dev->read)  || ((dev_type&DEV_READ)==0));
        is_ok = is_ok && ((NULL != dev->write) || ((dev_type&DEV_WRITE)==0));
        is_ok = is_ok && ((NULL != dev->ioctrl)|| ((dev_type&DEV_IOCTRL)==0));
        is_ok = is_ok && ((NULL != dev->close) || ((dev_type&DEV_CLOSE)==0));
        is_ok = is_ok && ((NULL != dev->ready_cb) || ((dev_type&DEV_DRCB)==0));
    }
    return is_ok?EMLIB_OK:EMLIB_ERROR;
}

elres_t device_reset(device_t * dev) {
    elres_t res = EMLIB_ERROR;
    if (dev != NULL) {
        *dev = DEVICE_RESET;
        res = EMLIB_OK;
    }
    return res;
}
/**
 * Write data to the device
 * @param dev device to write to
 * @param buffer data to write
 * @param cnt Count of bytes to write
 * @return EMLIB_ERROR ie NULL device given or write function is NULL
 */
elres_t device_write(device_t * dev, const uint8_t *buffer, uint16_t cnt){
    elres_t res = EMLIB_ERROR;
    if ((dev != NULL) && (NULL != dev->write)) {
        res = dev->write(dev->user_data, buffer, cnt);
    }
    return res;
}

elres_t device_read(device_t * dev, uint8_t *buffer, uint16_t cnt){
    elres_t res = EMLIB_ERROR;
    if ((dev != NULL) && (NULL != dev->read)) {
        res = dev->read(dev->user_data, buffer, cnt);
    }
    return res;
}



void device_print(const device_t * dev){
    if (dev != NULL) {
        printf("open  = %p\n",dev->open);
        printf("read  = %p\n",dev->read);
        printf("write = %p\n",dev->write);
        printf("ioctrl= %p\n",dev->ioctrl);
        printf("close = %p\n",dev->close);
        printf("udata = %p\n",dev->user_data);
    }
}



