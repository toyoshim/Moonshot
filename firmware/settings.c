// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "settings.h"

#include <string.h>

#include "flash.h"
#include "serial.h"

static struct settings_map settings;
static struct settings_sequence sequence[8];
static uint8_t flash[169];

static void load_rapid_fire_preset(void) {
  static const uint8_t patterns[] = {0x01, 0x01, 0x01, 0x03,
                                     0x03, 0x07, 0x07, 0x0f};
  static const uint8_t masks[] = {0x01, 0x03, 0x07, 0x0f,
                                  0x1f, 0x3f, 0x7f, 0xff};

  for (uint8_t i = 0; i < 8; ++i) {
    sequence[i].pattern = patterns[i];
    sequence[i].bit = 1;
    sequence[i].mask = masks[i];
    sequence[i].invert = false;
    sequence[i].on = true;
  }
}

bool settings_init(void) {
  if (flash_init(*((uint32_t*)"MS68"), false)) {
    if (!flash_read(8, (uint8_t*)&settings, sizeof(struct settings_map))) {
      // TODO: support default settings
      return false;
    }
    load_rapid_fire_preset();
  }
  return true;
}

struct settings_map* settings_get_map(void) {
  return &settings;
}

struct settings_sequence* settings_get_sequence(void) {
  return sequence;
}

void settings_rapid_sync(void) {
  for (uint8_t i = 1; i < 8; ++i) {
    sequence[i].bit <<= 1;
    if (0 == (sequence[i].bit & sequence[i].mask)) {
      sequence[i].bit = 1;
    }
    sequence[i].on = sequence[i].pattern & sequence[i].bit;
  }
}

void settings_load_map(struct settings_map* map) {
  memcpy(map, &settings, sizeof(struct settings_map));
}

void settings_save_map(const struct settings_map* map) {
  memcpy(&settings, map, sizeof(struct settings_map));
}

bool settings_commit(void) {
  flash[0] = 1;  // version
  flash[1] = 0;  // mode
  flash[2] = 0;  // padding
  flash[3] = 0;  // padding
  settings_load_map((struct settings_map*)&flash[4]);
  return flash_write(4, flash, 4 + sizeof(struct settings_map));
}