// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "controller.h"

#include "ch559.h"
#include "serial.h"

#include "settings.h"

static uint8_t digital_map[3][4];
static uint16_t analog[8];  // TODO: split to 2P

static uint16_t raw_digital[2];
static uint16_t raw_analog[2][6];

static bool button_check(uint16_t index, const uint8_t* data) {
  if (index == 0xffff) {
    return false;
  }
  uint8_t byte = index >> 3;
  uint8_t bit = index & 7;
  return data[byte] & (1 << bit);
}

uint16_t analog_check(const struct hid_info* info,
                      const uint8_t* data,
                      uint8_t index) {
  if (info->axis[index] == 0xffff) {
    // return 0x8000;
  } else if (info->axis_size[index] == 8) {
    uint8_t v = data[info->axis[index] >> 3];
    v <<= info->axis_shift[index];
    if (info->axis_sign[index]) {
      v += 0x80;
    }
    if (info->axis_polarity[index]) {
      v = 0xff - v;
    }
    return v << 8;
  } else if (info->axis_size[index] == 12) {
    uint8_t byte_index = info->axis[index] >> 3;
    uint16_t l = data[byte_index + 0];
    uint16_t h = data[byte_index + 1];
    uint16_t v = ((info->axis[index] & 7) == 0) ? (((h << 8) & 0x0f00) | l)
                                                : ((h << 4) | (l >> 4));
    v <<= info->axis_shift[index];
    if (info->axis_sign[index]) {
      v += 0x0800;
    }
    if (info->axis_polarity[index]) {
      v = 0x0fff - v;
    }
    return v << 4;
  } else if (info->axis_size[index] == 16) {
    uint8_t byte = info->axis[index] >> 3;
    uint16_t v = data[byte] | ((uint16_t)data[byte + 1] << 8);
    v <<= info->axis_shift[index];
    if (info->axis_sign[index]) {
      v += 0x8000;
    }
    if (info->axis_polarity[index]) {
      v = 0xffff - v;
    }
    return v;
  }
  return 0x8000;
}

static void reset_digital_map(uint8_t player) {
  for (uint8_t i = 0; i < 4; ++i) {
    digital_map[player][i] = 0;
  }
}

static void merge_digital_map() {
  for (uint8_t i = 0; i < 4; ++i) {
    digital_map[2][i] = digital_map[0][i] | digital_map[1][i];
  }
}

static void update_digital_map(uint8_t* dst, uint16_t src, bool on) {
  if (!on) {
    return;
  }
  dst[0] |= src >> 8;
  dst[1] |= src;
}

static void reset_raw_data(uint8_t player) {
  raw_digital[player] = 0;
  for (uint8_t i = 0; i < 6; ++i) {
    raw_analog[player][i] = 0;
  }
}

void controller_reset(void) {
  for (uint8_t p = 0; p < 3; ++p) {
    reset_digital_map(p);
  }
  for (uint8_t i = 0; i < 8; ++i) {
    analog[i] = 0;
  }
  for (uint8_t p = 0; p < 2; ++p) {
    reset_raw_data(p);
  }
}

void controller_update(const uint8_t hub,
                       const struct hid_info* info,
                       const uint8_t* data,
                       uint16_t size) {
  if (info->report_id) {
    if (info->report_id != data[0]) {
      return;
    }
    data++;
    size--;
  }
  reset_digital_map(hub);

  if (info->state != HID_STATE_READY) {
    merge_digital_map();
    reset_raw_data(hub);
    return;
  }

  uint16_t raw_data = 0;
  bool u = button_check(info->dpad[0], data);
  bool d = button_check(info->dpad[1], data);
  bool l = button_check(info->dpad[2], data);
  bool r = button_check(info->dpad[3], data);
  if (info->hat != 0xffff) {
    uint8_t byte = info->hat >> 3;
    uint8_t bit = info->hat & 7;
    uint8_t hat = (data[byte] >> bit) & 0xf;
    switch (hat) {
      case 0:
        u |= true;
        break;
      case 1:
        u |= true;
        r |= true;
        break;
      case 2:
        r |= true;
        break;
      case 3:
        r |= true;
        d |= true;
        break;
      case 4:
        d |= true;
        break;
      case 5:
        d |= true;
        l |= true;
        break;
      case 6:
        l |= true;
        break;
      case 7:
        l |= true;
        u |= true;
        break;
    }
  }
  raw_data =
      (u ? 0x8000 : 0) | (d ? 0x4000 : 0) | (l ? 0x2000 : 0) | (r ? 0x1000 : 0);

  struct settings* settings = settings_get();
  uint8_t alt_digital = 0;
  for (uint8_t i = 0; i < 6; ++i) {
    uint16_t value = analog_check(info, data, i);
    raw_analog[hub][i] = value;
    if (i == 0) {
      l |= value < 0x6000;
      r |= value > 0xa000;
    } else if (i == 1) {
      u |= value < 0x6000;
      d |= value > 0xa000;
    }
    uint8_t index = settings->map[hub].analog[i].map;
    if (index != 0xff) {
      analog[index] = value;
    }
  }

  update_digital_map(digital_map[hub], settings->map[hub].digital[0].map, u);
  update_digital_map(digital_map[hub], settings->map[hub].digital[1].map, d);
  update_digital_map(digital_map[hub], settings->map[hub].digital[2].map, l);
  update_digital_map(digital_map[hub], settings->map[hub].digital[3].map, r);
  for (uint8_t i = 0; i < 12; ++i) {
    bool on = button_check(info->button[i], data);
    raw_data |= on ? (1 << (11 - i)) : 0;
    uint8_t rapid_fire = settings->map[hub].digital[i].rapid_fire;
    update_digital_map(digital_map[hub], settings->map[hub].digital[4 + i].map,
                       (settings->sequence[rapid_fire].on && on) ^
                           settings->sequence[rapid_fire].invert);
  }
  raw_digital[hub] = raw_data;
  merge_digital_map();
}

uint8_t controller_data(uint8_t player, uint8_t index) {
  uint8_t line = (player << 1) + index;
  if (line >= 4) {
    return 0;
  }
  return digital_map[2][line];
}

uint16_t controller_analog(uint8_t index) {
  if (index < 8) {
    return analog[index];
  }
  return 0x8000;
}

uint16_t controller_raw_digital(uint8_t player) {
  if (player < 2) {
    return raw_digital[player];
  }
  return 0;
}

uint16_t controller_raw_analog(uint8_t player, uint8_t index) {
  if (player < 2 && index < 6) {
    return raw_analog[player][index];
  }
  return 0;
}