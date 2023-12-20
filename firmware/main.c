// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "ch559.h"
#include "gpio.h"
#include "led.h"
#include "serial.h"
#include "timer3.h"
#include "usb/cdc_device.h"
#include "usb/hid/hid.h"
#include "usb/usb_device.h"

#include "atari.h"
#include "command.h"
#include "controller.h"
#include "settings.h"
#include "version.h"

// #define _DBG

static const char kString01Manufacturer[] = "Mellow PCB - mellow.twintail.org";
static const char kString02Product[] = "Moonshot";
static const char kString03SerialNumber[] = VERSION_STRING;

static uint8_t get_string_length(uint8_t no) {
  switch (no) {
    case 1:
      return sizeof(kString01Manufacturer) - 1;
    case 2:
      return sizeof(kString02Product) - 1;
    case 3:
      return sizeof(kString03SerialNumber) - 1;
  }
  return 0;
}

static const char* get_string(uint8_t no) {
  switch (no) {
    case 1:
      return kString01Manufacturer;
    case 2:
      return kString02Product;
    case 3:
      return kString03SerialNumber;
  }
  return 0;
}

void recv(const uint8_t* buffer, uint8_t len) {
  uint8_t response[4];
  for (uint8_t i = 0; i < len; ++i) {
    command_execute(buffer[i], response);
    cdc_device_send(response, 4);
  }
}

static void detected(void) {
  led_oneshot(L_PULSE_ONCE);
}

static uint8_t get_flags(void) {
  return USE_HUB0 | USE_HUB1;
}

void main(void) {
  bool device_mode = false;

  initialize();

  timer3_tick_init();

  struct cdc_device device;
  device.id_vendor = 0x0483;
  device.id_product = 0x16c0;
  device.bcd_device = 0x0100;
  device.get_string_length = get_string_length;
  device.get_string = get_string;
  device.recv = recv;
  cdc_device_init(&device);

  led_init(1, 5, LOW);
  led_mode(L_ON);

  settings_init();

  atari_init();

  Serial.print("\nMP17-Moonshot ver " VERSION_STRING " - ");

  while (timer3_tick_raw() < 0x3000) {
    uint8_t state = usb_device_state();
    led_poll();
    if (state != UD_STATE_IDLE) {
      device_mode = true;
      break;
    }
  }
#ifndef _DBG
  if (device_mode) {
    Serial.println("CDC mode");
    led_mode(L_FASTER_BLINK);
    for (;;) {
      led_poll();
    }
  }
#endif
  Serial.println("HOST mode");

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
        Serial.println(
            "\r\nUp Dw Lf Rt B1 B2 B3 B4 L1 R1 L2 R2 Sl St L3 R3 A0   A1   A2 "
            "  A3   A4   A5");
        init = true;
      }
      uint16_t v = controller_raw_digital(1);
      for (uint16_t b = 0x8000; b != 0; b >>= 1) {
        Serial.printf("%d  ", (v & b) ? 1 : 0);
      }
      for (uint8_t i = 0; i < 6; ++i) {
        uint16_t v = controller_raw_analog(1, i);
        Serial.printf("%x%x,", (v >> 8) & 0xff, v & 0xff);
      }
      Serial.printf("\r");
    }
#endif
  }
}
