# ESP32にST7789ディスプレイをつなぐ
aliで買った"<b>ESP32-C3 supermini</b>"にSPIディスプレイST7789をつなぐテスト<br>
<br>

# esp32s3-supermiin
ボート名はPlatformIOには登録されていない。<br>
<br>
[https://community.platformio.org/t/esp32-s3-zero-does-not-work-on-platformio/40297/10 ](https://sigmdel.ca/michel/ha/esp8266/super_mini_esp32c3_en.html#platformio)<br>
上記の情報で自前で作成登録した。詳細はリンク先参照。<br>

# ディスプレイ PIN

* GND          GND
* VCC          3.3V
* SCL (SCLK) → GPIO04
* SDA (MOSI）→ GPIO06
* RES (RST)  → GPIO01
* DC        → GPIO02
* CS        → GPIO00
* BLK       → GPIO03

ピンは3.3vピンを基準に適当に配置。結構好きに変更可能。<br>
lovyan03/LovyanGFX@^1.2.7 を使用


## Dependency
Visual studio 2022 C#

## License
This software is released under the MIT License, see LICENSE

## Authors

bry-ful(Hiroshi Furuhashi)<br>
twitter:[bryful](https://twitter.com/bryful)<br>
Mail: bryful@gmail.com<br>

