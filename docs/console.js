// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

class Console {
  width = 0;
  height = 0;
  element = null;
  lines = null;
  cursor = { x: 0, y: 0 };
  escape = false;
  sequence = '';
  code = 0;
  remaining = 0;

  constructor(width, height, element) {
    this.width = width;
    this.height = height;
    this.element = element;
    this.lines = new Array(height);
    for (let y = 0; y < height; ++y) {
      const pre = document.createElement('pre');
      pre.className = 'console-line';
      this.element.appendChild(pre);
      this.lines[y] = {
        chars: new Array(width),
        element: pre
      };
      for (let x = 0; x < width; ++x) {
        this.lines[y].chars[x] = ' ';
      }
      pre.textContent = this.lines[y].chars.join('');
    }
  }

  next_line() {
    this.flush_current_line();
    this.cursor.y++;
    if (this.cursor.y >= this.height) {
      // TODO: scroll instead?
      this.cursor.y = 0;
    }
    this.cursor.x = 0;
  }

  putc(c) {
    this.lines[this.cursor.y].chars[this.cursor.x] = c;
    this.cursor.x++;
    if (this.cursor.x < this.width && c.charCodeAt(0) >= 0x100) {
      this.lines[this.cursor.y].chars[this.cursor.x] = '';
      this.cursor.x++;
    }
    if (this.cursor.x >= this.width) {
      this.next_line();
    }
  }

  cls() {
    this.cursor.x = 0;
    this.cursor.y = 0;
    for (let y = 0; y < this.height; ++y) {
      for (let x = 0; x < this.width; ++x) {
        this.lines[y].chars[x] = ' ';
      }
      this.lines[y].element.textContent = this.lines[y].chars.join('');
    }
  }

  print(fd, str) {
    for (let c of str) {
      if (this.escape) {
        this.sequence += c;
        if (this.sequence.match(/\[>5h/)) {
          this.escape = false;
        } else if (this.sequence.match(/\[2J/)) {
          this.cls();
          this.escape = false;
        } else if (this.sequence.match(/\[1K/)) {
          for (let x = 0; x < this.cursor.x; ++x) {
            this.lines[this.cursor.y].chars[x] = ' ';
          }
          this.escape = false;
        } else if (this.sequence.match(/\[([1-9][0-9]*);([1-9][0-9]*)H/)) {
          const y = parseInt(RegExp.$1) - 1;
          const x = parseInt(RegExp.$2) - 1;
          if (this.cursor.y != y) {
            this.flush_current_line();
          }
          this.cursor.x = x;
          this.cursor.y = y;
          this.escape = false;
        } else if (this.sequence.match(/\[([1-9][0-9]*)C/)) {
          const x = parseInt(RegExp.$1);
          this.cursor.x += x;
          this.escape = false;
        }
        continue;
      }
      const code = c.charCodeAt(0);
      if (code == 0x0a) {
        this.next_line();
      } else if (code == 0x1b) {
        this.escape = true;
        this.sequence = '';
      } else if (code < 0x80) {
        this.putc(c);
      } else if (code < 0xc0) {
        this.code <<= 6;
        this.code |= c.charCodeAt(0) & 0x3f;
        this.remaining--;
        if (!this.remaining) {
          this.putc(String.fromCharCode(this.code));
        }
      } else if (code < 0xe0) {
        this.code = c.charCodeAt(0) & 0x1f;
        this.remaining = 1;
      } else if (code < 0xf0) {
        this.code = c.charCodeAt(0) & 0x0f;
        this.remaining = 2;
      } else {
        this.code = c.charCodeAt(0) & 0x07;
        this.remaining = 3;
      }
    }
    this.flush_current_line();
  }

  flush_current_line() {
    this.lines[this.cursor.y].element.textContent = this.lines[this.cursor.y].chars.join('');
  }
}