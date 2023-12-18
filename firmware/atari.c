// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "atari.h"

#include <stdbool.h>
#include <stdint.h>

#include "chlib/src/io.h"
#include "chlib/src/timer3.h"
#include "gpio.h"
#include "grove.h"
#include "io.h"
#include "led.h"
#include "serial.h"
#include "timer3.h"
#include "uart1.h"

#include "command.h"
#include "controller.h"
#include "settings.h"

#define PROTO1

enum {
  MODE_NORMAL = 0,  // Normal 2 buttons, or Capcom 6 buttons mode
  MODE_CYBER = 1,   // Cyber Stick mode
  MODE_SAFE = 2,    // Safe mode
  MODE_MD = 3,      // Mega Drive 3B / 6B
#ifdef PROTO1
  MODE_LAST = MODE_SAFE,
#else
  MODE_LAST = MODE_MD,
#endif
};

#define MD_STATE_TIMEOUT 2048
#define MD_STATE_SHORT_TIMEOUT 64

static volatile uint8_t out[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uint16_t frame_timer = 0;
static bool button_pressed = false;
static volatile uint8_t mode = MODE_NORMAL;
static volatile uint8_t nop = 0;  // used to avoid compiler optimization

// 150 for cyberstick mode, 0x7fff for safe mode.
static volatile uint16_t cyber_timeout = 150;
// 10 for cyberstick mode, 0x7000 for safe mode.
static volatile uint16_t cyber_minimum = 10;

#ifdef PROTO1

#define GPIO_COM P2_6

#define SET_LOW_CYCLE_SIGNALS(v) P2 = 0x20 | (v)
#define SET_HIGH_CYCLE_SIGNALS(v) P2 = 0x30 | (v)
#define SET_READY() P2_5 = 0
#define RESET_READY() P2_5 = 1

#else

#define GPIO_COM P1_4

#define SET_LOW_CYCLE_SIGNALS(v)       \
  {                                    \
    uint8_t adjusted_value = swaph[v]; \
    P4_OUT = adjusted_value;           \
    P3 = 0x20 | adjusted_value;        \
    P2 = 0x10 | swapl[v];              \
  }
#define SET_HIGH_CYCLE_SIGNALS(v)      \
  {                                    \
    uint8_t adjusted_value = swaph[v]; \
    P4_OUT = adjusted_value;           \
    P3 = 0x30 | adjusted_value;        \
    P2 = 0x30 | swapl[v];              \
  }
#define SET_READY() \
  {                 \
    P2_4 = 0;       \
    P3_5 = 0;       \
  }

#define RESET_READY() \
  {                   \
    P2_4 = 1;         \
    P3_5 = 1;         \
  }

static uint8_t swaph[16] = {
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
static uint8_t swapl[16] = {
    0x00,  // xxxx0000 => xxxx0000
    0x02,  // xxxx0001 => xxxx0010
    0x01,  // xxxx0010 => xxxx0001
    0x03,  // xxxx0011 => xxxx0011
    0x04,  // xxxx0100 => xxxx0100
    0x06,  // xxxx0101 => xxxx0110
    0x05,  // xxxx0110 => xxxx0101
    0x07,  // xxxx0111 => xxxx0111
    0x08,  // xxxx1000 => xxxx1000
    0x0a,  // xxxx1001 => xxxx1010
    0x09,  // xxxx1010 => xxxx1001
    0x0b,  // xxxx1011 => xxxx1011
    0x0c,  // xxxx1100 => xxxx1100
    0x0e,  // xxxx1101 => xxxx1110
    0x0d,  // xxxx1110 => xxxx1101
    0x0f,  // xxxx1111 => xxxx1111
};
#endif

static void wait(uint16_t count) {
  for (uint16_t i = 0; i < count; ++i) {
    ++nop;
    if (GPIO_COM) {
      ++nop;
    }
  }
}

static bool wait_posedge(uint16_t timeout) {
  uint16_t count;
  for (count = 0; count != timeout; ++count) {
    ++nop;
    if (GPIO_COM) {
      break;
    }
  }
  return count != timeout;
}

static bool wait_negedge(uint16_t timeout) {
  uint16_t count;
  for (count = 0; count != timeout; ++count) {
    ++nop;
    if (!GPIO_COM) {
      break;
    }
  }
  return count != timeout;
}

static void gpio_int(void) {
  if (mode == MODE_MD) {
    if (!GPIO_COM) {
      return;
    }
    // The fist posedge.
    if (!wait_negedge(MD_STATE_TIMEOUT)) {
      goto MD_TIMEOUT;
    }
    if (!wait_posedge(MD_STATE_TIMEOUT)) {
      goto MD_TIMEOUT;
    }
    // The second posedge. Let's prepare for the next special low cycle there
    // 3:0 goes low.
    P2 = out[0] & 0xf0;
    if (!wait_negedge(MD_STATE_TIMEOUT)) {
      goto MD_TIMEOUT;
    }
    // Prepare for the next high cycle for X, Y, Z, and Mode.
    P3 = out[2];
    P4_OUT = out[2];
    if (!wait_posedge(MD_STATE_TIMEOUT)) {
      goto MD_TIMEOUT;
    }
    // Prepare for the next low cycle, the final state.
    P2 = out[0];
    wait_negedge(MD_STATE_SHORT_TIMEOUT);

  MD_TIMEOUT:
    // Reset for the initial idle state anyway.
    P2 = out[0];
    P3 = out[1];
    P4_OUT = out[1];
    return;
  } else if (mode == MODE_CYBER || mode == MODE_SAFE) {
    if (GPIO_COM) {
      return;
    }
    // The real cyberstick seems to have 4 speed mode, 25us, 50us, 75us, and
    // 100us per half a cycle, and decides one of them based on the request
    // pulse width. Also, if the request has negated over 100us, it starts
    // sending back data in the fastest mode, 25us.
    // The following magic number cyber_minimum and cyber_timeout practically
    // works in the range, 25us through to 100us in the cyberstick mode.
    // For the safe mode, they are set to longer values so that it can work on
    // slow communication cases such as XM6 over U-kun, or X68000Z.
    uint16_t count;
    for (count = 0; count != cyber_timeout; ++count) {
      ++nop;
      if (GPIO_COM) {
        break;
      }
    }
    if (count == cyber_timeout) {
      // Disable the GPIO interrupt once to permit timer interrupts, etc for the
      // timeout case as there is a possible case the signal is stuck at low.
      IE_GPIO = 0;
      count = cyber_minimum;
    } else {
      if (count < cyber_minimum) {
        count = cyber_minimum;
      }
      for (uint8_t n = 0; n < 5; ++n) {
        wait(count);
      }
    }
    uint16_t command = 0;
    uint8_t n;
    for (n = 0; n < 6; ++n) {
      command <<= 1;
      command |= GPIO_COM ? 1 : 0;
      SET_LOW_CYCLE_SIGNALS(out[n] >> 4);
      wait(count);
      SET_READY();
      wait(count << 1);
      RESET_READY();

      command <<= 1;
      command |= GPIO_COM ? 1 : 0;
      SET_HIGH_CYCLE_SIGNALS(out[n] & 0x0f);
      wait(count);
      SET_READY();
      wait(count << 1);
      RESET_READY();
    }
    if ((command >> 8) == 0x0a) {
      uint8_t data = command;
      while (command_in_transaction() || data != 0xff) {
        uint8_t result[4];
        command_execute(data, result);
        for (n = 0; n < 4; ++n) {
          data <<= 1;
          data |= GPIO_COM ? 1 : 0;
          SET_LOW_CYCLE_SIGNALS(result[n] >> 4);
          wait(count);
          SET_READY();
          wait(count << 1);
          RESET_READY();

          data <<= 1;
          data |= GPIO_COM ? 1 : 0;
          SET_HIGH_CYCLE_SIGNALS(result[n] & 0x0f);
          wait(count);
          SET_READY();
          wait(count << 1);
          RESET_READY();
        }
      }
    }
    wait(count);
    SET_LOW_CYCLE_SIGNALS(0x0f);
  }
}

static void set_mode(uint8_t new_mode) {
  mode = new_mode;
  switch (mode) {
    case MODE_NORMAL:
      led_mode(L_ON);
      grove_update_interrupt(0);
      break;
    case MODE_CYBER:
      led_mode(L_FAST_BLINK);
      cyber_timeout = 150;
      cyber_minimum = 10;
      SET_LOW_CYCLE_SIGNALS(0x0f);
      RESET_READY();
#ifdef PROTO1
      grove_update_interrupt(bIE_RXD1_LO);
#else
      P2 = 0xdf;
      grove_update_interrupt(bIE_P1_4_LO);
#endif
      break;
    case MODE_SAFE:
      led_mode(L_BLINK);
      cyber_timeout = 0x7fff;
      cyber_minimum = 0x7000;
      SET_LOW_CYCLE_SIGNALS(0x0f);
      RESET_READY();
#ifdef PROTO1
      grove_update_interrupt(bIE_RXD1_LO);
#else
      P2 = 0xdf;
      grove_update_interrupt(bIE_P1_4_LO);
#endif
      break;
    case MODE_MD:
      // PROTO1 doesn't enter this code path.
      led_mode(L_BLINK_TWICE);
      grove_update_interrupt(bIE_P5_7_HI);
      break;
  }
}

void atari_init(void) {
  pinMode(4, 6, INPUT_PULLUP);
  for (uint8_t bit = 0; bit < 6; ++bit) {
    pinMode(2, bit, OUTPUT);
    digitalWrite(2, bit, HIGH);
  }

#ifdef PROTO1
  uart1_init(UART1_P2, UART1_115200);
#else
  pinMode(1, 4, INPUT_PULLUP);  // COM

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

  grove_init(gpio_int);
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

    uint16_t buttons = controller_digital(0);
    uint8_t h = buttons >> 8;
    uint8_t l = buttons;
    out[0] = ((h & 0x20) ? 0 : 0x01) | ((h & 0x10) ? 0 : 0x02) |
             ((h & 0x08) ? 0 : 0x04) | ((h & 0x04) ? 0 : 0x08) |
             ((h & 0x02) ? 0 : 0x10) | ((h & 0x01) ? 0 : 0x20);
    out[1] = ((h & 0x40) ? 0 : 0x08) | ((h & 0x80) ? 0 : 0x20) |
             ((l & 0x10) ? 0 : 0x01) | ((l & 0x20) ? 0 : 0x02) |
             ((l & 0x40) ? 0 : 0x04) | ((l & 0x80) ? 0 : 0x10);

    if (h & 0x40) {
      out[0] &= 0xfc;  // SELECT == U + D
    }
    if (h & 0x80) {
      out[0] &= 0xf3;  // START == L + R
    }
    P2 = out[0];
#else
    // P2_0: D  D      Bit 1
    // P2_1: U  U      Bit 0
    // P2_2: L  L      Bit 2
    // P2_3: R  R      Bit 3
    // P2_4: B2 B2
    // P2_5: B1 B1
    // P3_4: B1 B3
    // P3_5: B2 START
    // P3_6: R  SELECT
    // P3_7: U  B6
    // P4_0: L  B4
    // P4_1: D  B5
    uint16_t buttons = controller_digital(0);
    uint8_t h = buttons >> 8;
    uint8_t l = buttons;
    out[0] = ((h & 0x10) ? 0 : 0x01) | ((h & 0x20) ? 0 : 0x02) |
             ((h & 0x08) ? 0 : 0x04) | ((h & 0x04) ? 0 : 0x08) |
             ((h & 0x01) ? 0 : 0x10) | ((h & 0x02) ? 0 : 0x20);
    out[1] = ((h & 0x40) ? 0 : 0x40) | ((h & 0x80) ? 0 : 0x20) |
             ((l & 0x40) ? 0 : 0x01) | ((l & 0x20) ? 0 : 0x02) |
             ((l & 0x80) ? 0 : 0x10) | ((l & 0x10) ? 0 : 0x80);

    if (h & 0x40) {
      out[0] &= 0xfc;  // SELECT == U + D
    }
    if (h & 0x80) {
      out[0] &= 0xf3;  // START == L + R
    }

    P2 = out[0];
    P3 = out[1];
    P4_OUT = out[1];
#endif
  } else if (mode == MODE_CYBER || mode == MODE_SAFE) {
    uint16_t d = ~controller_digital(0);
    uint8_t d0 = d >> 8;
    uint8_t d1 = d;
    uint8_t a0 = controller_analog(0, 0) >> 8;
    uint8_t a1 = controller_analog(0, 1) >> 8;
    uint8_t a2 = controller_analog(0, 2) >> 8;
    uint8_t a3 = controller_analog(0, 3) >> 8;
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
    // A, B, A', B', -, -, -, -
    out[5] = (d0 << 6) | ((d1 & 0x0c) << 2) | 0x0f;
    // Serial.printf("%x %x %x %x %x%x\n", a0, a1, a2, a3, d0, d1);
  } else if (mode == MODE_MD) {
    // PROTO1 doesn't enter this code path.
    // P2_0: D  D
    // P2_1: U  U
    // P2_2: L  0
    // P2_3: R  0
    // P2_4: B2 Start
    // P2_5: B1 B1
    // P3_4: B1 B2     B2
    // P3_5: B2 B3     B3
    // P3_6: R  R      Select
    // P3_7: U  U      B6
    // P4_0: L  L      B4
    // P4_1: D  D      B5
    uint16_t buttons = controller_digital(0);
    uint8_t h = buttons >> 8;
    uint8_t l = buttons;
    // D | U | Start | B1
    out[0] = ((h & 0x10) ? 0 : 0x01) | ((h & 0x20) ? 0 : 0x02) |
             ((h & 0x80) ? 0 : 0x10) | ((h & 0x02) ? 0 : 0x20);
    // L | D | B2 | R | U | B3
    out[1] = ((h & 0x08) ? 0 : 0x01) | ((h & 0x10) ? 0 : 0x02) |
             ((h & 0x01) ? 0 : 0x10) | ((h & 0x04) ? 0 : 0x40) |
             ((h & 0x20) ? 0 : 0x80) | ((l & 0x80) ? 0 : 0x20);

    // B2 | Select | B3 | B4 | B5 | B6
    out[2] = ((h & 0x01) ? 0 : 0x10) | ((h & 0x40) ? 0 : 0x40) |
             ((l & 0x80) ? 0 : 0x20) | ((l & 0x40) ? 0 : 0x01) |
             ((l & 0x20) ? 0 : 0x02) | ((l & 0x10) ? 0 : 0x80);
    P2 = out[0];
    P3 = out[1];
    P4_OUT = out[1];
  }

  if (mode != MODE_NORMAL) {
    // Enable again.
    IE_GPIO = 1;
  }
  if (timer3_tick_msec_between(frame_timer, frame_timer + 16)) {
    return;
  }
  frame_timer = timer3_tick_msec();
  settings_rapid_sync();

  bool current_button_pressed = digitalRead(4, 6) == LOW;
  if (button_pressed & !current_button_pressed) {
    uint8_t new_mode = mode++;
    if (new_mode > MODE_LAST) {
      new_mode = MODE_NORMAL;
    }
    set_mode(new_mode);
  }
  button_pressed = current_button_pressed;
}