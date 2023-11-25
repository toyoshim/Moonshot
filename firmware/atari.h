// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __atari_h__
#define __atari_h__

#include <stdbool.h>

#include "grove.h"
#include "interrupt.h"

void atari_init(void);
void atari_poll(void);

#endif  // __atari_h__