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
 *    Date:				5.10.2023
 *
 *    File name   :
 *    Description :
 *                      used for:
 *                      -
 *                      -
 *                      -
 *
 **************************************************************************/
#include "common.h"
#include "i2c.h"
#include "hal_port.h"
static i2c_t my_dev[I2C_HANDLLE_CNT+1];

i2c_handle li2c_init(I2C_HandleTypeDef * i2c_dev, uint8_t i2cAdr)
{
	i2c_handle handle  = 1;
	while (my_dev[handle].i2c != NULL){
		handle++;
	}
	my_dev[handle].i2c = i2c_dev;
	my_dev[handle].adr = i2cAdr;
	return handle;
}



em_msg i2c_write(i2c_handle i2c_h, uint8_t cnt, uint8_t *txbuffer)
{
	if ((i2c_h > 0) && (my_dev[i2c_h].i2c != NULL) && (cnt <MAX_TRANSFER_SIZE))
	{
		uint8_t buffer[MAX_TRANSFER_SIZE];
		uint16_t adr = my_dev[i2c_h].adr|0x01;
		HAL_I2C_Master_Transmit(my_dev[i2c_h].i2c, adr, txbuffer, cnt, 100);
		memcpy(buffer, txbuffer, cnt);
	} else {
		printf("Invalid write"NL);
		return EM_ERR;
	}
	return EM_OK;
}

em_msg i2c_read(i2c_handle i2c_h, uint8_t cnt, uint8_t *rxbuffer)
{
	if ((i2c_h > 0) && (my_dev[i2c_h].i2c = NULL) && (cnt <MAX_TRANSFER_SIZE))
	{
		uint16_t adr =  my_dev[i2c_h].adr&0xFE;
		UNUSED(adr);
	} else {
		printf("Invalid read"NL);
		return EM_ERR;
	}

	return EM_OK;
}
