---
layout: default
title: サポートページ
permalink: /
---
{: align="center"}
![Moonshot](MS1.png)

# できること
- 左側のUSBポートに刺したゲームコントローラをJOY STICK端子に変換する
  + 標準的な2ボタンと上下・左右同時押しを利用したSTART/SELECTボタンに対応
    * 製品候補版以降はCPSF準拠で6ボタンに対応
  + 基板上のボタンでモードを切り替え、サイバースティックのアナログモードに対応
  + 製品候補版以降では基板上のボタンでモードを切り替え、チェルノブコンバーターを想定したメガドライブモードに対応
    * X,Y,Z,Modeを追加した6ボタン仕様にも対応
  + 公式にサポートするゲームコントローラは[こちら](https://toyoshim.github.io/iona-us/firmware)を参照、実際はほとんどのコントローラが動作するはず
  + 動かないコントローラについてはご相談ください
- サイバースティックモードではプロトコルを拡張してホストと双方向通信が可能
  + レイアウト設定ツールがX68kから利用できるようになりました

# 将来できるかもしれないこと
- 専用ドライバを使い右側のポートを2P用に利用
  + IOCSをフックする予定なので、直接I/Oを叩いたゲーム等には対応できません
- 同様にしてキーボード、マウスに対応
- USBハブを経由した接続に対応

# 動作モード
基板上のボタンを押すことで切り替わり、LEDの状態でモードを区別できる
- 点灯: 通常モード
- 高速点滅: サイバースティックモード
- 低速点滅: 低速通信モード（じょいぽーとU君用）
- 2回点滅: メガドライブモード

# ボタンのレイアウト
[X68kから利用するツール](https://github.com/toyoshim/Moonshot/tree/main/tools)と
同ツールをウェブから利用できるようにしたツールがあります。
ウェブから設定するためには左側のUSBポートをUSB A to AのケーブルでPCに接続してください。
各種OSからはシリアルポートとして認識され、Chromeからは"Moonshot"の名前でアクセスできるようになります。

# ファームウェアアップデート
## 事前準備
### USB Type A to Type Aのケーブル
USB規格からは外れる特殊ケーブルですが、両端がType Aのケーブルを使いPCと接続する必要があります。
Amazonで500円から1,000円くらいで入手可能です。
この際、必ずJOY STICKポートからは取り外し、USBのみを接続してください。双方を同時に接続すると、両者から電源が流れ込み、接続した機器の故障や発火の原因になります。
また接続する時は基板上のボタンを押して接続してください。ファームウェア更新モードで接続するとLEDは消灯します。

### WinUSBのドライバの設定（Windowsのみ）
Windowsに限ってドライバが必要となります。Windows 10以降では標準で搭載されています。
自動ではインストールされないので、手動で`ユニバーサル シリアル バス デバイス`から`WinUsb デバイス`を選択してインストールする必要があります。
詳細は[Microsoft公式情報](https://learn.microsoft.com/ja-jp/windows-hardware/drivers/usbcon/winusb-installation#installing-winusb-by-specifying-the-system-provided-device-class)をご参照ください。
参考動画も[こちら](https://www.youtube.com/watch?v=5yzpc2vI_94)にあります。
USBホストとの相性問題があるようで、ドライバがうまくスタートできない事があります。
使用しているチップの仕様なので根本的な修正ができないのですが、リトライする、他のUSBポートを試してみる、USBハブ越しに接続する、などが有効なようです。Windows以外のOSでは相性問題は発生しません。

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
    print: (text) => {},
    printErr: (text) => {},
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
    },
    postRun: () => {
      window.console.log(this);
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

## アップデート
ここから先はChromiumベースのブラウザが必要です。
Google ChromeやMicrosoft Edgeなどで動作するはずです。

まずは事前準備の通り、`他の機器には接続せず`、`基板上のボタンを押しながら左のUSB端子をPCに接続`します。
LEDが消灯している事を確認してください。

書き込みたいバージョンをメニューから選び`書き込み`ボタンを押すと、デバイス選択ダイアログが表示されます。
デバイスが正しく認識されていれば`WinChipHead製の不明なデバイス`に準じた名前がリストされますので、その名前を選択した上で`接続`ボタンを押すことで、アップデートが開始します。
アップデートの際、レイアウト設定もデフォルトに再設定されます。

チップが一枚だけ載った初期の物がプロトタイプ、チップが3枚載り裏にMoonshotと記載されているのが製品候補です。

<script src="https://toyoshim.github.io/CH559Flasher.js/CH559Flasher.js"></script>
<script>
async function flash() {
  const firmwares = [
    'firmwares/ms_v0_99_8.bin',
    'firmwares/ms_v0_99_9.bin',
    'firmwares/ms_v0_99_10.bin',
    'firmwares/ms_v1_00_0.bin',
    'firmwares/ms2_v0_99_8.bin',
    'firmwares/ms2_v0_99_9.bin',
    'firmwares/ms2_v0_99_10.bin',
    'firmwares/ms2_v1_00_0.bin',
  ];
  const data = [
    'firmwares/data.bin',
    'firmwares/data2.bin',
    'firmwares/data2.bin',
    null,
    'firmwares/data.bin',
    'firmwares/data2.bin',
    'firmwares/data2.bin',
    null,
  ];
  const progressWrite = document.getElementById('progress_write');
  const progressVerify = document.getElementById('progress_verify');
  const error = document.getElementById('error');
  progressWrite.value = 0;
  progressVerify.value = 0;
  error.innerText = '';

  const flasher = new CH559Flasher();
  await flasher.connect();

  await flasher.eraseData();
  const data_url = data[document.getElementById('version').selectedIndex];
  if (data_url) {
    const data_response = await fetch(data_url);
    if (data_response.ok) {
      const data_bin = await data_response.arrayBuffer();
      for (let i = 0; i < 1024; i += 32) {
        await flasher.writeDataInRange(i, data_bin.slice(i, i + 32));
      }
    }
  } else {
    await flasher.eraseData();
  }

  await flasher.erase();
  const url = firmwares[document.getElementById('version').selectedIndex];
  const response = await fetch(url);
  if (response.ok) {
    const bin = await response.arrayBuffer();
    await flasher.write(bin, rate => progressWrite.value = rate);
    await flasher.verify(bin, rate => progressVerify.value = rate);
    error.innerText = flasher.error ? flasher.error : '成功';
  } else {
    error.innerText = 'ファームウェアが見つかりません';
  }
}
</script>

<select id="version">
<option>プロトタイプ用 Ver 0.99.8</option>
<option>プロトタイプ用 Ver 0.99.9</option>
<option>プロトタイプ用 Ver 0.99.10</option>
<option>プロトタイプ用 Ver 1.00.0</option>
<option>製品候補用 Ver 0.99.8</option>
<option>製品候補用 Ver 0.99.9</option>
<option>製品候補用 Ver 0.99.10</option>
<option selected>製品候補用 Ver 1.00.0</option>
</select>
<button onclick="flash();">書き込み</button>

| | |
|-|-|
|書き込み|0% <progress id="progress_write" max=1 value=0></progress> 100%|
|検証|0% <progress id="progress_verify" max=1 value=0></progress> 100%|

結果
<pre id="error"></pre>

