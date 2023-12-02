// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __settings_h__
#define __settings_h__

#include <stdbool.h>
#include <stdint.h>

#include "led.h"

struct settings {
  struct {
    struct {
      uint16_t map;
      uint8_t rapid_fire;
    } digital[16];
    struct {
      uint8_t map;
      bool polarity;
    } analog[6];
  } map[2];
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
struct settings* settings_get(void);
void settings_led_mode(uint8_t mode);
void settings_rapid_sync(void);

#endif  // __settings_h__