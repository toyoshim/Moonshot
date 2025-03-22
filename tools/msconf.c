#include <doslib.h>
#include <iocslib.h>
#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "layout_config.h"

#ifdef XDEV68K
#define _dos_kflushio KFLUSHIO
#define _iocs_bitsns BITSNS
#define _iocs_b_super B_SUPER
#endif

static const unsigned char msconf_version_major = 1;
static const unsigned char msconf_version_minor = 2;
static const unsigned char msconf_version_patch = 2;

static unsigned char communication_mode = 0;
static unsigned char version_major = 0;
static unsigned char version_minor = 0;
static unsigned char version_patch = 0;
static struct layout_config config;
static int label_type = 0;
static int cursor_y = 0; /* 0 - 17 */
static int cursor_x = 0; /* 0 - 10 */
static int cursor_analog_x = 0;
static int cursor_digital_x = 0;
static int dirty = 0;
static int commit_or_quit = 0;

enum {
  CURSOR_DIGITAL_Y_MAX = 15,
  CURSOR_ANALOG_Y_MIN = 16,
  CURSOR_ANALOG_Y_MAX = 21,
  CURSOR_DIGITAL_X_MAX = 14,
  CURSOR_ANALOG_X_MAX = 4,
};

void show_label(int type) {
  /* clang-format off */
  char* labels[] = {
      "Cyber >Ñ†             Ñ† Sl St Ñ† Ç` Ça Çb Ñ† Çc E1 E2 Ñ† A' B' Ñ†+- Çx Çw Th X2",
      " CPSF >Ñ† Å™ Å´ Å© Å® Ñ† Sl St Ñ† Ç` Ça Çk Ñ† Çw Çx Çq Ñ†       Ñ†              ",
      " MD6B >Ñ† Å™ Å´ Å© Å® Ñ† Sl St Ñ† Ç` Ça Çb Ñ† Çw Çx Çy Ñ†       Ñ†              ",
      "NORMAL>Ñ† Å™ Å´ Å© Å® Ñ† Sl St Ñ† Ç` Ça    Ñ†          Ñ†       Ñ†              ",
  };
  char* labelsz[] = {
      "Legacy>Ñ† Å™ Å´ Å© Å® Ñ†       Ñ† Ç` Ça    Ñ†          Ñ†       Ñ†              ",
      "Std2B >Ñ† Å™ Å´ Å© Å® Ñ†       Ñ† Ç` Ça    Ñ†          Ñ†       Ñ†              ",
      "Cyber >Ñ†             Ñ† Sl St Ñ† Ç` Ça Çb Ñ† Çc E1 E2 Ñ† A' B' Ñ†+- Çx Çw Th X2",
      "MagPad>Ñ† Å™ Å´ Å© Å® Ñ†       Ñ† Ç` Ça Çb Ñ† Çc Çk Çq Ñ†       Ñ†              ",
      " CPSF >Ñ† Å™ Å´ Å© Å® Ñ† Sl St Ñ† Ç` Ça Çk Ñ† Çw Çx Çq Ñ†       Ñ†              ",
      " MD6B >Ñ† Å™ Å´ Å© Å® Ñ† Sl St Ñ† Ç` Ça Çb Ñ† Çw Çx Çy Ñ†       Ñ†              ",
  };
  /* clang-format on */
  printf("\033[3;9H%s", (communication_mode == COMM_MODE_JOYPAD_MAPPER_Z)
                            ? labelsz[type]
                            : labels[type]);
  comm_set_layout_mode(communication_mode, type);
}

void show_cursor(int active) {
  int digital_xs[15] = {
      18, 21, 24, 27, 33, 36, 42, 45, 48, 54, 57, 60, 66, 69, 80,
  };
  int analog_xs[5] = {
      75, 77, 80, 83, 86,
  };
  int digital_widths[15] = {
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5,
  };
  int analog_widths[5] = {
      1, 2, 2, 2, 2,
  };
  int* xs = (cursor_y > CURSOR_DIGITAL_Y_MAX) ? analog_xs : digital_xs;
  int* widths =
      (cursor_y > CURSOR_DIGITAL_Y_MAX) ? analog_widths : digital_widths;
  if (active) {
    printf("\033[%d;%dH[\033[%dC]", 7 + cursor_y, xs[cursor_x],
           widths[cursor_x]);
  } else {
    printf("\033[%d;%dH \033[%dC ", 7 + cursor_y, xs[cursor_x],
           widths[cursor_x]);
  }
}

