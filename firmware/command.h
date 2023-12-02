// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __command_h__
#define __command_h__

#include <stdbool.h>
#include <stdint.h>

void command_execute(uint8_t command, uint8_t* result);
bool command_in_transaction(void);

#endif  // __command_h__