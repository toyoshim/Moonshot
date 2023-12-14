// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

export class MsConf {
  imports = {
    env: {
      ms_comm: null,
      ms_get_version: null,
      ms_load_config: null,
      ms_save_config: null,
      _iocs_bitsns: null,
      emscripten_memcpy_js: null
    },
    wasi_snapshot_preview1: {
      fd_write: null,
      fd_seek: null,
      fd_close: null
    }
  };

  keyinfo = {
    ArrowUp: { group: 7, bit: 0x10 },
    ArrowDown: { group: 7, bit: 0x40 },
    ArrowLeft: { group: 7, bit: 0x08 },
    ArrowRight: { group: 7, bit: 0x20 },
    Space: { group: 6, bit: 0x20 },
    KeyS: { group: 3, bit: 0x80 },
  };

  u8 = null;
  u32 = null;
  communication_buffer_address = 0;
  wasm = null;
  exports = null;
  intf = {};
  console = null;
  bitsns = null;
  lastsns = 0;

  constructor(console) {
    this.console = console;
    window.addEventListener('keydown', this.keydown.bind(this), true);
    window.addEventListener('keyup', this.keyup.bind(this), true);

    this.bitsns = new Array(16);
    for (let i = 0; i < 16; ++i) {
      this.bitsns[i] = 0;
    }
    this.imports.env.ms_comm = this.ms_comm.bind(this);
    this.imports.env.ms_get_version = this.ms_get_version.bind(this);
    this.imports.env.ms_load_config = this.ms_load_config.bind(this);
    this.imports.env.ms_save_config = this.ms_save_config.bind(this);
    this.imports.env._iocs_bitsns = this._iocs_bitsns.bind(this);
    this.imports.env.emscripten_memcpy_js = this.emscripten_memcpy_js.bind(this);
    this.imports.wasi_snapshot_preview1.fd_write = this.fd_write.bind(this);
    this.imports.wasi_snapshot_preview1.fd_seek = this.fd_seek.bind(this);
    this.imports.wasi_snapshot_preview1.fd_close = this.fd_close.bind(this);
  }

  async run() {
    WebAssembly.instantiateStreaming(fetch('msconf.wasm'), this.imports).then(obj => {
      this.wasm = obj;
      const memory = obj.instance.exports.memory.buffer;
      this.u8 = new Uint8Array(memory);
      this.u32 = new Uint32Array(memory);
      this.exports = obj.instance.exports;
      obj.instance.exports.setup();
      this.loop();
    });
  }

  loop() {
    this.lastsns = this.wasm.instance.exports.loop(this.lastsns);
    requestAnimationFrame(this.loop.bind(this));
  }

  ms_comm(len, cmd_ptr, res_ptr) {
    return 0;
  }

  ms_get_version(major_ptr, minor_ptr, protocol_ptr) {
    console.log('ms_get_version');
    this.u8[major_ptr] = 0x00;
    this.u8[minor_ptr] = 0x00;
    this.u8[protocol_ptr] = 0x00;
    return 0;
  }

  ms_load_config(config_ptr) {
    console.log('ms_load_config');
    return 0;
  }

  ms_save_config(config_ptr) {
    console.log('ms_save_config');
    return 1;
  }

  _iocs_bitsns(group) {
    return this.bitsns[group];
  }

  emscripten_memcpy_js(dst_ptr, src_ptr, len) {
    console.log('emscripten_memcpy_js');
  }

  fd_close(fd) {
    console.log('fd_close');
    return 0;
  }

  fd_write(fd, iov, iovcnt, pnum) {
    let n = 0;
    const chars = [];
    for (let i = 0; i < iovcnt; ++i) {
      const ptr = this.u32[(iov >> 2) + i * 2 + 0];
      const len = this.u32[(iov >> 2) + i * 2 + 1];
      for (let j = 0; j < len; ++j) {
        chars.push(String.fromCharCode(this.u8[ptr + j]));
      }
      n += len;
    }
    this.u32[pnum >> 2] = n;
    this.console.print(fd, chars.join(''));
    return 0;
  }

  fd_seek(fd, offset_low, offset_high, when, new_offset_ptr) {
    console.log('fd_seek');
    return 0;
  }

  keydown(e) {
    const info = this.keyinfo[e.code];
    if (info) {
      this.bitsns[info.group] |= info.bit;
    }
    e.preventDefault();
  }

  keyup(e) {
    const info = this.keyinfo[e.code];
    if (info) {
      this.bitsns[info.group] &= ~info.bit;
    }
    e.preventDefault();
  }
}