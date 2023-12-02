// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "settings.h"

#include "flash.h"
#include "serial.h"

static struct settings settings;
static uint8_t flash[169];

static bool load_ionc_config(void) {
  if (!flash_read(10, flash, 169)) {
    return false;
  }
  uint16_t offset = 3;  // Skip core settings
  // Analiog map
  for (uint8_t p = 0; p < 2; ++p) {
    for (uint8_t i = 0; i < 6; ++i) {
      const uint8_t data = flash[offset++];
      const uint8_t type = data >> 4;
      settings.map[p].analog[i].map = (type == 2) ? (data & 7) : 0xff;
      settings.map[p].analog[i].polarity = data & 8;
    }
  }

  // Digital map
  for (uint8_t p = 0; p < 2; ++p) {
    for (uint8_t i = 0; i < 16; ++i) {
      uint16_t map = (flash[offset++] << 8) | flash[offset++];
      settings.map[p].digital[i].map = map;
      // Skip cross-player map
      offset += 2;
    }
  }

  // Rapid fire settings
  for (uint8_t p = 0; p < 2; ++p) {
    for (uint8_t i = 0; i < 8; ++i) {
      const uint8_t data = (i < 2) ? 0 : flash[offset++];
      settings.map[p].digital[i * 2 + 0].rapid_fire = (data >> 4) & 7;
      settings.map[p].digital[i * 2 + 1].rapid_fire = data & 7;
    }
  }

  // Rapid fire patterns.
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

static void load_rapid_fire_preset(void) {
  static const uint8_t patterns[] = {0x01, 0x01, 0x01, 0x03,
                                     0x03, 0x07, 0x07, 0x0f};
  static const uint8_t masks[] = {0x01, 0x03, 0x07, 0x0f,
                                  0x1f, 0x3f, 0x7f, 0xff};

  for (uint8_t i = 1; i < 8; ++i) {
    settings.sequence[i].pattern = patterns[i];
    settings.sequence[i].bit = 1;
    settings.sequence[i].mask = masks[i];
    settings.sequence[i].invert = false;
    settings.sequence[i].on = true;
  }
}

static bool load_ms68_config(void) {
  // TODO: support default mode
  struct settings_ms68 ms68;
  if (!flash_read(8, (uint8_t*)&ms68, sizeof(struct settings_ms68))) {
    return false;
  }
  settings_deserialize(&ms68, 0);

  load_rapid_fire_preset();
  return true;
}

bool settings_init(void) {
  if (flash_init(*((uint32_t*)"IONC"), false)) {
    if (!load_ionc_config()) {
      return false;
    }
  } else if (flash_init(*((uint32_t*)"MS68"), false)) {
    if (!load_ms68_config()) {
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

void settings_serialize(struct settings_ms68* ms68, uint8_t player) {
  for (uint8_t i = 0; i < 16; ++i) {
    ms68->digital[i].map1 = settings.map[player].digital[i].map >> 8;
    ms68->digital[i].map2 = settings.map[player].digital[i].map;
    ms68->digital[i].rapid_fire = settings.map[player].digital[i].rapid_fire;
    ms68->digital[i].padding = 0;
  }
  for (uint8_t i = 0; i < 6; ++i) {
    ms68->analog[i] = settings.map[player].analog[i].map;
  }
}

void settings_deserialize(const struct settings_ms68* ms68, uint8_t player) {
  for (uint8_t i = 0; i < 16; ++i) {
    settings.map[player].digital[i].map =
        (ms68->digital[i].map1 << 8) | ms68->digital[i].map2;
    settings.map[player].digital[i].rapid_fire = ms68->digital[i].rapid_fire;
  }
  for (uint8_t i = 0; i < 6; ++i) {
    settings.map[player].analog[i].map = ms68->analog[i];
    settings.map[player].analog[i].polarity = false;
  }
}

bool settings_save(void) {
  uint8_t magic[4];
  if (!flash_read(0, magic, 4) || *(uint32_t*)magic != *(uint32_t*)"MS68") {
    if (!flash_init(*((uint32_t*)"MS68"), true)) {
      return false;
    }
    load_rapid_fire_preset();
  }
  settings_serialize((struct settings_ms68*)&flash[4], 0);
  settings_serialize(
      (struct settings_ms68*)&flash[4 + sizeof(struct settings_ms68)], 1);
  flash[0] = 1;  // version
  flash[1] = 0;  // mode
  flash[2] = 0;  // padding
  flash[3] = 0;  // padding
  return flash_write(4, flash, 4 + sizeof(struct settings_ms68) * 2);
}