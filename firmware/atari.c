// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "atari.h"

#include <stdbool.h>
#include <stdint.h>

#include "gpio.h"
#include "io.h"
#include "led.h"
#include "serial.h"
#include "timer3.h"
#include "uart1.h"

#include "controller.h"
#include "settings.h"

enum {
  MODE_NORMAL = 0,  // Normal 2 buttons, or Capcom 6 buttons mode
  MODE_CYBER = 1,   // Cyber Stick mode
  MODE_LAST = MODE_CYBER,
};

static volatile uint8_t out[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uint16_t frame_timer = 0;
static bool button_pressed = false;
static uint8_t mode = MODE_NORMAL;
static volatile uint8_t nop = 0;

static void wait(uint16_t count) {
  for (uint16_t i = 0; i < count; ++i) {
    ++nop;
    if (P2_6) {
      ++nop;
    }
  }
}

void gpio_int(void) __interrupt(INT_NO_GPIO) __using(0) {
  // For proto.1
  if (!P2_6) {
    uint16_t count;
    for (count = 0; count != 65535; ++count) {
      ++nop;
      if (P2_6) {
        break;
      }
    }
    for (uint8_t n = 0; n < 5; ++n) {
      wait(count);
    }
    for (uint8_t n = 0; n < 6; ++n) {
      P2 = 0x20 | (out[n] >> 4);
      wait(count);
      P2_5 = 0;
      wait(count << 1);
      P2_5 = 1;

      P2 = 0x30 | (out[n] & 0x0f);
      wait(count);
      P2_5 = 0;
      wait(count << 1);
      P2_5 = 1;
    }
    wait(count);
    P2_4 = 0;
  }
  settings_poll();
}

void atari_init(void) {
  for (uint8_t bit = 0; bit < 6; ++bit) {
    pinMode(2, bit, OUTPUT);
    digitalWrite(2, bit, HIGH);
  }

  // For proto.1
  uart1_init(UART1_P2, UART1_115200);

  frame_timer = timer3_tick_msec();
}

void atari_poll(void) {
  // ______0__1_
  // P2_0: U  6
  // P2_1: D  5
  // P2_2: L  4
  // P2_3: R Select
  // P2_4: 1  3
  // P2_5: 2 Start
  if (mode == MODE_NORMAL) {
    uint8_t buttons = controller_data(0, 0, 0);
    out[0] = ((buttons & 0x20) ? 0 : 0x01) | ((buttons & 0x10) ? 0 : 0x02) |
             ((buttons & 0x08) ? 0 : 0x04) | ((buttons & 0x04) ? 0 : 0x08) |
             ((buttons & 0x02) ? 0 : 0x10) | ((buttons & 0x01) ? 0 : 0x20);
    out[1] = ((buttons & 0x40) ? 0 : 0x08) | ((buttons & 0x80) ? 0 : 0x20);

    buttons = controller_data(0, 1, 0);
    out[1] |= ((buttons & 0x10) ? 0 : 0x01) | ((buttons & 0x20) ? 0 : 0x02) |
              ((buttons & 0x40) ? 0 : 0x04) | ((buttons & 0x80) ? 0 : 0x10);
    P2 = out[0];
  } else if (mode == MODE_CYBER) {
    uint8_t d0 = ~controller_data(0, 0, 0);
    uint8_t d1 = ~controller_data(0, 1, 0);
    uint8_t a0 = controller_analog(0) >> 8;
    uint8_t a1 = controller_analog(1) >> 8;
    uint8_t a2 = controller_analog(2) >> 8;
    uint8_t a3 = controller_analog(3) >> 8;
    // A|A', B|B', C, D, E1, E2, START SELECT
    out[0] = (d0 << 6) | ((d1 & 0x0c) << 4) | ((d1 & 0xf0) >> 2) | (d0 >> 6);
    // Ch.0 high, Ch.1 high
    out[1] = (a0 & 0xf0) | ((a1 & 0xf0) >> 4);
    // Ch.2 high, Ch.3 high
    out[2] = (a2 & 0xf0) | ((a3 & 0xf0) >> 4);
    // Ch.0 low, Ch.1 low
    out[3] = (a0 << 4) | (a1 & 0x0f);
    // Ch.2 low, Ch.3 low
    out[4] = (a2 << 4) | (a3 & 0x0f);
    // A, B, A', B7, -, -, -, -
    out[5] = (d0 << 6) | ((d1 & 0x0c) << 2) | 0x0f;
  }

  if (timer3_tick_msec_between(frame_timer, frame_timer + 16)) {
    return;
  }
  frame_timer = timer3_tick_msec();
  settings_rapid_sync();

  bool current_button_pressed = settings_service_pressed();
  if (button_pressed & !current_button_pressed) {
    if (mode == MODE_LAST) {
      mode = MODE_NORMAL;
    } else {
      mode++;
    }
    switch (mode) {
      case MODE_NORMAL:
        settings_led_mode(L_ON);
        gpio_enable_interrupt(0, true);
        break;
      case MODE_CYBER:
        settings_led_mode(L_BLINK);
        // For proto.1
        P2 = 0xff;
        P2_4 = 0;  // L/H
        P2_5 = 1;  // ACK
        gpio_enable_interrupt(bIE_RXD1_LO, true);
        break;
    }
  }
  button_pressed = current_button_pressed;
}