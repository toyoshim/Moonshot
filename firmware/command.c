// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "command.h"

#include "controller.h"
#include "settings.h"
#include "version.h"

enum {
  GET_VERSION = 0x00,
  GET_RAW_DIGITAL = 0x01,
  GET_RAW_ANALOG_1_3 = 0x02,
  GET_RAW_ANALOG_4_6 = 0x03,

  STORE_OPERATING_PLAYER = 0x10,
  STORE_LAYOUT_SETTINGS = 0x11,
  LOAD_LAYOUT_SETTINGS = 0x12,

  SET_TRANSFER_SPACE = 0xf0,
  SET_TRANSFER_ADDRESS = 0xf1,
  SET_TRANSFER_HIGH_ADDRESS = 0xf2,
  SET_TRANSFER_LENGTH = 0xf3,
  TRANSFER_TO_DEVICE = 0xf4,
  TRANSFER_TO_HOST = 0xf5,
};

static uint8_t scratch_memory[256];
static uint8_t operating_player = 0;
static uint8_t space = 0;
static uint16_t address = 0;
static uint8_t length = 0;
static uint8_t transaction_command = 0;
static uint8_t check_xor = 0;

static uint8_t transfer(void) {
  if (length == 0) {
    return check_xor;
  }
  uint8_t data = scratch_memory[address++ & 0xff];
  check_xor ^= data;
  length--;
  return data;
}

static bool run_transaction(uint8_t data, uint8_t* result) {
  bool success = true;
  switch (transaction_command) {
    case SET_TRANSFER_SPACE:
      space = data;
      break;
    case SET_TRANSFER_ADDRESS:
      address = (address & 0xff00) | data;
      break;
    case SET_TRANSFER_HIGH_ADDRESS:
      address = (address & 0x00ff) | ((uint16_t)data << 8);
      break;
    case SET_TRANSFER_LENGTH:
      length = data;
      break;
    case TRANSFER_TO_DEVICE:
      if (data != (uint8_t)~TRANSFER_TO_DEVICE) {
        success = false;
        break;
      }
      transaction_command = (uint8_t)~TRANSFER_TO_DEVICE;
      break;
    case (uint8_t)~TRANSFER_TO_DEVICE:
      if (length == 0) {
        success = false;
        break;
      }
      scratch_memory[address++ & 0xff] = data;
      --length;
      break;
    case TRANSFER_TO_HOST:
      if (data != (uint8_t)~TRANSFER_TO_HOST) {
        success = false;
        break;
      }
      result[0] = transfer();
      result[1] = transfer();
      result[2] = transfer();
      result[3] = transfer();
      return true;
    default:
      transaction_command = 0;
      return false;
  }
  if (success) {
    result[0] = data;
    result[1] = data;
    result[2] = data;
    result[3] = data;
  } else {
    result[0] = 0;
    result[1] = 0;
    result[2] = 0;
    result[3] = 0;
  }
  transaction_command = 0;
  return true;
}

void command_execute(uint8_t command, uint8_t* result) {
  if (transaction_command) {
    if (run_transaction(command, result)) {
      return;
    }
  } else {
    uint16_t u16;
    switch (command) {
      case GET_VERSION:
        result[0] = VERSION_MAJOR;
        result[1] = VERSION_MINOR;
        result[2] = VERSION_PATCH;
        break;
      case GET_RAW_DIGITAL:
        u16 = controller_raw_digital(operating_player);
        result[0] = u16 >> 8;
        result[1] = u16 & 0xff;
        result[2] = 0;
        break;
      case GET_RAW_ANALOG_1_3:
        result[0] = controller_raw_analog(operating_player, 0) >> 8;
        result[1] = controller_raw_analog(operating_player, 1) >> 8;
        result[2] = controller_raw_analog(operating_player, 2) >> 8;
        break;
      case GET_RAW_ANALOG_4_6:
        result[0] = controller_raw_analog(operating_player, 3) >> 8;
        result[1] = controller_raw_analog(operating_player, 4) >> 8;
        result[2] = controller_raw_analog(operating_player, 5) >> 8;
        break;

      case STORE_OPERATING_PLAYER:
        operating_player = scratch_memory[0] & 1;
        result[0] = operating_player;
        result[1] = 0;
        result[2] = 0;
        break;
      case STORE_LAYOUT_SETTINGS:
        settings_deserialize((const struct settings_ms68*)scratch_memory,
                             operating_player);
        result[0] = settings_save() ? sizeof(struct settings_ms68) : 0;
        result[1] = 0;
        result[2] = 0;
        break;
      case LOAD_LAYOUT_SETTINGS:
        settings_serialize((struct settings_ms68*)scratch_memory,
                           operating_player);
        result[0] = sizeof(struct settings_ms68);
        result[1] = 0;
        result[2] = 0;
        break;

      case SET_TRANSFER_SPACE:
      case SET_TRANSFER_ADDRESS:
      case SET_TRANSFER_HIGH_ADDRESS:
      case SET_TRANSFER_LENGTH:
      case TRANSFER_TO_DEVICE:
      case TRANSFER_TO_HOST:
        transaction_command = command;
        check_xor = 0;
        result[0] = command;
        result[1] = command;
        result[2] = command;
        break;
      default:  // Unknown
        result[0] = command;
        result[1] = 0xff;
        result[2] = 0xff;
        result[3] = 0xe0;  // Invalid SUM
        return;
    }
  }
  result[3] = command ^ result[0] ^ result[1] ^ result[2];
}

bool command_in_transaction(void) {
  return transaction_command != 0;
}