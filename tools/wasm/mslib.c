// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "../mslib.h"

#include <emscripten.h>

EM_ASYNC_JS(int, ms_comm, (int len, unsigned char* cmd, unsigned char* res), {
  return await this.Module.ms_comm(len, cmd, res);
});

EM_JS(void, ms_set_timeout, (unsigned short timeout), {});

EM_JS(int, _iocs_bitsns, (int bitsns), { return this.Module.bitsns(bitsns); });