---
layout: default
title: サポートページ
permalink: /setting
---
# レイアウト設定
---
左側のUSBポートをUSB A to AのケーブルでPCに接続してください。
標準規格外のケーブルですが、Amazonで「USB A to A」などで検索すると見つかります。
この際、ジョイスティック端子には接続せずに、単体でPCと接続してください。
LEDが高速に点滅していたら設定モードでの起動に成功しています。
OSからはシリアルポートとして認識され、ブラウザからは"Moonshot"の名前でアクセスできるようになります。

デバイスを繋げたら「接続」ボタンを押し、デバイス選択ダイアログからMoonshotを選んで「接続」を押します。
デバイスがない場合は「デモ」ボタンから擬似的にツールを体験できます。

（注）ウェブ上での設定はChromiumベースのブラウザ専用です。Google ChromeやMicrosoft Edgeなどでご利用ください。

<button id="connect">接続</button>
<button id="demo">デモ</button>
<pre id="console" align="center"></pre>

<style type="text/css">
@font-face {
  font-family: 'Elisa8';
  src: url('JF-Dot-Elisa8.ttf') format('truetype');
}

pre.console-line {
  margin: 0;
  padding: 0;
  border: 0;
  font-size: 12px;
  font-family: 'Elisa8', monospace;
  line-height: 1;
  box-shadow: none;
  text-shadow: 0 0 1rem #0f0, 0 0 1rem #00f;
}
</style>
<script type="text/javascript" src="./console.js"></script>
<script type="text/javascript" src="./io.js"></script>
<script type="text/javascript">
  var Module = {
    console: new Console(96, 32, document.getElementById('console')),
    io: new IO(this.console),
    print: (text) => console.log(text),
    printErr: (text) => console.error(text),
    ms_comm: async (len, cmd_ptr, res_ptr) => {
      if (!this.Module.io.connected()) {
        return 128;
      }
      this.Module.HEAPU8[res_ptr + 0] = 0xff;
      this.Module.HEAPU8[res_ptr + 1] = 0x00;
      this.Module.HEAPU8[res_ptr + 2] = 0x00;
      this.Module.HEAPU8[res_ptr + 3] = 0x00;
      this.Module.HEAPU8[res_ptr + 4] = 0x00;
      this.Module.HEAPU8[res_ptr + 5] = 0xff;
      let error = 0;
      for (let i = 0; i < len; ++i) {
        let result = await this.Module.io.comm(this.Module.HEAPU8[cmd_ptr + i]);
        if (!result) {
          error = 129;
          break;
        }
        this.Module.HEAPU8[res_ptr + 4 * i + 6] = result[0];
        this.Module.HEAPU8[res_ptr + 4 * i + 7] = result[1];
        this.Module.HEAPU8[res_ptr + 4 * i + 8] = result[2];
        this.Module.HEAPU8[res_ptr + 4 * i + 9] = result[3];
      }
      return new Promise((resolve, reject) => {
        setTimeout(e => {
          resolve(error);
        }, 1);
      });
    },
    bitsns: bitsns => {
      return this.Module.io.iocs_bitsns(bitsns);
    },
    preRun: () => {
      this.printChar = (stream, curr) => {
        this.Module.console.print(stream, String.fromCharCode(curr));
      };
      this.Module.console.print(0, "A>MSCONF.X\n");
    },
    postRun: () => {
      setTimeout(e => {
        this.Module.console.print(0, "A>\n");
      }, 1);
    },
  };
  document.getElementById('connect').addEventListener('click', async e => {
    await Module.io.connect();
    if (Module.io.connected()) {
      Module._main();
    }
  });
  document.getElementById('demo').addEventListener('click', async e => {
    Module.io.demo = true;
    if (Module.io.connected()) {
      Module._main();
    }
  });
</script>
<script async type="text/javascript" src="msconf.js"></script>

レイアウト変更後は「S」ボタンで設定後のレイアウトをMoonshotに保存します。
このページと同様の設定がX68k上から行えます。
実機上では押したボタンや設定した出力の状態がリアルタイムで表示されるため、確認が簡単にできます。

（注）接続やデモは複数回押すとうまく動作しないため、エラーが出たらページをリロードしてお試しください。

# X680x0版設定ツール
---
[X68kから利用するツール msconf.x](https://github.com/toyoshim/Moonshot/tree/main/tools)が用意されています。

各種エミュレータやX68000ZからじょいぽーとＵ君ごしに利用している場合、
U君をNOTIFYモードにした上で Moonshotを「低速通信モード」に、
それ以外では「サイバースティックモード」にした後にmsconf.xを起動して設定します。
