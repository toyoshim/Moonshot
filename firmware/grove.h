// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __grove_h__
#define __grove_h__

#include "i2c.h"

void grove_init(void (*interrupt_handler)(void));
void grove_update_interrupt(uint8_t ie);

#endif  // __grove_h__
