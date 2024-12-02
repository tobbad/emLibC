/*
 * terminal.h
 *
 *  Created on: Jun 10, 2024
 *      Author: TBA
 */

#ifndef TERMINAL_H
#define TERMINAL_H
#include "main.h"
extern kybd_t terminal_dev;

int8_t terminal_waitForKey(char **key);

#endif /* TERMINAL_H_ */
