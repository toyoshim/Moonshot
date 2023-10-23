// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "atari.h"

#include <stdbool.h>
#include <stdint.h>

#include "gpio.h"
#include "io.h"
#include "serial.h"
#include "uart1.h"

#include "controller.h"

static volatile uint8_t out[2] = {0xff, 0xff};

void atari_init(void) {
  gpio_enable_opendrain(2);

  for (uint8_t bit = 0; bit < 6; ++bit) {
    pinMode(2, bit, OUTPUT);
    digitalWrite(2, bit, HIGH);
  }

  // For proto.1
  pinMode(2, 6, INPUT_PULLUP);
}

void atari_poll(void) {
  // ______0__1_
  // P2_0: U  6
  // P2_1: D  5
  // P2_2: L  4
  // P2_3: R Select
  // P2_4: 1  3
  // P2_5: 2 Start
  uint8_t buttons = controller_data(0, 0, 0);
  out[0] = ((buttons & 0x20) ? 0 : 0x01) | ((buttons & 0x10) ? 0 : 0x02) |
           ((buttons & 0x08) ? 0 : 0x04) | ((buttons & 0x04) ? 0 : 0x08) |
           ((buttons & 0x02) ? 0 : 0x10) | ((buttons & 0x01) ? 0 : 0x20);
  out[1] = ((buttons & 0x40) ? 0 : 0x08) | ((buttons & 0x80) ? 0 : 0x20);

  buttons = controller_data(0, 1, 0);
  out[1] |= ((buttons & 0x10) ? 0 : 0x01) | ((buttons & 0x20) ? 0 : 0x02) |
            ((buttons & 0x40) ? 0 : 0x04) | ((buttons & 0x80) ? 0 : 0x10);
  P2 = out[0];
}