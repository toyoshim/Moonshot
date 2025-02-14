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
    KeyL: { group: 4, bit: 0x40 },
  };

  console = null;
  bitsns = null;
  port = null;
  reader = null;
  demo = false;
  transfer = 0;
  transfer_length = 0;

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
      this.writer = await this.port.writable.getWriter();
      this.reader = await this.port.readable.getReader();
    } catch (e) {
      this.port = null;
      console.log(e);
    }
  }

  async comm(command) {
    if (this.demo) {
      const data = new Uint8Array(4);
      if (this.transfer) {
        switch (this.transfer) {
          case 0xf0:
          case 0xf1:
          case 0xf2:
            data[0] = command;
            data[1] = command;
            data[2] = command;
            data[3] = command;
            break;
          case 0xf3:
            this.transfer_length = command;
            data[0] = command;
            data[1] = command;
            data[2] = command;
            data[3] = command;
            break;
          case 0xf4:
            this.transfer_length++;
            data[0] = command;
            data[1] = command;
            data[2] = command;
            data[3] = command;
            break;
          case 0xf5:
            this.transfer_length = (this.transfer_length + 4) >> 2;
            data[0] = 0;
            data[1] = 0;
            data[2] = 0;
            data[3] = 0;
            break;
          default:
            console.log('Unknown transfer: $' + this.transfer.toString(16));
            break;
        }
        if (this.transfer == 0xf4 || this.transfer == 0xf5) {
          this.transfer_length--;
          if (this.transfer_length < 0) {
            this.transfer = 0;
          }
        } else {
          this.transfer = 0;
        }
        return data;
      }
      switch (command) {
        case 0x00:
          data[0] = 0x01;
          data[1] = 0x00;
          data[2] = 0x00;
          break;
        case 0x01:
          data[0] = 0x00;
          data[1] = 0x00;
          data[2] = 0x00;
          break;
        case 0x02:
        case 0x03:
          data[0] = 0x80;
          data[1] = 0x80;
          data[2] = 0x80;
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
        case 0xf4:
        case 0xf5:
          data[0] = command;
          data[1] = command;
          data[2] = command;
          this.transfer = command;
          break;
        default:
          console.log('Unknown command: $' + command.toString(16));
          return null;
      }
      data[3] = command ^ data[0] ^ data[1] ^ data[2];
      return data;
    }
    if (!this.port) {
      return null;
    }
    if (this.transfer) {
      if (this.transfer == 0xf3) {
        this.transfer_length = command;
      }
      if (this.transfer == 0xf5 || this.transfer == 0xf4) {
        this.transfer_length--;
        if (this.transfer_length < 0) {
          this.transfer = 0;
        }
      } else {
        this.transfer = 0;
      }
    } else {
      switch (command) {
        case 0x01:
        case 0x02:
        case 0x03:
          return [0, 0, 0, command];
        case 0xf0:
        case 0xf1:
        case 0xf2:
        case 0xf3:
          this.transfer = command;
          break;
        case 0xf4:
          this.transfer = command;
          this.transfer_length++;
          break;
        case 0xf5:
          this.transfer = command;
          this.transfer_length = (this.transfer_length >> 2) + 1;
          break;
        default:
          break;
      }
    }
    const commandBuffer = new Uint8Array(1);
    commandBuffer[0] = command;
    await this.writer.write(commandBuffer);
    const receiveBuffer = new Uint8Array(4);
    let read = 0;
    do {
      const { value, done } = await this.reader.read();
      for (let i = 0; i < value.length; ++i) {
        receiveBuffer[read++] = value[i];
      }
    } while (read != 4);
    return receiveBuffer;
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