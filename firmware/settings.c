// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "settings.h"

#include <string.h>

#include "flash.h"
#include "serial.h"

enum {
  NUM_OF_SEQUENCE = 8,
};

static struct settings_sequence sequence[NUM_OF_SEQUENCE];
static uint8_t flash[4 + sizeof(struct settings_map) * 2];
static struct settings_map* settings = (struct settings_map*)&flash[4];
static uint8_t* version = &flash[0];
static uint8_t* mode = &flash[1];

static void load_rapid_fire_preset(void) {
  static const uint8_t patterns[NUM_OF_SEQUENCE] = {0x01, 0x01, 0x01, 0x03,
                                                    0x03, 0x07, 0x07, 0x0f};
  static const uint8_t masks[NUM_OF_SEQUENCE] = {0x01, 0x03, 0x07, 0x0f,
                                                 0x1f, 0x3f, 0x7f, 0xff};

  for (uint8_t i = 0; i < NUM_OF_SEQUENCE; ++i) {
    sequence[i].pattern = patterns[i];
    sequence[i].bit = 1;
    sequence[i].mask = masks[i];
    sequence[i].on = true;
  }
}

static void load_map_preset(void) {
  *version = 1;
  *mode = 0;
  flash[2] = 0;  // padding
  flash[3] = 0;  // padding
  for (uint8_t p = 0; p < 2; ++p) {
    uint16_t bit = 0x2000;
    for (uint8_t i = 0; i < 16; ++i) {
      if (i == 12) {
        settings[p].digital[i].map1 = 0x80;
        settings[p].digital[i].map2 = 0x00;
      } else if (i == 13) {
        settings[p].digital[i].map1 = 0x40;
        settings[p].digital[i].map2 = 0x00;
      } else {
        settings[p].digital[i].map1 = bit >> 8;
        settings[p].digital[i].map2 = bit & 0xff;
        bit >>= 1;
      }
      settings[p].digital[i].rapid_fire = 0;
      settings[p].digital[i].padding = 0;
    }
    settings[p].analog[0] = 1;
    settings[p].analog[1] = 0;
    settings[p].analog[2] = 3;
    settings[p].analog[3] = 2;
    settings[p].analog[4] = 0xff;
    settings[p].analog[5] = 0xff;
  }
}

void settings_init(void) {
  flash_init(*((uint32_t*)"MS68"), true);
  if (!flash_read(4, flash, 4) || *version != 1) {
    load_map_preset();
  } else {
    flash_read(8, (uint8_t*)settings, sizeof(struct settings_map) * 2);
  }
  load_rapid_fire_preset();
}

struct settings_map* settings_get_map(void) {
  return settings;
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
  memcpy(map, settings, sizeof(struct settings_map));
}

void settings_save_map(const struct settings_map* map) {
  memcpy(settings, map, sizeof(struct settings_map));
}

uint8_t settings_load_mode(void) {
  return *mode;
}

void settings_commit_mode(uint8_t new_mode) {
  *mode = new_mode;
  flash_write(4, flash, 4);
}

bool settings_commit(void) {
  return flash_write(4, flash, 4 + sizeof(struct settings_map) * 2);
}