void show_button(int y, int x, int active) {
  printf("\033[%d;%dH%s", y, x, active ? "Åú" : "Åõ");
}

void show_value(int y, int x, unsigned char value) {
  printf("\033[%d;%dH%02x", y, x, value);
}

void show_rapid_fire(int y, int x, int mode) {
  char* modes[] = {
      " OFF ", " 30/s", " 20/s", " 15/s", " 12/s", " 10/s", "8.6/s", "7.5/s",
  };
  printf("\033[%d;%dH%s", y, x,
         (communication_mode == COMM_MODE_JOYPAD_MAPPER_Z) ? " N/A "
                                                           : modes[mode]);
}

void show_version() {
  printf("\033[%d;%dH%d.%02d.%d", 27, 66, msconf_version_major,
         msconf_version_minor, msconf_version_patch);
  printf("\033[%d;%dH%d.%02d.%d", 28, 66, version_major, version_minor,
         version_patch);
}

void show_status(const char* status) {
  int length = strlen(status);
  printf("\033[%d;%dH\033[1K%s", 30, 88 - length, status);
  fflush(stdout);
}

void show_digital(int i) {
  show_button(7 + i, 19, config.digital[i].map1 & 0x20); /* Up     */
  show_button(7 + i, 22, config.digital[i].map1 & 0x10); /* Down   */
  show_button(7 + i, 25, config.digital[i].map1 & 0x08); /* Left   */
  show_button(7 + i, 28, config.digital[i].map1 & 0x04); /* Right  */
  show_button(7 + i, 34, config.digital[i].map1 & 0x40); /* Select */
  show_button(7 + i, 37, config.digital[i].map1 & 0x80); /* Start  */
  show_button(7 + i, 43, config.digital[i].map1 & 0x02); /* 1      */
  show_button(7 + i, 46, config.digital[i].map1 & 0x01); /* 2      */
  show_button(7 + i, 49, config.digital[i].map2 & 0x80); /* 3      */
  show_button(7 + i, 55, config.digital[i].map2 & 0x40); /* 4      */
  show_button(7 + i, 58, config.digital[i].map2 & 0x20); /* 5      */
  show_button(7 + i, 61, config.digital[i].map2 & 0x10); /* 6      */
  show_button(7 + i, 67, config.digital[i].map2 & 0x08); /* 7      */
  show_button(7 + i, 70, config.digital[i].map2 & 0x04); /* 8      */

  show_rapid_fire(7 + i, 81, config.digital[i].rapid_fire);
}

void show_analog(int i) {
  unsigned char value = config.analog[i];
  unsigned char target = value & ~4;
  unsigned char polarity = (value & 4) >> 2;
  printf("\033[%d;%dH%c %s %s %s %s", 23 + i, 76, polarity ? '-' : '+',
         (target == 0) ? "Åú" : "Åõ", (target == 1) ? "Åú" : "Åõ",
         (target == 2) ? "Åú" : "Åõ", (target == 3) ? "Åú" : "Åõ");
}

void show_config() {
  int i;
  for (i = 0; i < 16; ++i) {
    show_digital(i);
  }
  for (i = 0; i < 6; ++i) {
    show_analog(i);
  }
}

void flip() {
  if (cursor_y <= CURSOR_DIGITAL_Y_MAX) {
    if (cursor_x < 4) {
      /* Up / Down / Left / Right */
      config.digital[cursor_y].map1 ^= (0x20 >> cursor_x);
    } else if (cursor_x < 6) {
      /* Select / Start */
      config.digital[cursor_y].map1 ^= ((cursor_x == 4) ? 0x40 : 0x80);
    } else if (cursor_x < 8) {
      /* 1 - 2 */
      config.digital[cursor_y].map1 ^= (0x80 >> cursor_x);
    } else if (cursor_x < 14) {
      /* 3 - 8 */
      config.digital[cursor_y].map2 ^= (0x80 >> (cursor_x - 8));
    } else if (communication_mode != COMM_MODE_JOYPAD_MAPPER_Z) {
      /* Rapid Fire */
      config.digital[cursor_y].rapid_fire++;
      config.digital[cursor_y].rapid_fire &= 7;
    }
    show_digital(cursor_y);
  } else {
    int index = cursor_y - CURSOR_ANALOG_Y_MIN;
    if (cursor_x == 0) {
      config.analog[index] ^= 4;
    } else {
      if ((config.analog[index] & ~4) == (cursor_x - 1)) {
        config.analog[index] = 255;
      } else {
        config.analog[index] = cursor_x - 1;
      }
    }
    show_analog(index);
  }
  if (communication_mode != COMM_MODE_MOONSHOT_SLOW) {
    comm_save_config(communication_mode, COMM_SAVE_ONLY, &config);
  }
  dirty = 1;
}

