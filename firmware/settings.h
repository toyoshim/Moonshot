// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __settings_h__
#define __settings_h__

#include <stdbool.h>
#include <stdint.h>

struct settings_map {
  struct {
    uint8_t map1;
    uint8_t map2;
    uint8_t rapid_fire;
    uint8_t padding;
  } digital[16];
  uint8_t analog[6];
};
struct settings_sequence {
  uint8_t pattern;
  uint8_t bit;
  uint8_t mask;
  bool on;
};

void settings_init(void);
struct settings_map* settings_get_map(void);
struct settings_sequence* settings_get_sequence(void);
void settings_rapid_sync(void);
void settings_load_map(struct settings_map* map);
void settings_save_map(const struct settings_map* map);
bool settings_commit(void);

#endif  // __settings_h__