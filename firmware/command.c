// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "command.h"

#include "controller.h"
#include "version.h"

void command_execute(uint8_t command, uint8_t* result) {
  uint16_t u16;
  switch (command) {
    case 0x00:  // Get Version
      result[0] = VERSION_MAJOR;
      result[1] = VERSION_MINOR;
      result[2] = VERSION_PATCH;
      break;
    case 0x01:  // Get Raw Digital
      u16 = controller_raw_digital(0);
      result[0] = u16 >> 8;
      result[1] = u16 & 0xff;
      result[2] = 0;
      break;
    case 0x02:  // Get Raw Analog 1-3
      result[0] = controller_raw_analog(0, 0) >> 8;
      result[1] = controller_raw_analog(0, 1) >> 8;
      result[2] = controller_raw_analog(0, 2) >> 8;
      break;
    case 0x03:  // Get Raw Analog 4-6
      result[0] = controller_raw_analog(0, 3) >> 8;
      result[1] = controller_raw_analog(0, 4) >> 8;
      result[2] = controller_raw_analog(0, 5) >> 8;
      break;
    default:  // Unknown
      result[0] = command;
      result[1] = 0xff;
      result[2] = 0xff;
      result[3] = 0xff;  // Invalid SUM
      return;
  }
  result[3] = command ^ result[0] ^ result[1] ^ result[2];
}