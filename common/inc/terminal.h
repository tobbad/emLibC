/*
 * terminal.h
 *
 *  Created on: Jun 10, 2024
 *      Author: TBA
 */

#ifndef TERMINAL_H
#define TERMINAL_H
#include "keyboard.h"
extern kybd_t terminal_dev;

int8_t terminal_waitForNumber(char **key);

#endif /* TERMINAL_H_ */
