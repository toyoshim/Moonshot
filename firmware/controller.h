// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __controller_h__
#define __controller_h__

#include <stdbool.h>
#include <stdint.h>

#include "usb/hid/hid.h"

void controller_update(const uint8_t hub,
                       const struct hid_info* info,
                       const uint8_t* data,
                       uint16_t size);
uint16_t controller_digital(uint8_t player);
uint16_t controller_analog(uint8_t player, uint8_t index);
uint16_t controller_raw_digital(uint8_t player);
uint16_t controller_raw_analog(uint8_t player, uint8_t index);

#endif  // __controller_h__