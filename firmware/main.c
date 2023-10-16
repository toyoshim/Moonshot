// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "ch559.h"
#include "hid.h"
#include "led.h"
#include "serial.h"

#include "controller.h"
#include "settings.h"

#define VER "0.90"

static struct settings* settings = 0;
static uint8_t button_masks[7] = {
    0x20,  // Up
    0x10,  // Down
    0x08,  // Left
    0x04,  // Right
    0x02,  // B1
    0x01,  // B2
    0x00,  // COM
};

static void detected(void) {
  led_oneshot(L_PULSE_ONCE);
}

static uint8_t get_flags(void) {
  return USE_HUB1;
}

void main(void) {
  initialize();

  for (uint8_t bit = 0; bit < 7; ++bit) {
    pinMode(2, bit, INPUT_PULLUP);
    digitalWrite(2, bit, LOW);
  }

  settings_init();
  settings = settings_get();

  delay(30);

  struct hid hid;
  hid.report = controller_update;
  hid.detected = detected;
  hid.get_flags = get_flags;
  hid_init(&hid);

  Serial.printf("Joyner ver %s\n", VER);

  for (;;) {
    hid_poll();
    settings_poll();
    controller_poll();

    uint8_t buttons = controller_data(1, 0, 0);
    for (uint8_t bit = 0; bit < 7; ++bit) {
      pinMode(2, bit, (buttons & button_masks[bit]) ? OUTPUT : INPUT_PULLUP);
    }
  }
}
