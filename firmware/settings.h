// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __settings_h__
#define __settings_h__

#include <stdbool.h>
#include <stdint.h>

#include "led.h"

// Decoded struct of https://github.com/toyoshim/iona-us/wiki/v2-settings
struct settings {
  uint8_t id;
  uint8_t analog_index[2][6];
  bool analog_polarity[2][6];
  struct {
    uint8_t data[4];
  } digital_map[2][16];
  uint8_t rapid_fire[2][12];
  struct {
    uint8_t pattern;
    uint8_t bit;
    uint8_t mask;
    bool invert;
    bool on;
  } sequence[8];
};

void settings_init(void);
void settings_poll(void);
void settings_select(uint8_t id);
struct settings* settings_get(void);
void settings_led_mode(uint8_t mode);
void settings_rapid_sync(void);

#endif  // __settings_h__