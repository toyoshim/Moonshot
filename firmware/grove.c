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
  FACES,
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
  if (address != 0x58 && address != 0x08) {
    return false;
  }
  phase = (address == 0x08) ? FACES : REG;
  return true;
}

static bool write(uint8_t data) {
  if (phase == REG) {
    reg = data;
    phase = VALUE;
    return true;
  } else if (phase == FACES) {
    return true;
  }
  return false;
}

static bool read(uint8_t* data) {
  if (phase == IDLE) {
    return false;
  }
  if (phase == FACES) {
    uint16_t digital = controller_digital(0);
    *data = 0xff;
    if (digital & 0x8000) {
      *data &= 0x7f;
    }
    if (digital & 0x4000) {
      *data &= 0xbf;
    }
    if (digital & 0x2000) {
      *data &= 0xfe;
    }
    if (digital & 0x1000) {
      *data &= 0xfd;
    }
    if (digital & 0x0800) {
      *data &= 0xfb;
    }
    if (digital & 0x0400) {
      *data &= 0xf7;
    }
    if (digital & 0x0200) {
      *data &= 0xef;
    }
    if (digital & 0x0100) {
      *data &= 0xdf;
    }
    return true;
  }
  switch (reg) {
    case 0x00:
      *data = controller_raw_digital(0) >> 8;
      break;
    case 0x01:
      *data = controller_raw_digital(0);
      break;
    case 0x02:
      *data = controller_raw_analog(0, 0) >> 8;
      break;
    case 0x03:
      *data = controller_raw_analog(0, 0);
      break;
    case 0x04:
      *data = controller_raw_analog(0, 1) >> 8;
      break;
    case 0x05:
      *data = controller_raw_analog(0, 1);
      break;
    case 0x06:
      *data = controller_raw_analog(0, 2) >> 8;
      break;
    case 0x07:
      *data = controller_raw_analog(0, 2);
      break;
    case 0x08:
      *data = controller_raw_analog(0, 3) >> 8;
      break;
    case 0x09:
      *data = controller_raw_analog(0, 3);
      break;
    case 0x0a:
      *data = controller_raw_analog(0, 4) >> 8;
      break;
    case 0x0b:
      *data = controller_raw_analog(0, 4);
      break;
    case 0x0c:
      *data = controller_raw_analog(0, 5) >> 8;
      break;
    case 0x0d:
      *data = controller_raw_analog(0, 5);
      break;
    default:
      *data = 0;
      break;
  }
  reg++;
  return true;
}

static void end(void) {}

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