void update() {
  static struct layout_status last_status = {
      {0xff, 0x00, 0x00, 0x00, 0x00, 0xff},
      {0, 0},
      {0, 0, 0, 0, 0, 0},
  };
  static unsigned char last_output_map1 = 0;
  struct layout_status status;
  unsigned char output_map1;
  unsigned short input_map;
  int i;
  if (comm_get_status(communication_mode, &status)) {
    return;
  }
  /* Calculate cooked d-pad data as Cyberstick doesn't send them */
  input_map = (status.raw_digital[0] << 8) | status.raw_digital[1];
  output_map1 = 0;
  for (i = 0; i < 16; ++i) {
    if (input_map & (0x8000 >> i)) {
      output_map1 |= config.digital[i].map1;
    }
  }
  //  Layout Mapper Z does in H5.
  output_map1 |= (~status.cyber_map[5] & 0x0f) << 2;

  if (last_output_map1 != output_map1) {
    show_button(4, 19, output_map1 & 0x20); /* UP    */
    show_button(4, 22, output_map1 & 0x10); /* DOWN  */
    show_button(4, 25, output_map1 & 0x08); /* LEFT  */
    show_button(4, 28, output_map1 & 0x04); /* RIGHT */
    last_output_map1 = output_map1;
  }
  if (last_status.cyber_map[0] != status.cyber_map[0]) {
    unsigned char data = status.cyber_map[0];
    show_button(4, 49, ~data & 0x20); /* C    */
    show_button(4, 55, ~data & 0x10); /* D    */
    show_button(4, 58, ~data & 0x08); /* E1   */
    show_button(4, 61, ~data & 0x04); /* E2   */
    show_button(4, 37, ~data & 0x02); /* St   */
    show_button(4, 34, ~data & 0x01); /* Sl   */
    last_status.cyber_map[0] = data;
  }
  if (last_status.cyber_map[1] != status.cyber_map[1] ||
      last_status.cyber_map[3] != status.cyber_map[3]) {
    /* Y / X */
    show_value(4, 78,
               (status.cyber_map[1] & 0xf0) | (status.cyber_map[3] >> 4));
    show_value(4, 81,
               (status.cyber_map[1] << 4) | (status.cyber_map[3] & 0x0f));
    last_status.cyber_map[1] = status.cyber_map[1];
    last_status.cyber_map[3] = status.cyber_map[3];
  }
  if (last_status.cyber_map[2] != status.cyber_map[2] ||
      last_status.cyber_map[4] != status.cyber_map[4]) {
    /* Th / X2 */
    show_value(4, 84,
               (status.cyber_map[2] & 0xf0) | (status.cyber_map[4] >> 4));
    show_value(4, 87,
               (status.cyber_map[2] << 4) | (status.cyber_map[4] & 0x0f));
    last_status.cyber_map[2] = status.cyber_map[2];
    last_status.cyber_map[4] = status.cyber_map[4];
  }
  if (last_status.cyber_map[5] != status.cyber_map[5]) {
    unsigned char data = status.cyber_map[5];
    show_button(4, 43, ~data & 0x80); /* A    */
    show_button(4, 46, ~data & 0x40); /* B    */
    show_button(4, 67, ~data & 0x20); /* A'   */
    show_button(4, 70, ~data & 0x10); /* B'   */
    last_status.cyber_map[5] = data;
  }
  if (last_status.raw_digital[0] != status.raw_digital[0]) {
    unsigned char data = status.raw_digital[0];
    show_button(7, 14, data & 0x80);  /* UP    */
    show_button(8, 14, data & 0x40);  /* DOWN  */
    show_button(9, 14, data & 0x20);  /* LEFT  */
    show_button(10, 14, data & 0x10); /* RIGHT */
    show_button(11, 14, data & 0x08); /* Dig 1 */
    show_button(12, 14, data & 0x04); /* Dig 2 */
    show_button(13, 14, data & 0x02); /* Dig 3 */
    show_button(14, 14, data & 0x01); /* Dig 4 */
    last_status.raw_digital[0] = data;
  }
  if (last_status.raw_digital[1] != status.raw_digital[1]) {
    unsigned char data = status.raw_digital[1];
    show_button(15, 14, data & 0x80); /* Dig 5 */
    show_button(16, 14, data & 0x40); /* Dig 6 */
    show_button(17, 14, data & 0x20); /* Dig 7 */
    show_button(18, 14, data & 0x10); /* Dig 8 */
    show_button(19, 14, data & 0x08); /* Dig 9 */
    show_button(20, 14, data & 0x04); /* Dig A */
    show_button(21, 14, data & 0x02); /* Dig B */
    show_button(22, 14, data & 0x01); /* Dig C */
    last_status.raw_digital[1] = data;
  }
  for (i = 0; i < 6; ++i) {
    if (last_status.raw_analog[i] != status.raw_analog[i]) {
      show_value(23 + i, 14, status.raw_analog[i]);
      last_status.raw_analog[i] = status.raw_analog[i];
    }
  }
}

