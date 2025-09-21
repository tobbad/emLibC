/*
 * keyboard.h
 *
 *  Created on: Oct 30, 2024
 *      Author: badi
 */
#ifndef __KEYBOARD_H_
#define __KEYBOARD_H_

#include "common.h"
#include "device.h"
#include "state.h"

#define SETTLE_TIME_MS 1
#define SCAN_MS 5

extern char *key_state_3c[];
extern char *key_state_2c[];

typedef struct kybd_s {
  em_msg (*init)(dev_handle_t dev, dev_type_e dev_type, void *device);
  int16_t (*scan)(dev_handle_t dev);
  void (*reset)(dev_handle_t dev);
  void (*state)(dev_handle_t dev, state_t *ret);
  em_msg (*add)(dev_handle_t dev, state_t *add);
  em_msg (*diff)(dev_handle_t dev, state_t *ref, state_t *diff);
  bool (*isdirty)(dev_handle_t dev);
  void (*undirty)(dev_handle_t dev);
  dev_type_e dev_type;
  uint8_t cnt;
  uint8_t first;
  int32_t pcnt;
  int32_t plcnt;
} kybd_t;

dev_handle_t keyboard_init(kybd_t *kybd, void *device);
// If Result is negativ -value is the number, which was entered (0...127)
// If the result >0: 1 alpha Higher case char where entered
int16_t keyboard_scan(dev_handle_t dev);
void keyboard_reset(dev_handle_t dev);
void keyboard_state(dev_handle_t dev, state_t *ret);
void keyboard_add(dev_handle_t dev, state_t *add);
em_msg keyboard_diff(dev_handle_t dev, state_t *ret, state_t *diff);
bool keyboard_isdirty(dev_handle_t dev);
void keyboard_undirty(dev_handle_t dev);
void keyboard_print(state_t *state, char *start); // Show returned

#endif /* __KEYBOARD_H_ */
