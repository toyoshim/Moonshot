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

// #define PROTO1

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

#ifndef PROTO1
static uint8_t swap[16] = {
    0x00,  // xxxx0000 => 00xxxx00
    0x80,  // xxxx0001 => 10xxxx00
    0x02,  // xxxx0010 => 00xxxx10
    0x82,  // xxxx0011 => 10xxxx10
    0x01,  // xxxx0100 => 00xxxx01
    0x81,  // xxxx0101 => 10xxxx01
    0x03,  // xxxx0110 => 00xxxx11
    0x83,  // xxxx0111 => 10xxxx11
    0x40,  // xxxx1000 => 01xxxx00
    0xc0,  // xxxx1001 => 11xxxx00
    0x42,  // xxxx1010 => 01xxxx10
    0xc2,  // xxxx1011 => 11xxxx10
    0x41,  // xxxx1100 => 01xxxx01
    0xc1,  // xxxx1101 => 11xxxx01
    0x43,  // xxxx1110 => 01xxxx11
    0xc3,  // xxxx1111 => 11xxxx11
};
#endif

static void wait(uint16_t count) {
  for (uint16_t i = 0; i < count; ++i) {
    ++nop;
#ifdef PROTO1
    if (P2_6) {
#else
    if (P1_4) {
#endif
      ++nop;
    }
  }
}

void gpio_int(void) __interrupt(INT_NO_GPIO) __using(0) {
#ifdef PROTO1
  if (!P2_6) {
    uint16_t count;
    for (count = 0; count != 256; ++count) {
      ++nop;
      if (P2_6) {
        break;
      }
    }
    if (count == 256) {
      // Disable the GPIO interrupt once to permit timer interrupts, etc.
      IE_GPIO = 0;
      return;
    }
    if (count < 7) {
      count = 7;
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
#else
  if (!P1_4) {
    uint16_t count;
    for (count = 0; count != 256; ++count) {
      ++nop;
      if (P1_4) {
        break;
      }
    }
    if (count == 256) {
      // Disable the GPIO interrupt once to permit timer interrupts, etc.
      IE_GPIO = 0;
      return;
    }
    if (count < 7) {
      count = 7;
    }
    for (uint8_t n = 0; n < 5; ++n) {
      wait(count);
    }
    for (uint8_t n = 0; n < 6; ++n) {
      uint8_t v = swap[out[n] >> 4];
      P4_OUT = v;
      P3 = 0x20 | v;
      wait(count);
      P3_5 = 0;
      wait(count << 1);
      P3_5 = 1;

      v = swap[out[n] & 0x0f];
      P4_OUT = v;
      P3 = 0x30 | v;
      wait(count);
      P3_5 = 0;
      wait(count << 1);
      P3_5 = 1;
    }
    wait(count);
    P3_4 = 0;
  }
#endif
}

void atari_init(void) {
  for (uint8_t bit = 0; bit < 6; ++bit) {
    pinMode(2, bit, OUTPUT);
    digitalWrite(2, bit, HIGH);
  }

#ifdef PROTO1
  uart1_init(UART1_P2, UART1_115200);
#else
  pinMode(1, 4, INPUT);  // COM

  pinMode(3, 4, OUTPUT);  // B3
  pinMode(3, 5, OUTPUT);  // START
  pinMode(3, 6, OUTPUT);  // SELECT
  pinMode(3, 7, OUTPUT);  // B6
  digitalWrite(3, 4, HIGH);
  digitalWrite(3, 5, HIGH);
  digitalWrite(3, 6, HIGH);
  digitalWrite(3, 7, HIGH);

  pinMode(4, 0, OUTPUT);  // B4
  pinMode(4, 1, OUTPUT);  // B5
  digitalWrite(4, 0, HIGH);
  digitalWrite(4, 1, HIGH);

  pinMode(4, 2, OUTPUT);  // /OE
  digitalWrite(4, 2, LOW);
#endif

  frame_timer = timer3_tick_msec();
}

void atari_poll(void) {
  if (mode == MODE_NORMAL) {
#ifdef PROTO1
    // ______0__1_
    // P2_0: U  6
    // P2_1: D  5
    // P2_2: L  4
    // P2_3: R Select
    // P2_4: 1  3
    // P2_5: 2 Start

    uint8_t buttons = controller_data(0, 0, 0);
    out[0] = ((buttons & 0x20) ? 0 : 0x01) | ((buttons & 0x10) ? 0 : 0x02) |
             ((buttons & 0x08) ? 0 : 0x04) | ((buttons & 0x04) ? 0 : 0x08) |
             ((buttons & 0x02) ? 0 : 0x10) | ((buttons & 0x01) ? 0 : 0x20);
    out[1] = ((buttons & 0x40) ? 0 : 0x08) | ((buttons & 0x80) ? 0 : 0x20);
    if (buttons & 0x40) {
      out[0] &= 0xfc;  // SELECT == U + D
    }
    if (buttons & 0x80) {
      out[0] &= 0xf3;  // START == L + R
    }

    buttons = controller_data(0, 1, 0);
    out[1] |= ((buttons & 0x10) ? 0 : 0x01) | ((buttons & 0x20) ? 0 : 0x02) |
              ((buttons & 0x40) ? 0 : 0x04) | ((buttons & 0x80) ? 0 : 0x10);
    P2 = out[0];
#else
    // P2_0: D  D
    // P2_1: U  U
    // P2_2: L  L
    // P2_3: R  R
    // P2_4: B2 B2
    // P2_5: B1 B1
    // P3_4: B1 B3
    // P3_5: B2 START
    // P3_6: R  SELECT
    // P3_7: U  B6
    // P4_0: L  B4
    // P4_1: D  B5
    uint8_t buttons = controller_data(0, 0, 0);
    out[0] = ((buttons & 0x10) ? 0 : 0x01) | ((buttons & 0x20) ? 0 : 0x02) |
             ((buttons & 0x08) ? 0 : 0x04) | ((buttons & 0x04) ? 0 : 0x08) |
             ((buttons & 0x01) ? 0 : 0x10) | ((buttons & 0x02) ? 0 : 0x20);
    out[1] = ((buttons & 0x40) ? 0 : 0x40) | ((buttons & 0x80) ? 0 : 0x20);
    if (buttons & 0x40) {
      out[0] &= 0xfc;  // SELECT == U + D
    }
    if (buttons & 0x80) {
      out[0] &= 0xf3;  // START == L + R
    }

    buttons = controller_data(0, 1, 0);
    out[1] |= ((buttons & 0x40) ? 0 : 0x01) | ((buttons & 0x20) ? 0 : 0x02) |
              ((buttons & 0x80) ? 0 : 0x10) | ((buttons & 0x10) ? 0 : 0x80);
    P2 = out[0];
    P3 = out[1];
    P4_OUT = out[1];
#endif
  } else if (mode == MODE_CYBER) {
    uint8_t d0 = ~controller_data(0, 0, 0);
    uint8_t d1 = ~controller_data(0, 1, 0);
    uint8_t a0 = controller_analog(0) >> 8;
    uint8_t a1 = controller_analog(1) >> 8;
    uint8_t a2 = controller_analog(2) >> 8;
    uint8_t a3 = controller_analog(3) >> 8;
    //  A|A', B|B', C, D, E1, E2, START SELECT
    out[0] = ((d0 << 6) & ((d1 & 0x0c) << 4)) | ((d1 & 0xf0) >> 2) | (d0 >> 6);
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
    // Serial.printf("%x %x %x %x %x%x\n", a0, a1, a2, a3, d0, d1);
  }

  if (mode == MODE_CYBER) {
    // Enable again.
    IE_GPIO = 1;
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
        settings_select(0);
        settings_led_mode(L_ON);
        gpio_enable_interrupt(0, true);
        break;
      case MODE_CYBER:
        settings_select(1);
        settings_led_mode(L_BLINK);
#ifdef PROTO1
        P2 = 0xff;
        P2_4 = 0;  // L/H
        P2_5 = 1;  // ACK
        gpio_enable_interrupt(bIE_RXD1_LO, true);
#else
        P2 = 0xff;
        P3 = 0xff;
        P4_OUT = 0xfb;  // /OE
        P2_5 = 0;       // L/H for Low
        P3_4 = 0;       // L/H for High
        P2_4 = 1;       // ACK for Low
        P3_5 = 1;       // ACK for High
        gpio_enable_interrupt(bIE_P1_4_LO, true);
#endif
        break;
    }
  }
  button_pressed = current_button_pressed;
}