// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "command.h"

#include "atari.h"
#include "controller.h"
#include "settings.h"
#include "version.h"

enum {
  GET_VERSION = 0x00,
  GET_RAW_DIGITAL = 0x01,
  GET_RAW_ANALOG_1_3 = 0x02,
  GET_RAW_ANALOG_4_6 = 0x03,
  COMMIT_LAYOUT_SETTINGS = 0x04,

  INV_TRANSFER_TO_HOST = 0x0a,
  INV_TRANSFER_TO_DEVICE = 0x0b,

  STORE_OPERATING_PLAYER = 0x10,
  STORE_LAYOUT_SETTINGS = 0x11,
  LOAD_LAYOUT_SETTINGS = 0x12,
  STORE_OPERATING_MODE = 0x13,

  SET_TRANSFER_SPACE = 0xf0,
  SET_TRANSFER_ADDRESS = 0xf1,
  SET_TRANSFER_HIGH_ADDRESS = 0xf2,
  SET_TRANSFER_LENGTH = 0xf3,
  TRANSFER_TO_DEVICE = 0xf4,
  TRANSFER_TO_HOST = 0xf5,

  SPACE_SCRATCH_MEMORY = 0,
  SPACE_REGISTER = 1,
};

static uint8_t scratch_memory[256];
static uint8_t operating_player = 0;
static uint8_t space = 0;
static uint16_t address = 0;
static uint8_t length = 0;
static uint8_t transaction_command = 0;
static uint8_t check_xor = 0;

static void data_write(uint16_t address, uint8_t data) {
  if (space == SPACE_SCRATCH_MEMORY) {
    scratch_memory[address & 0xff] = data;
  }
}

static uint8_t data_read(uint16_t address) {
  if (space == SPACE_SCRATCH_MEMORY) {
    return scratch_memory[address & 0xff];
  }
  return 0;
}

static uint8_t transfer(void) {
  if (length == 0) {
    transaction_command = 0;
    return check_xor;
  }
  uint8_t data = data_read(address++);
  check_xor ^= data;
  length--;
  return data;
}

static bool run_transaction(uint8_t data, uint8_t* result) {
  bool success = true;
  switch (transaction_command) {
    case SET_TRANSFER_SPACE:
      space = data;
      transaction_command = 0;
      break;
    case SET_TRANSFER_ADDRESS:
      address = (address & 0xff00) | data;
      transaction_command = 0;
      break;
    case SET_TRANSFER_HIGH_ADDRESS:
      address = (address & 0x00ff) | ((uint16_t)data << 8);
      transaction_command = 0;
      break;
    case SET_TRANSFER_LENGTH:
      length = data;
      transaction_command = 0;
      break;
    case TRANSFER_TO_DEVICE:
      if (data != INV_TRANSFER_TO_DEVICE) {
        success = false;
        break;
      }
      transaction_command = INV_TRANSFER_TO_DEVICE;
      break;
    case INV_TRANSFER_TO_DEVICE:
      data_write(address++, data);
      --length;
      if (length == 0) {
        transaction_command = 0;
      }
      break;
    case TRANSFER_TO_HOST:
      if (data != INV_TRANSFER_TO_HOST) {
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
    result[0] = data;
    result[1] = data;
    result[2] = data;
    result[3] = 0xe0;
    transaction_command = 0;
  }
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
      case COMMIT_LAYOUT_SETTINGS:
        settings_commit();
        result[0] = sizeof(struct settings_map);
        result[1] = 0;
        result[2] = 0;
        break;

      case STORE_OPERATING_PLAYER:
        operating_player = scratch_memory[0] & 1;
        result[0] = operating_player;
        result[1] = 0;
        result[2] = 0;
        break;
      case STORE_LAYOUT_SETTINGS:
        settings_save_map((const struct settings_map*)scratch_memory);
        result[0] = sizeof(struct settings_map);
        result[1] = 0;
        result[2] = 0;
        break;
      case LOAD_LAYOUT_SETTINGS:
        settings_load_map((struct settings_map*)scratch_memory);
        result[0] = sizeof(struct settings_map);
        result[1] = 0;
        result[2] = 0;
        break;
      case STORE_OPERATING_MODE:
        atari_set_mode(scratch_memory[0]);
        result[0] = scratch_memory[0];
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