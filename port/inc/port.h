/*
 * port.h
 *
 *  Created on: 28.12.2019
 *      Author: badi
 */

#ifndef LIB_PORT_PORT_H_
#define LIB_PORT_PORT_H_

#include "pin.h"

typedef enum {
	PORT_NA=0,
	PORTA,
	PORTB,
	PORTC,
	PORTD,
	PORTE,
	PORTF,
	PORTG,
	PORTH,
	PORTI,
	PORT_CNT,
} port_t;

#endif /* LIB_PORT_PORT_H_ */
