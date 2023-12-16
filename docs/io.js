// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

class IO {
  keyinfo = {
    ArrowUp: { group: 7, bit: 0x10 },
    ArrowDown: { group: 7, bit: 0x40 },
    ArrowLeft: { group: 7, bit: 0x08 },
    ArrowRight: { group: 7, bit: 0x20 },
    Space: { group: 6, bit: 0x20 },
    KeyS: { group: 3, bit: 0x80 },
  };

  console = null;
  bitsns = null;
  port = null;
  demo = false;
  demo_transfer = 0;
  demo_transfer_length = 0;

  constructor(console) {
    this.console = console;
    window.addEventListener('keydown', this.keydown.bind(this), true);
    window.addEventListener('keyup', this.keyup.bind(this), true);

    this.bitsns = new Array(16);
    for (let i = 0; i < 16; ++i) {
      this.bitsns[i] = 0;
    }
  }

  connected() {
    return this.demo || this.port != null;
  }

  async connect() {
    try {
      this.port = await navigator.serial.requestPort({
        filters: [{ usbVendorId: 0x0483, usbProductId: 0x16c0 }]
      });
      await this.port.open({ baudRate: 115200 });
    } catch (e) {
      console.log(e);
    }
  }

  async comm(command) {
    if (this.demo) {
      const data = new Uint8Array(4);
      if (this.demo_transfer) {
        switch (this.demo_transfer) {
          case 0xf3:
            this.demo_transfer_length = (command + 4) >> 2;
          case 0xf0:
          case 0xf1:
          case 0xf2:
            data[0] = command;
            data[1] = command;
            data[2] = command;
            data[3] = command;
            break;
          case 0xf5:
            data[0] = 0;
            data[1] = 0;
            data[2] = 0;
            data[3] = 0;
            break;
        }
        if (this.demo_transfer == 0xf5) {
          this.demo_transfer_length--;
          if (this.demo_transfer_length < 0) {
            this.demo_transfer = 0;
          }
        } else {
          this.demo_transfer = 0;
        }
        return data;
      }
      switch (command) {
        case 0x00:
          data[0] = 0x01;
          data[1] = 0x00;
          data[2] = 0x00;
          break;
        case 0x12:
          data[0] = 0x46;
          data[1] = 0x00;
          data[2] = 0x00;
          break;
        case 0xf0:
        case 0xf1:
        case 0xf2:
        case 0xf3:
        case 0xf5:
          data[0] = command;
          data[1] = command;
          data[2] = command;
          this.demo_transfer = command;
          break;
        default:
          return null;
      }
      data[3] = command ^ data[0] ^ data[1] ^ data[2];
      return data;
    }
    if (!this.port) {
      return null;
    }
    const writer = this.port.writable.getWriter();
    const commandBuffer = new Uint8Array(1);
    commandBuffer[0] = command;
    await writer.write(commandBuffer);
    await writer.close();
    const reader = this.port.readable.getReader();
    const readBuffer = new Uint8Array(4);
    const { value, done } = await reader.read();
    await reader.close();
    console.log(value);
    return value;
  }

  iocs_bitsns(group) {
    return this.bitsns[group];
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