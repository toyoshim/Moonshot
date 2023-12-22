// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "controller.h"

#include "ch559.h"
#include "serial.h"

#include "settings.h"

static uint16_t digital[2];
static uint16_t analog[2][6];

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
  } else if (info->axis_size[index] == 10 || info->axis_size[index] == 12) {
    uint8_t byte_index = info->axis[index] >> 3;
    uint16_t l = data[byte_index + 0];
    uint16_t h = data[byte_index + 1];
    uint16_t v = (((h << 8) | l) >> (info->axis[index] & 7))
                 << (16 - info->axis_size[index]);
    v <<= info->axis_shift[index];
    if (info->axis_sign[index]) {
      v += 0x8000;
    }
    if (info->axis_polarity[index]) {
      v = 0xffff - v;
    }
    return v;
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

static void reset_data(uint8_t player) {
  digital[player] = 0;
  raw_digital[player] = 0;
  for (uint8_t i = 0; i < 6; ++i) {
    analog[player][i] = 0;
    raw_analog[player][i] = 0;
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
  if (info->state != HID_STATE_READY) {
    reset_data(hub);
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

  struct settings_map* settings = settings_get_map();
  uint8_t i;
  for (i = 0; i < 6; ++i) {
    uint16_t value = analog_check(info, data, i);
    raw_analog[hub][i] = value;
    if (i == 0) {
      l |= value < 0x6000;
      r |= value > 0xa000;
    } else if (i == 1) {
      u |= value < 0x6000;
      d |= value > 0xa000;
    }
    uint8_t index = settings->analog[i];
    if (index != 0xff) {
      analog[hub][index] = value;
    }
  }

  uint8_t digital_map1 = 0;
  uint8_t digital_map2 = 0;
  if (u) {
    digital_map1 |= settings->digital[0].map1;
    digital_map2 |= settings->digital[0].map2;
  }
  if (d) {
    digital_map1 |= settings->digital[1].map1;
    digital_map2 |= settings->digital[1].map2;
  }
  if (l) {
    digital_map1 |= settings->digital[2].map1;
    digital_map2 |= settings->digital[2].map2;
  }
  if (r) {
    digital_map1 |= settings->digital[3].map1;
    digital_map2 |= settings->digital[3].map2;
  }

  struct settings_sequence* sequence = settings_get_sequence();
  for (i = 0; i < 12; ++i) {
    bool on = button_check(info->button[i], data);
    raw_data |= on ? (1 << (11 - i)) : 0;
    uint8_t rapid_fire = settings->digital[4 + i].rapid_fire;
    if (sequence[rapid_fire].on && on) {
      digital_map1 |= settings->digital[4 + i].map1;
      digital_map2 |= settings->digital[4 + i].map2;
    }
  }
  digital[hub] = (digital_map1 << 8) | digital_map2;
  raw_digital[hub] = raw_data;
}

uint16_t controller_digital(uint8_t player) {
  return digital[player];
}

uint16_t controller_analog(uint8_t player, uint8_t index) {
  return analog[player][index];
}

uint16_t controller_raw_digital(uint8_t player) {
  return raw_digital[player];
}

uint16_t controller_raw_analog(uint8_t player, uint8_t index) {
  return raw_analog[player][index];
}