void screen_setup() {
  /* clang-format off */
  char* screen[] = {
      "                          * Moonshot Layout Confugurations *                          ",
      "Ñ¨Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ≠",
      "Ñ´<      >Ñ†             Ñ†       Ñ†          Ñ†          Ñ†       Ñ†               Ñ´",
      "Ñ´[OUTPUT]Ñ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†   00 00 00 00 Ñ´",
      "Ñ∞Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ≤",
      "Ñ´[INPUT] Ñ†                   Layout Map Matrix                   Ñ†   RAPID FIRE  Ñ´",
      "Ñ´  Å™  ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´  Å´  ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´  Å©  ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´  Å®  ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Dig1 ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Dig2 ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Dig3 ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Dig4 ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Dig5 ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Dig6 ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Dig7 ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Dig8 ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Dig9 ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ DigA ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ DigB ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ DigC ÅõÑ† Åõ Åõ Åõ Åõ Ñ† Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Åõ Ñ† Åõ Åõ Ñ†       OFF     Ñ´",
      "Ñ´ Ana1 00Ñ†                                                       Ñ† + Åõ Åõ Åõ Åõ Ñ´",
      "Ñ´ Ana2 00Ñ† L: Label Switch (Cyber/CPSF/MD6B)                     Ñ† + Åõ Åõ Åõ Åõ Ñ´",
      "Ñ´ Ana3 00Ñ† S: Save to Device                                     Ñ† + Åõ Åõ Åõ Åõ Ñ´",
      "Ñ´ Ana4 00Ñ† Q: Quit                                               Ñ† + Åõ Åõ Åõ Åõ Ñ´",
      "Ñ´ Ana5 00Ñ†                             MsConfig Version : 0.00.0 Ñ† + Åõ Åõ Åõ Åõ Ñ´",
      "Ñ´ Ana6 00Ñ†                             Firmware Version : 0.00.0 Ñ† + Åõ Åõ Åõ Åõ Ñ´",
      "ÑØÑ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™Ñ™ÑÆ",
      0,
  };
  char* screenz[] = {
      "                          * X68000 Z Layout Confugurations *                          ",
      "Ñ´ Ana2 00Ñ† L: Layout Mode (Legacy/Std2B/Cyber/Mag.Pad/CPSF/MD6B) Ñ† + Åõ Åõ Åõ Åõ Ñ´",
  };
  /* clang-format on */
  int i;
  printf("\033[2J");
  for (i = 0; screen[i]; ++i) {
    char* message = screen[i];
    if (communication_mode == COMM_MODE_JOYPAD_MAPPER_Z) {
      if (i == 0) {
        message = screenz[0];
      } else if (i == 23) {
        message = screenz[1];
      }
    }
    printf("\033[%dC%s\n", 5, message);
  }
  label_type = comm_get_layout_mode(communication_mode);
  show_label(label_type);
  fflush(stdout);
}

int setup(void) {
  int result = comm_connect(&communication_mode, &version_major, &version_minor,
                            &version_patch);
  if (result != 0) {
    return 1;
  }
  result = comm_load_config(communication_mode, &config);
  if (result != 0) {
    fprintf(stderr, "firmware: %d.%02d.%d\n", version_major, version_minor,
            version_patch);
    return 2;
  }

  printf("\033[>5h");

  screen_setup();
  show_version();
  show_config();
  show_cursor(1);
  show_status("Welcome to Controller Layout Configurations!");

  return 0;
}

