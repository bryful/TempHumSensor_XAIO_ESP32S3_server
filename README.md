# EPS32S3を使った温度湿度計 親機
XIAO ESP32S3を使った温度湿度計。WiFi(tcp)で子機から送られてきた情報を一覧表示します。<br>
3Dプリンターのフィラメントケースの湿度管理のために作成したもの。<br>
<br>
音素湿度センサーは AHT20+BMP280モジュールを使用。ただし、BMP280を使った気圧センシングは行わない（親機はおまけで表示）

# 構成

* 親機は2.8インチのモニタ(ST7789)をつなげて子機から送られてくる温度湿度を一覧表示。電源は有線USBから配電。
* 温度センサはAHT20を使用
* 画面表示リフレッシュはは２０分間隔。ただし、子機からの情報を受信したら即表示。
* LOG保存は行わない（次期バージョンで考慮）そのため時間管理は適当。
* 湿度が30%以上になったら表示を黄色に
* 子機からの情報が一定期間送られてこなかったら赤表示に
* 子機は16850電池駆動。
* バッテリー管理はまだ行わない（止まったら交換でということで）
* バッテリーはUSB端子から。バッテリーごと交換で充電はしない。
* 子機はdeepSleep機能を使い節電。

# 特徴

* LovyanGFXを使ったグラフィカルな表示。
* 最大6機の子機からの湿度リスト。
* 30%以上になったら黄色く警戒色に
* 初めてのardiunoのプログラムなのでなるべく簡単に
* WifiのSSIDパスワード親機のIPアドレス等のプライベート情報はコードに記入しないで、USB接続シリアル通信で設定。
<br>
現在とりあえず作っていろいろ調整中。


# 初めて作った感想

### Ardiuno IDEより VSCode+PlatgformIOの方が楽
Ardiuno IDEはエディタがなじめずサンプルソースしか見なくなった。

### WiFiが凄く不安定
Wifi接続が不安定。プログラムのほとんどがこれの対処だった。

### ESP32への書き込み失敗の多さ
かなりのストレス。
ただ、esptool.pyのバージョンアップでかなり改善。
platformio.iniに以下の項目を追加することでバージョンアップできた。
```
platform_packages = platformio/tool-esptoolpy@1.40801.0

```

###
# ディスプレイ PIN


* GND           GND
* VCC           3.3V
* SCL (SCLK) → GPIO07
* SDA (MOSI）→ GPIO09
* RES (RST)  → GPIO01
* DC         → GPIO02
* CS         → GPIO04
* BLK        → GPIO43

SDA/SCLをl2cの為に開けて配置

## Dependency
VScode + PlatfornIO

## License
This software is released under the MIT License, see LICENSE

## Authors

bry-ful(Hiroshi Furuhashi)<br>
twitter:[bryful](https://twitter.com/bryful)<br>
Mail: bryful@gmail.com<br>

