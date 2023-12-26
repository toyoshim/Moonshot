---
layout: default
title: ファームウェア更新
permalink: /firmware
---
# ファームウェアアップデート
---
## 事前準備
### USB Type A to Type Aのケーブル
Moonshot上の青いボタンを押しながら、左側のUSBポートをUSB A to AのケーブルでPCに接続してください。
標準規格外のケーブルですが、Amazonで「USB A to A」などで検索すると見つかります。
この際、ジョイスティック端子には接続せずに、単体でPCと接続してください。

ファームウェア更新モードで接続するとLEDは消灯します。

### WinUSBのドライバの設定（Windowsのみ）
Windowsに限ってドライバが必要となります。Windows 10以降では標準で搭載されています。
自動ではインストールされないので、手動で`ユニバーサル シリアル バス デバイス`から`WinUsb デバイス`を選択してインストールする必要があります。
詳細は[Microsoft公式情報](https://learn.microsoft.com/ja-jp/windows-hardware/drivers/usbcon/winusb-installation#installing-winusb-by-specifying-the-system-provided-device-class)をご参照ください。
参考動画も[こちら](https://www.youtube.com/watch?v=5yzpc2vI_94)にあります。

USBホストとの相性問題があるようで、ドライバがうまくスタートできない事があります。
使用しているチップのファームウェア更新モードにある問題なので根本的な修正ができないのですが、リトライする、他のUSBポートを試してみる、USBハブ越しに接続する、他のUSB機器（特に近いポートに刺さっているもの）を外して試す、などが有効なようです。Windows以外のOSでは相性問題は発生しません。

## アップデート
ここから先はChromiumベースのブラウザが必要です。
Google ChromeやMicrosoft Edgeなどをご利用ください。

書き込みたいバージョンをメニューから選び「書き込み」ボタンを押すと、デバイス選択ダイアログが表示されます。
デバイスが正しく認識されていれば「WinChipHead製の不明なデバイス」に準じた名前がリストされますので、その名前を選択した上で「接続」ボタンを押すことで、アップデートが開始します。

チップが一枚だけ載った初期の物がプロトタイプ、チップが3枚載り裏にMoonshotと記載されているのが製品候補です。

<script src="https://toyoshim.github.io/CH559Flasher.js/CH559Flasher.js"></script>
<script>
async function flash() {
  const firmwares = [
    'firmwares/ms_v1_00_0.bin',
    'firmwares/ms2_v1_00_0.bin',
  ];
  const progressWrite = document.getElementById('progress_write');
  const progressVerify = document.getElementById('progress_verify');
  const error = document.getElementById('error');
  progressWrite.value = 0;
  progressVerify.value = 0;
  error.innerText = '';

  const flasher = new CH559Flasher();
  await flasher.connect();
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
<option>プロトタイプ用 Ver 1.00.0</option>
<option selected>製品候補用 Ver 1.00.0</option>
</select>
<button onclick="flash();">書き込み</button>

| | |
|-|-|
|書き込み|0% <progress id="progress_write" max=1 value=0></progress> 100%|
|検証|0% <progress id="progress_verify" max=1 value=0></progress> 100%|

結果
<pre id="error"></pre>

