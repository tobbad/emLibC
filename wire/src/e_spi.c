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
#include "main.h"
#include "common.h"
#include "gpio.h"
#include "e_spi.h"
static spi_t myDev[DEV_CNT];

spi_t* spi_init(void * spi_dev, spi_t *spi, GpioPin_t *sel_pin, uint8_t cpol, uint8_t cpha){
	return &myDev[0];
}

elres_t spi_readWrite(spi_t *spi_dev, int16_t adr, spi_dir dir, uint8_t cnt, uint8_t *){
	return EMLIB_OK;

}

elres_t spi_writeCmd(spi_t *spi_dev, uint8_t cmd){
	return EMLIB_OK;

}
