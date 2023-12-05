#include "mscmd.h"

#include "mslib.h"

int ms_check_version(unsigned char* major, unsigned char* minor, unsigned char* patch) {
  int try;
  unsigned char command = 0x00;
  unsigned char data[6+4];
  for (try = 0; try < 2; ++try) {
    unsigned char xor;
    int result = ms_comm(1, &command, data);
    if (result != 0) {
      continue;
    }
    xor = command ^ data[6] ^ data[7] ^ data[8];
    if (data[9] != xor) {
      continue;
    }
    *major = data[6];
    *minor = data[7];
    *patch = data[8];
    return 0;
  }
  return 1;
}

int ms_load_config(struct ms_config* config) {
  unsigned char commands[28] = {
    0x12,        /* Load Layout Settings */
    0xf0, 0x00,  /* Set Transfer Space: Scratch Memory */
    0xf1, 0x00,  /* Set Transfer Address: 0x00 */
    0xf2, 0x00,  /* Set Transfer High Address: 0x00 */
    0xf3, 0x46,  /* Set Transfer Length: 70 bytes */
    0xf5,        /* Taransfer to Host */
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a,  /* transmission for 72 bytes */
  };
  unsigned char data[6 + 28 * 4];
  unsigned char xor;
  int i;
  int offset;
  int result = ms_comm(28, commands, data);
  if (result != 0) {
    return 1;
  }
  offset = 6;
  for (i = 0; i < 10; ++i) {
    unsigned char cmd = commands[i];
    printf("check command %d - $%02x\n", i, cmd);
    xor = cmd ^ data[offset] ^ data[offset + 1] ^ data[offset + 2];
    if (xor != data[offset + 3]) {
      return 2;
    }
    switch (cmd) {
    case 0x0a:
      break;
    case 0x12:
      if (data[offset] != 0x46) {
	return 3;
      }
      break;
    case 0xf0:
    case 0xf1:
    case 0xf2:
    case 0xf3:
    case 0xf5:
      if (data[offset] != cmd || data[offset + 1] != cmd || data[offset + 2] != cmd) {
	return 4;
      }
      break;
    default:
      return 5;
    }
    offset += 4;
    if (cmd < 0xf0 || cmd == 0xf5) {
      continue;
    }
    cmd = commands[++i];
    if (data[offset] != cmd || data[offset + 1] != cmd || data[offset + 2] != cmd ||
	data[offset + 3] != cmd) {
      return 6;
    }
    offset += 4;
  }
  xor = 0;
  for (i = 0; i < 70; ++i) {
    xor ^= data[offset + i];
  }
  if (xor != data[offset + 71]) {
    return 7;
  }
  for (i = 0; i < 16; ++i) {
    config->digital[i].map1 = data[offset++];
    config->digital[i].map2 = data[offset++];
    config->digital[i].rapid_fire = data[offset++];
    offset++;
  }
  for (i = 0; i < 6; ++i) {
    config->analog[i] = data[offset++];
  }
  return 0;
}

int ms_save_config(const struct ms_config* config) {
  unsigned char commands[10 + 70] = {
    0xf0, 0x00,  /* Set Transfer Space: Scratch Memory */
    0xf1, 0x00,  /* Set Transfer Address: 0x00 */
    0xf2, 0x00,  /* Set Transfer High Address: 0x00 */
    0xf3, 0x46,  /* Set Transfer Length: 70 bytes */
    0xf4, 0x0b,  /* Taransfer to Device */
    /* 70 data follows */
  };
  unsigned char commit = 0x11;  /* Store Layout Settings */
  unsigned char data[6 + 80 * 4];
  int result;
  int i;
  int offset;
  unsigned char xor;
  memcpy(&commands[10], config, 70);
  result = ms_comm(80, commands, data);
  if (result != 0) {
    return 1;
  }
  offset = 6;
  for (i = 0; i < 10; ++i) {
    unsigned char cmd = commands[i];
    xor = cmd ^ data[offset] ^ data[offset + 1] ^ data[offset + 2];
    if (xor != data[offset + 3]) {
      return 2;
    }
    if (data[offset] != cmd || data[offset + 1] != cmd || data[offset + 2] != cmd) {
      return 3;
    }
    offset += 4;
    cmd = commands[++i];
    if (data[offset] != cmd || data[offset + 1] != cmd || data[offset + 2] != cmd ||
	data[offset + 3] != cmd) {
      return 4;
    }
    offset += 4;
  }
  for (; i < 80; ++i) {
    unsigned char cmd = commands[i];
    if (data[offset] != cmd || data[offset + 1] != cmd || data[offset + 2] != cmd ||
	data[offset + 3] != cmd) {
      return 5;
    }
    offset += 4;
  }
  result = ms_comm(1, &commit, data);
  if (result != 0) {
    return 6;
  }
  xor = commit ^ data[6] ^ data[7] ^ data[8];
  if (xor != data[9] || data[6] != 0x46) {
    return 7;
  }
  return 0;
}
