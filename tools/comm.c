#include "comm.h"

#include <stdio.h>

#include "mscmd.h"
#include "mslib.h"

volatile unsigned short* mapper_feature = (volatile unsigned short*)0x000ecd000;
volatile unsigned short* mapper_address = (volatile unsigned short*)0x000ecd002;
volatile unsigned short* mapper_data = (volatile unsigned short*)0x000ecd004;
volatile unsigned short* mapper_commit = (volatile unsigned short*)0x000ecd006;

int comm_connect(unsigned char* mode, unsigned char* major, unsigned char* minor, unsigned char* patch) {
  int result = ms_get_version(major, minor, patch);
  if (result == 0) {
    *mode = COMM_MODE_MOONSHOT;
    return 0;
  }
  fprintf(stderr, "Moonshot (%d): device not found, retrying with slow mode option\n", result);

  ms_enter_slow_mode();
  result = ms_get_version(major, minor, patch);
  if (result == 0) {
    *mode = COMM_MODE_MOONSHOT_SLOW;
    return 0;
  }
  fprintf(stderr, "Moonshot (%d): still device not found\n", result);

  if (0x5100 == (*mapper_feature & 0xff00)) {
    unsigned char feature = *mapper_feature;
    *mode = COMM_MODE_JOYPAD_MAPPER_Z;
    *major = 1;
    *minor = (feature >> 4) & 0x0f;
    *patch = feature & 0x0f;
    return 0;
  }

  return 1;
}

int comm_load_config(unsigned char mode, struct layout_config* config) {
  if (mode == COMM_MODE_MOONSHOT || mode == COMM_MODE_MOONSHOT_SLOW) {
    int result = ms_load_config(config);
    if (result != 0) {
      fprintf(stderr, "Moonshot (%d): communication failed to load configuration\n", result);
    }
    return result;
  }

  if (mode == COMM_MODE_JOYPAD_MAPPER_Z) {
    int i;
    for (i = 0; i < 16; ++i) {
      unsigned short data;
      *mapper_address = 16 + i;
      data = *mapper_data;
      config->digital[i].map1 = data >> 8;
      config->digital[i].map2 = data;
      config->digital[i].rapid_fire = 0;
      config->digital[i].padding = 0;
    }
    for (i = 0; i < 6; ++i) {
      *mapper_address = 48 + i;
      config->analog[i] = *mapper_data;
    }
    return 0;
  }
  return 1;
}

int comm_save_config(unsigned char mode, unsigned char transaction, const struct layout_config* config) {
  if (mode == COMM_MODE_MOONSHOT || mode == COMM_MODE_MOONSHOT_SLOW) {
    int result = ms_save_config(config);
    if (result != 0) {
      return result;
    }
    if (transaction == COMM_SAVE_AND_COMMIT) {
      result = ms_commit_config();
    }
    return result;
  }

  if (mode == COMM_MODE_JOYPAD_MAPPER_Z) {
    int i;
    for (i = 0; i < 16; ++i) {
      unsigned short data = (config->digital[i].map1 << 8) | config->digital[i].map2;
      *mapper_address = 16 + i;
      *mapper_data = data;
    }
    for (i = 0; i < 6; ++i) {
      *mapper_address = 48 + i;
      *mapper_data = config->analog[i];
    }
    if (transaction == COMM_SAVE_AND_COMMIT) {
      *mapper_commit = 1;
    }
    return 0;
  }
  return 1;
}

int comm_get_status(unsigned char mode, struct layout_status* status) {
  if (mode == COMM_MODE_MOONSHOT || mode == COMM_MODE_MOONSHOT_SLOW) {
    unsigned char commands[3];
    unsigned char data[6 + 4 * 3];
    int result;
    int i;
    commands[0] = 0x01;
    commands[1] = 0x02;
    commands[2] = 0x03;
    result = ms_comm(3, commands, data);
    for (i = 0; i < 3; ++i) {
      int offset = 6 + i * 4;
      unsigned char xor;
      if (result != 0) {
        return result;
      }
      xor = commands[i] ^ data[offset + 0] ^ data[offset + 1] ^ data[offset + 2];
      if (data[offset + 3] != xor) {
        return -1;
      }
    }
    for (i = 0; i < 6; ++i) {
      status->cyber_map[i] = data[i];
    }
    status->raw_digital[0] = data[6];
    status->raw_digital[1] = data[7];
    status->raw_analog[0] = data[10];
    status->raw_analog[1] = data[11];
    status->raw_analog[2] = data[12];
    status->raw_analog[3] = data[14];
    status->raw_analog[4] = data[15];
    status->raw_analog[5] = data[16];
    return 0;
  }

  if (mode == COMM_MODE_JOYPAD_MAPPER_Z) {
    int i;
    unsigned short data;
    unsigned char analog[4];

    *mapper_address = 96;
    data = ~*mapper_data;
    for (int i = 0; i < 4; ++i) {
      *mapper_address = 104 + i;
      analog[i] = *mapper_data;
    }
    status->cyber_map[0] =
      ((data >> 2) & 0xfc) |   // A,B,C,D,E1,E2,_,_
      ((data >> 14) & 0x03) |  // _,_,_,_,_,_,Start,Select
      ((data << 4) & 0xc0);    // A',B',_,_,_,_,_,_
    status->cyber_map[1] =
      (analog[0] >> 4) | (analog[1] & 0xf0);
    status->cyber_map[2] =
      (analog[2] >> 4) | (analog[3] & 0xf0);
    status->cyber_map[3] =
      (analog[0] & 0x0f) | (analog[1] << 4);
    status->cyber_map[4] =
      (analog[2] & 0x0f) | (analog[3] << 4);
    status->cyber_map[5] =
      ((data >> 2) & 0xc0) |   // A,B,_,_,_,_,_,_
      ((data << 2) & 0x30) |   // _,_,A',B',_,_,_,_
      0x0f;

    *mapper_address = 64;
    data = *mapper_data;
    status->raw_digital[0] = data >> 8;
    status->raw_digital[1] = data & 0xff;
    for (int i = 0; i < 6; ++i) {
      *mapper_address = 72 + i;
      data = *mapper_data;
      status->raw_analog[i] = data;
    }
    return 0;
  }
  return 1;
}