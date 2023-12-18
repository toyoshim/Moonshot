// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __atari_h__
#define __atari_h__

#include <stdbool.h>
#include <stdint.h>

#include "grove.h"
#include "interrupt.h"

void atari_init(void);
void atari_poll(void);
void atari_set_mode(uint8_t mode);

#endif  // __atari_h__