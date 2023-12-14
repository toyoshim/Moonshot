// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __doslib_h__
#define __doslib_h__

#include <emscripten/emscripten.h>
#include <string.h>

void _dos_kflushio(int mode) {}

EMSCRIPTEN_KEEPALIVE int setup(void);
EMSCRIPTEN_KEEPALIVE int loop(int);
EMSCRIPTEN_KEEPALIVE void teardown(void);

#endif  // __doslib_h__