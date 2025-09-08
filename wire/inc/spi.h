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

#ifndef __SPI_H
#define __SPI_H
#ifdef __cplusplus
extern "C" {
#endif

#include "_gpio.h"
#define MAX_SPI_TRANSFER_SIZE 4
#define SPI_TIMEOUT 10
typedef struct _spi_t
{
	void *  spi;
	uint8_t cpol;
	uint8_t cpha;
	gpio_pin_t *sel_pin;
} spi_t;

typedef enum _spi_dir
{
	WRITE = 0,
	READ = 1,
	CMD = 2,
} spi_dir;
spi_t * spi_init(void * spi_dev, spi_t *spi, gpio_pin_t *sel_pin, uint8_t cpol, uint8_t cpha);
em_msg spi_readWrite(spi_t *spi_dev, int16_t adr, spi_dir dir, uint8_t cnt, uint8_t *);

em_msg spi_writeCmd(spi_t *spi_dev, uint8_t cmd);


#ifdef __cplusplus
extern "C" }
#endif

#endif /* __SPI_H */
