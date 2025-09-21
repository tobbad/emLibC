/*************************************************************************
 *
 *    W a v e L a b
 *
 *    Engineering AG
 *
 *    Project:          Radio Bell
 *
 *    Programmer:		Tobias Badertscher
 *
 *    Date:				xx.yy.yyyy
 *
 *    File name   :
 *    Description :
 *
 *
 *
 **************************************************************************/

#ifndef __LI2C_H
#define __LI2C_H
#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"
#define I2C_HANDLLE_CNT 1
#define MAX_TRANSFER_SIZE 10
typedef struct _i2c_t {
  I2C_HandleTypeDef *i2c;
  uint8_t adr;
} i2c_t;
typedef uint8_t i2c_handle;

i2c_handle li2c_init(I2C_HandleTypeDef *i2c_dev, uint8_t i2cAdr);
em_msg i2c_write(i2c_handle i2c_h, uint8_t cnt, uint8_t *txbuffer);
em_msg i2c_read(i2c_handle i2c_h, uint8_t cnt, uint8_t *rxbuffer);

#endif /* __LI2C_H */
