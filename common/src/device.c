/*
 * device.c
 *
 *  Created on: 22.05.2020
 *      Author: badi
 */
#include "device.h"

static const device_t dev_reset={.open=NULL,.read=NULL, .write=NULL, .ioctrl=NULL, .close=NULL};

/*
 * \brief Check if at least the functions indicated by dev_type are set in the structure dev
 * \param dev Device structure holding the access functions
 * \param Bitmap indicateing the function which must exist
 */
elres_t device_check(const device_t * dev, dev_func_t dev_type) {
    bool is_ok=false;
    if (dev != NULL) {
        is_ok =          ((NULL != dev->open)  || ((dev_type&DEV_OPEN)==0));
        is_ok = is_ok && ((NULL != dev->read)  || ((dev_type&DEV_READ)==0));
        is_ok = is_ok && ((NULL != dev->write) || ((dev_type&DEV_WRITE)==0));
        is_ok = is_ok && ((NULL != dev->ioctrl)|| ((dev_type&DEV_IOCTRL)==0));
        is_ok = is_ok && ((NULL != dev->close) || ((dev_type&DEV_CLOSE)==0));
    }
    return is_ok?EMLIB_OK:EMLIB_ERROR;
}


elres_t device_free(device_t * dev) {
    elres_t res = EMLIB_ERROR;
    if (dev != NULL) {
        *dev = dev_reset;
        res = EMLIB_OK;
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
    }
}