int save() {
  const char* message = "Save Layout to Device ...";
  char buf[256];
  int result;
  show_status(message);
  result = comm_save_config(communication_mode, COMM_SAVE_AND_COMMIT, &config);
  if (result) {
    sprintf(buf, "%s Error %d", message, result);
  } else {
    sprintf(buf, "%s Done", message);
    dirty = 0;
  }
  show_status(buf);
  return result;
}

int loop(int bitsns) {
  /*
   * current_bitsns: 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
   *          group:        2              2  3  7  7  7  7  5  4  6
   *            key:        Y              Q  S dn rt up lt  N  L sp
   */
  int current_bitsns =
      (_iocs_bitsns(7) & 0x78) | ((_iocs_bitsns(6) & 0x20) >> 5) |
      ((_iocs_bitsns(5) & 0x80) >> 5) | ((_iocs_bitsns(4) & 0x40) >> 5) |
      (_iocs_bitsns(3) & 0x80) | ((_iocs_bitsns(2) & 0x42) << 7);
  int edge = current_bitsns ^ bitsns;
  int posedge = edge & current_bitsns;
  if (posedge) {
    show_cursor(0);
    if (commit_or_quit) {
      if (posedge & 0x2000) { /* Y */
        if (save() == 0) {
          return -1;
        }
        commit_or_quit = 0;
      } else if (posedge & 0x0004) { /* N */
        show_status("Quit");
        return -1;
      }
    } else if (posedge & 0x0010) { /* up */
      cursor_y--;
      if (cursor_y < 0) {
        cursor_y = CURSOR_ANALOG_Y_MAX;
        cursor_digital_x = cursor_x;
        cursor_x = cursor_analog_x;
      }
      if (cursor_y == CURSOR_DIGITAL_Y_MAX) {
        cursor_analog_x = cursor_x;
        cursor_x = cursor_digital_x;
      }
    } else if (posedge & 0x0040) { /* down */
      cursor_y++;
      if (cursor_y > CURSOR_ANALOG_Y_MAX) {
        cursor_y = 0;
        cursor_analog_x = cursor_x;
        cursor_x = cursor_digital_x;
      }
      if (cursor_y == CURSOR_ANALOG_Y_MIN) {
        cursor_digital_x = cursor_x;
        cursor_x = cursor_analog_x;
      }
    } else if (posedge & 0x0008) { /* left */
      cursor_x--;
      if (cursor_x < 0) {
        cursor_x = (cursor_y > CURSOR_DIGITAL_Y_MAX) ? CURSOR_ANALOG_X_MAX
                                                     : CURSOR_DIGITAL_X_MAX;
      }
    } else if (posedge & 0x0020) { /* right */
      int max = (cursor_y > CURSOR_DIGITAL_Y_MAX) ? CURSOR_ANALOG_X_MAX
                                                  : CURSOR_DIGITAL_X_MAX;
      cursor_x++;
      if (cursor_x > max) {
        cursor_x = 0;
      }
    } else if (posedge & 0x0001) { /* space */
      flip();
    } else if (posedge & 0x0100) { /* Q */
      if (dirty) {
        show_status("Save to persistent memory before quitting? [Y/N]");
        commit_or_quit = 1;
      } else {
        show_status("Quit");
        return -1;
      }
    } else if (posedge & 0x0080) { /* S */
      save();
    } else if (posedge & 0x0002) { /* L */
      label_type = (label_type + 1) %
                   (communication_mode == COMM_MODE_JOYPAD_MAPPER_Z ? 6 : 4);
      show_label(label_type);
    }
    show_cursor(1);
  }
  if (communication_mode != COMM_MODE_MOONSHOT_SLOW) {
    update();
  }
  fflush(stdout);
  return current_bitsns;
}

void teardown(void) {
  printf("\033[%d;%dH", 31, 1);
  printf("\033[>5l");
  _dos_kflushio(0);
}

int main(void) {
  int ssp = _iocs_b_super(0);

  int result = setup();
  if (!result) {
    int bitsns = 0;
    do {
      bitsns = loop(bitsns);
    } while (bitsns >= 0);
    teardown();
  }

  if (ssp) {
    _iocs_b_super(ssp);
  }
  return result;
}
