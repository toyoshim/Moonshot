// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "settings.h"

#include "flash.h"
#include "serial.h"

static struct settings settings;

static bool apply(void) {
  uint8_t flash[169];
  if (!flash_read(10, flash, 169)) {
    return false;
  }
  uint16_t offset = 10;
  const uint8_t core0 = flash[offset++];
  const uint8_t core1 = flash[offset++];
  const uint8_t core2 = flash[offset++];
  for (uint8_t p = 0; p < 2; ++p) {
    for (uint8_t i = 0; i < 6; ++i) {
      const uint8_t data = flash[offset++];
      const uint8_t type = data >> 4;
      settings.map[p].analog[i].map = (type == 2) ? (data & 7) : 0xff;
      settings.map[p].analog[i].polarity = data & 8;
    }
  }

  for (uint8_t p = 0; p < 2; ++p) {
    for (uint8_t i = 0; i < 16; ++i) {
      settings.map[p].digital[i].map = (flash[offset++] << 8) | flash[offset++];
      offset += 2;
    }
  }

  for (uint8_t p = 0; p < 2; ++p) {
    for (uint8_t i = 0; i < 6; ++i) {
      const uint8_t data = flash[offset++];
      settings.map[p].digital[i * 2 + 0].rapid_fire = (data >> 4) & 7;
      settings.map[p].digital[i * 2 + 1].rapid_fire = data & 7;
    }
  }
  settings.sequence[0].pattern = 0xff;
  settings.sequence[0].bit = 1;
  settings.sequence[0].mask = 0xff;
  settings.sequence[0].invert = false;
  settings.sequence[0].on = true;
  for (uint8_t i = 1; i < 8; ++i) {
    settings.sequence[i].pattern = flash[offset++];
    settings.sequence[i].bit = 1;
    settings.sequence[i].mask = (2 << (flash[offset] & 7)) - 1;
    settings.sequence[i].invert = flash[offset++] & 0x80;
    settings.sequence[i].on = true;
  }
  return true;
}

bool settings_init(void) {
  if (flash_init(*((uint32_t*)"IONC"), false)) {
    if (!apply()) {
      return false;
    }
  } else {
    return false;
  }
  return true;
}

struct settings* settings_get(void) {
  return &settings;
}

void settings_rapid_sync(void) {
  for (uint8_t i = 1; i < 8; ++i) {
    settings.sequence[i].bit <<= 1;
    if (0 == (settings.sequence[i].bit & settings.sequence[i].mask)) {
      settings.sequence[i].bit = 1;
    }
    settings.sequence[i].on =
        settings.sequence[i].pattern & settings.sequence[i].bit;
  }
}