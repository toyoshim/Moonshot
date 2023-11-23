---
layout: default
title: サポートページ
permalink: /
---
# できること
- 左側のUSBポートに刺したゲームコントローラをJOY STICK端子に変換する
  + 標準的な2ボタンと上下・左右同時押しを利用したSTART/SELECTボタンに対応
    * 製品候補版以降はCPSF準拠で6ボタンに対応
  + 基板上のボタンでモードを切り替え、サイバースティックのアナログモードに対応
  + 基板上のボタンでモードを切り替え、チェルノブコンバーターを想定したメガドライブモードに対応
    * 製品候補版以降はX,Y,Z,Modeを追加した6ボタン仕様にも対応
  + 公式にサポートするゲームコントローラは[こちら](https://toyoshim.github.io/iona-us/firmware)を参照、実際はほとんどのコントローラが動作するはず
  + 動かないコントローラについてはご相談ください

# 将来できるかもしれないこと
- 専用ドライバを使い右側のポートを2P用に利用
  + IOCSをフックする予定なので、直接I/Oを叩いたゲーム等には対応できません
- 同様にしてキーボード、マウスに対応
- USBハブを経由した接続に対応

# 動作モード
基板上のボタンを押すことで切り替わり、LEDの状態でモードを区別できる
- 点灯: 通常モード
- 点滅1: サイバースティックモード
- 点滅2: メガドライブモード

# ボタンのレイアウト
将来的には本体でもレイアウト変えられるようにしたいと思いつつ、現状は既存のIONA-US用の[設定](https://toyoshim.github.io/iona-us/setting)ページを流用できるようにしてあります。
元々がアーケード向けなので関係ない設定項目がありますが、Moonshotでは無視されます。
通常モードで設定1が、サイバースティックモードで設定2が適用されるようになっています。
内部的には最大2つのコントローラに対し、それぞれ13ボタン6アナログ入力まで認識しています。
設定するためには以下のファームウェアアップデートに必要な事前準備を済ませる必要があります。

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
    'firmwares/ms_v0_97.bin',
    'firmwares/ms_v0_98.bin',
    'firmwares/ms_v0_99.bin',
    'firmwares/ms_v0_99_1.bin',
    'firmwares/ms_v0_99_2.bin',
    'firmwares/ms2_v0_99.bin',
    'firmwares/ms2_v0_99_1.bin',
    'firmwares/ms2_v0_99_2.bin',
    'firmwares/ms2_v0_99_3.bin',
    'firmwares/ms2_v0_99_4.bin',
    'firmwares/ms2_v0_99_5.bin',
    'firmwares/ms2_v0_99_6.bin',
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
  const data_url = 'firmwares/data.bin';
  const data_response = await fetch(data_url);
  if (data_response.ok) {
    const data_bin = await data_response.arrayBuffer();
    for (let i = 0; i < 1024; i += 32) {
      await flasher.writeDataInRange(i, data_bin.slice(i, i + 32));
    }
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
<option>プロトタイプ用 Ver 0.97</option>
<option>プロトタイプ用 Ver 0.98</option>
<option>プロトタイプ用 Ver 0.99</option>
<option>プロトタイプ用 Ver 0.99.1</option>
<option>プロトタイプ用 Ver 0.99.2</option>
<option>製品候補用 Ver 0.99</option>
<option>製品候補用 Ver 0.99.1</option>
<option>製品候補用 Ver 0.99.2</option>
<option>製品候補用 Ver 0.99.3</option>
<option>製品候補用 Ver 0.99.4</option>
<option>製品候補用 Ver 0.99.5</option>
<option selected>製品候補用 Ver 0.99.6</option>
</select>
<button onclick="flash();">書き込み</button>

| | |
|-|-|
|書き込み|0% <progress id="progress_write" max=1 value=0></progress> 100%|
|検証|0% <progress id="progress_verify" max=1 value=0></progress> 100%|

結果
<pre id="error"></pre>
