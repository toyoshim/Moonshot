// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "ch559.h"
#include "gpio.h"
#include "led.h"
#include "serial.h"
#include "usb/hid/hid.h"

#include "atari.h"
#include "controller.h"
#include "settings.h"
#include "version.h"

// #define _DBG

static struct settings* settings = 0;

static void detected(void) {
  led_oneshot(L_PULSE_ONCE);
}

static uint8_t get_flags(void) {
  return USE_HUB0 | USE_HUB1;
}

void main(void) {
  initialize();

  led_init(1, 5, LOW);
  Serial.printf("MP17-Moonshot ver " VERSION_STRING);

  if (!settings_init()) {
    led_mode(L_BLINK_THREE_TIMES);
    for (;;) {
      led_poll();
    }
  }
  led_mode(L_ON);
  settings = settings_get();

  atari_init();

  struct hid hid;
  hid.report = controller_update;
  hid.detected = detected;
  hid.get_flags = get_flags;
  hid_init(&hid);

  for (;;) {
    hid_poll();
    led_poll();
    atari_poll();

#ifdef _DBG
    {
      static bool init = false;
      if (!init) {
        Serial.printf(
            "Ts T1 T2 T3 -- -- -- -- St Sv Up Dw Lf Rt B1 B2 B3 B4 B5 B6 B7 "
            "B8 B9 B10 A0  A1  A2  A3  A4  A5");
        Serial.printf("\n");
        init = true;
      }
      for (uint8_t i = 0; i < 2; ++i) {
        uint8_t v = controller_data(0, i);
        for (uint8_t b = 0x80; b != 0; b >>= 1) {
          Serial.printf("%d  ", (v & b) ? 1 : 0);
        }
      }
      for (uint8_t i = 0; i < 6; ++i) {
        uint16_t v = controller_analog(i);
        Serial.printf("%x%x,", (v >> 8) & 0xff, v & 0xff);
      }
      Serial.printf("\r");
    }
#endif
  }
}
