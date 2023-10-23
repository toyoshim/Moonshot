// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "ch559.h"
#include "hid.h"
#include "serial.h"

#include "atari.h"
#include "controller.h"
#include "settings.h"

#define VER "0.96"

static struct settings* settings = 0;

static void detected(void) {
  led_oneshot(L_PULSE_ONCE);
}

static uint8_t get_flags(void) {
  return USE_HUB0 | USE_HUB1;
}

void main(void) {
  initialize();

  settings_init();
  settings = settings_get();

  atari_init();

  struct hid hid;
  hid.report = controller_update;
  hid.detected = detected;
  hid.get_flags = get_flags;
  hid_init(&hid);

  Serial.printf("MP17-Moonshot ver %s\n", VER);

  for (;;) {
    hid_poll();
    settings_poll();
    controller_poll();
    atari_poll();
  }
}
