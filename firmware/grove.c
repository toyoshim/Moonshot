// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "grove.h"

#include <string.h>

#include "controller.h"
#include "serial.h"

enum {
  IDLE,
  REG,
  VALUE,
};

static uint8_t phase = IDLE;
static uint8_t reg = 0;

// Reg. | Value
// -----+-----------------
// 0x00 | P1 DIGITAL 0
// 0x01 | P1 DIGITAL 1
// 0x02 | P1 ANALOG 0 HIGH
// 0x03 | P1 ANALOG 0 LOW
// 0x04 | P1 ANALOG 1 HIGH
// 0x05 | P1 ANALOG 1 LOW
// 0x06 | P1 ANALOG 2 HIGH
// 0x07 | P1 ANALOG 2 LOW
// 0x08 | P1 ANALOG 3 HIGH
// 0x09 | P1 ANALOG 3 LOW
// 0x0a | P1 ANALOG 4 HIGH
// 0x0b | P1 ANALOG 4 LOW
// 0x0c | P1 ANALOG 5 HIGH
// 0x0d | P1 ANALOG 5 LOW

static bool start(uint8_t address, uint8_t dir) {
  dir;
  if (address != 0x58) {
    return false;
  }
  phase = REG;
  return true;
}

static bool write(uint8_t data) {
  if (phase == REG) {
    reg = data;
    phase = VALUE;
    return true;
  }
  return false;
}

static bool read(uint8_t* data) {
  if (phase == IDLE) {
    return false;
  }
  switch (reg) {
    case 0x00:
      *data = controller_data(0, 0, 0);
      break;
    case 0x01:
      *data = controller_data(0, 1, 0);
      break;
    case 0x02:
      *data = controller_analog(0) >> 8;
      break;
    case 0x03:
      *data = controller_analog(0);
      break;
    case 0x04:
      *data = controller_analog(1) >> 8;
      break;
    case 0x05:
      *data = controller_analog(1);
      break;
    case 0x06:
      *data = controller_analog(2) >> 8;
      break;
    case 0x07:
      *data = controller_analog(2);
      break;
    case 0x08:
      *data = controller_analog(3) >> 8;
      break;
    case 0x09:
      *data = controller_analog(3);
      break;
    case 0x0a:
      *data = controller_analog(4) >> 8;
      break;
    case 0x0b:
      *data = controller_analog(4);
      break;
    case 0x0c:
      *data = controller_analog(5) >> 8;
      break;
    case 0x0d:
      *data = controller_analog(5);
      break;
    default:
      *data = 0;
      break;
  }
  reg++;
  return true;
}

static void end() {}

void grove_init(void (*interrupt_handler)(void)) {
  struct i2c i2c;
  memset(&i2c, 0, sizeof(i2c));
  i2c.exclusive_time_raw = 1000 * 16;  // 1sec
  i2c.interrupt_handler = interrupt_handler;
  i2c.sda = I2C_SDA_P0_2;
  i2c.start = start;
  i2c.write = write;
  i2c.read = read;
  i2c.end = end;
  i2c_init(&i2c);
}

void grove_update_interrupt(uint8_t ie) {
  i2c_update_interrupt(ie);
}