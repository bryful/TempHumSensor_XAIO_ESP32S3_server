#include <Arduino.h>
#include <WiFi.h>
#include <WiFiServer.h>
#include <ArduinoJson.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

// lovyan
#include "LGFX_XIAO_ESP32S3_SPI_ST7789B.hpp"

#include "FsUtils.hpp"
#include "Sensor.hpp"

// 操作確認用LED
#define LED_PIN (44)
// クライアントの最大数
#define MAX_CLIENTS 6
// 画面書き換えタイミング1秒ごと
// #define SCAN_MSEC 1000
#define SCAN_MSEC (60 * 1000)
// 接続チェックのタイミング
#define SCAN_TIME (30 * 60 * 1000)
// クライアントの接続チェックこの数値以上エラーがあったら注意表示
#define TIMEOUT_COUNT 3

#define SCR_DOWN_MSEC (20 * 1000)
#define SCR_BR_MAX 90
#define SCR_BR_MIN 15

// 画面サイズ
#define SCR_WIDTH 240
#define SCR_HEIGHT 320
#define SCR_HEAD_HEIGHT 60
#define SCR_LINE_HEIGHT 42

// 準備したクラスのインスタンスを作成します。
LGFX_XIAO_ESP32S3_SPI_ST7789B display;

// 画面描画用の裏画面
lgfx::LGFX_Sprite headbuf(&display);
lgfx::LGFX_Sprite scrbuf(&display);

FsUtils fsu;

Adafruit_BMP280 bmp; // I2c 0x77
Adafruit_AHTX0 aht;  // I2c 0x38

WiFiServer server(SERVER_PORT);

SensorData clients[MAX_CLIENTS];
SensorData OwnTemp;

const char *HostName = "Temp-Hum Senssor by bry-ful";
unsigned long nextTime = 0;
unsigned long errorTime = 0;
unsigned long brTime = 0;

// -----------------------------------------------------------------------
// 画面の消去
void DisplayClear()
{
  display.fillScreen(TFT_BLACK);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(TFT_WHITE);
  display.setBrightness(80);
}
// 画面へメッセージ
void DisplayPrint(String s)
{
  DisplayClear();
  display.setBrightness(100);
  display.println(s);
  // 10秒表示
  nextTime = millis() + 10000;
}
// ==== ディスプレイ初期化 ====
void setupDisplay()
{
  // 裏画面作成
  headbuf.createSprite(SCR_WIDTH, SCR_HEAD_HEIGHT);
  headbuf.setFont(&fonts::lgfxJapanGothic_16);
  scrbuf.createSprite(SCR_WIDTH, SCR_LINE_HEIGHT);
  scrbuf.setFont(&fonts::lgfxJapanGothic_28);

  display.init();
  display.setRotation(2);
  display.setFont(&fonts::lgfxJapanGothic_16);

  display.fillScreen(TFT_BLACK);
  display.setTextColor(TFT_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 10);
  display.println("Booting...");
}
// -----------------------------------------------------------------------
// == == Wi - Fi と時刻初期化 == ==
bool getNTP()
{
  bool ret = false;
  display.println("get NTP");
  if (WiFi.status() != WL_CONNECTED)
  {
    DisplayPrint("ERROR getNTP WiFi Disconnect");
    return ret;
  }
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp", "time.google.com"); // 2.7.0以降, esp32コンパチ

  time_t t;
  struct tm *timeinfo;
  t = time(NULL);
  timeinfo = localtime(&t);
  int retry = 0;
  while (!getLocalTime(timeinfo, 1000))
  {
    if (retry > 10)
    {
      retry = -1;
      break;
    }
    display.print("*");
    delay(500);
    retry++;
  }
  display.print("\n");
  if (retry < 0)
  {
    DisplayPrint("ERROR NTP/getLocalTime()");
    ret = false;
  }
  else
  {
    display.println("NTP OK!");
    ret = true;
  }
  return ret;
}
void setupWiFiAndTime()
{

  DisplayClear();
  display.println("Connecting WiFi...");
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFi.disconnect();
  }
  // 固定IP設定

  int lc[4] = {0, 0, 0, 0};
  int gw[4] = {0, 0, 0, 0};
  if (fsu.getPrefIPA(SERVER_IP_KEY, lc) == false)
  {
    Serial.println("ERR ! fixedip");
    display.printf("ERR! fixedip / \nserverip\n%d.%d.%d.%d", lc[0], lc[1], lc[2], lc[3]);
    nextTime = millis() + 10000;
  }
  if (fsu.getPrefIPA(GATWAY_IP_KEY, gw) == false)
  {
    Serial.println("ERR ! gateway");
    display.printf("gateway %d.%d.%d.%d", lc[0], lc[1], lc[2], lc[3]);
    nextTime = millis() + 10000;
  }
  if ((lc[0] != 0) && (gw[0] != 0))
  {
    IPAddress local_IP(lc[0], lc[1], lc[2], lc[3]);
    IPAddress gateway(gw[0], gw[1], gw[2], gw[3]);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress primaryDNS(8, 8, 8, 8);   // オプション
    IPAddress secondaryDNS(8, 8, 4, 4); // オプション
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    {
      display.setCursor(10, 70);
      display.println("STA Failed to configure");
      nextTime = millis() + 10000;
    }
    else
    {
      display.printf("Config OK!\nip:%s\n", WiFi.localIP().toString());
      nextTime = millis() + 10000;
    }
  }
  WiFi.setHostname(HostName);
  WiFi.begin();
  // WiFi.setTxPower(WIFI_POWER_8_5dBm);
  int retry = 0;
  bool done = true;
  while (WiFi.status() != WL_CONNECTED)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      break;
      ;
    }
    display.print("*");
    delay(350);
    if (retry % 10 == 9)
    {
      WiFi.disconnect();
      WiFi.reconnect();
    }
    else if (retry > 30)
    {
      break;
      ;
    }
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    display.println("\nWiFi Connected!");
  }
  else
  {
    display.println("\nWiFi Failed");
  }

  getNTP();
}

// ==== clientの更新 ====
void updateClientData(const String &id, const String &timeStr, float temp, float hum)
{
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (clients[i].id == id || clients[i].id == "")
    {
      clients[i].id = id;
      clients[i].timeStr = timeStr;
      clients[i].temp = temp;
      clients[i].hum = hum;
      clients[i].missedCount = 0;
      return;
    }
  }
}
// ==== 自機センサーの更新 ====
void readOwnTemp()
{
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  float pres = bmp.readPressure() / 100.0;
  // 現在時刻取得
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%Y/%m/%d %H:%M", &timeinfo);

  OwnTemp.temp = temp.temperature;
  OwnTemp.hum = humidity.relative_humidity;
  OwnTemp.pres = pres;
  OwnTemp.timeStr = timeStr;
}
// ==== 上部表示の更新 ====
void headbudPrint(int col)
{

  headbuf.fillScreen(TFT_BLACK);
  headbuf.setTextSize(1);
  headbuf.setTextColor(col);
  headbuf.setCursor(10, 2);
  headbuf.println("Temp-Hum Sensor by bry-ful");
  headbuf.setCursor(30, 20);
  headbuf.println(OwnTemp.timeStr);
  headbuf.setCursor(30, 38);
  headbuf.setTextSize(1);
  headbuf.printf("%.1f℃ %.1f%% %.1fpHa", OwnTemp.temp, OwnTemp.hum, OwnTemp.pres);
  headbuf.drawRoundRect(0, 0, SCR_WIDTH - 1, SCR_HEAD_HEIGHT - 1, 6, col);
  headbuf.setTextSize(1);
}
// ==== クライアント表示の更新 ====
void scrbudPrint(int index)
{
  if (index >= MAX_CLIENTS)
    return;
  String id = clients[index].id;
  float t = clients[index].temp;
  float h = clients[index].hum;
  String tmstr = clients[index].timeStr;
  int mode = 0;
  /*
      0 no connect
      1 normal
      -1 Count err /Disconnect
    */
  int col = TFT_BLACK;
  if (id == "")
  {
    mode = 0;
    col = TFT_DARKGRAY;
  }
  else
  {
    col = TFT_WHITE;
    mode = 1;
    if (clients[index].missedCount >= TIMEOUT_COUNT)
    {
      mode = 2;
      col = TFT_RED;
    }
  }
  bool b = false;
  b = (((OwnTemp.hum - h < 10) || (h > 30)) && (mode == 1));
  scrbuf.fillScreen(TFT_BLACK);

  // ＩＤの描画
  scrbuf.setTextSize(1);
  scrbuf.setTextColor(col);
  scrbuf.setCursor(15, 1);
  scrbuf.setFont(&fonts::lgfxJapanGothic_16);
  if (mode == -1)
  {
    scrbuf.print("-----");
  }
  else
  {
    scrbuf.print(id);
  }
  // 温度
  scrbuf.setTextSize(1);
  scrbuf.setCursor(15, 17);
  scrbuf.setFont(&fonts::lgfxJapanGothic_24);
  if (mode == -1)
  {
    scrbuf.print("--.-℃");
  }
  else
  {
    scrbuf.printf("%.1f℃", t);
  }

  // 下線
  scrbuf.drawLine(10, SCR_LINE_HEIGHT - 1, SCR_WIDTH - 10, SCR_LINE_HEIGHT - 1);
  // 時間
  scrbuf.setTextSize(1);
  scrbuf.setCursor(200, 1);
  scrbuf.setFont(&fonts::lgfxJapanGothicP_12);
  // 2025/06/25 00:00:00
  // 0123456789ABCDEF0123
  scrbuf.println(tmstr.substring(0x0b, 0x10));

  // メーター
  int xp = 110;
  int yp = 25;
  int st = 90 + 45;
  int en = 360 + 45;
  int ea = st + (en - st) * h / 100;
  scrbuf.fillArc(xp, yp, 14, 10, st, en, TFT_DARKGRAY);
  scrbuf.fillArc(xp, yp, 14, 10, st, ea, col);
  int l = 18;
  double hr = st + (en - st) * (double)OwnTemp.hum / 100;

  double x = (cos(hr * PI / 180) * l);
  double y = (sin(hr * PI / 180) * l);
  scrbuf.drawArc(xp, yp, 18, 18, st, (int)hr, col);
  scrbuf.drawLine((int)(xp + x * 0.1), (int)(yp + y * 0.1), (int)(xp + x), (int)(yp + y), col);
  scrbuf.drawCircle(xp, yp, 6, col);
  // 湿度
  scrbuf.setCursor(130, 6);
  scrbuf.setFont(&fonts::lgfxJapanGothic_36);
  if ((b) && (mode == 1))
  {
    scrbuf.setTextColor(TFT_YELLOW);
  }
  if (mode == -1)
  {
    scrbuf.print("--.-");
  }
  else
  {
    scrbuf.printf("%.1f", h);
  }
  scrbuf.setFont(&fonts::lgfxJapanGothic_24);
  scrbuf.setCursor(205, 16);
  scrbuf.print("%");
}
// ==== フッター表示の更新 ====
void footorPrint(int col)
{

  scrbuf.fillScreen(TFT_BLACK);

  int bitV = (nextTime >> 3) & 0b11111;
  for (int i = 0; i < 5; i++)
  {
    if (bitV & 0x01 == 0x01)
    {
      scrbuf.fillRect(20 + i * 30, 2, 24, 6, col);
    }
    bitV = bitV >> 1;
    scrbuf.drawRect(20 + i * 30, 2, 24, 6, col);
  }
  scrbuf.setCursor(180, 0);
  scrbuf.setFont(&fonts::lgfxJapanGothic_8);
  scrbuf.setTextColor(TFT_RED);
  scrbuf.setTextSize(1);
  scrbuf.println(WiFi.localIP().toString());
}
// ==== 画面表示の更新 ====
void PrintScrren()
{
  int col = TFT_WHITE;
  if (WiFi.status() != WL_CONNECTED)
  {
    col = TFT_RED;
  }
  headbudPrint(col);
  headbuf.pushSprite(0, 0);

  int idx = 0;
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    scrbudPrint(i);
    scrbuf.pushSprite(0, SCR_HEAD_HEIGHT + SCR_LINE_HEIGHT * i);
  }

  brTime = millis() + SCR_DOWN_MSEC;
  footorPrint(TFT_WHITE);
  scrbuf.pushSprite(0, SCR_HEAD_HEIGHT + SCR_LINE_HEIGHT * 6);
  display.setBrightness(SCR_BR_MAX);
}

// ==== 未受信カウント更新 ====
void incrementMissedCounts()
{
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (clients[i].id != "")
    {
      clients[i].missedCount++;
    }
  }
}
//---------------------------------------------------
void SerialSelect()
{
  bool isSend = false;
  String tx = fsu.GetSerialReadAll();
  tx.trim();
  if (tx == "")
    return;
  bool ok = false;
  JsonDocument jd = fsu.GetJsonData(tx, &ok);
  if (ok == false)
  {
    if (tx == "getid")
    {
      Serial.print(fsu.getBoardName(""));
    }
    else if (tx == "getstatus")
    {
      Serial.print(fsu.EPS_Status_json());
    }
    else
    {
      Serial.print("return:" + tx);
    }
    return;
  }
  JsonDocument res;
  if (fsu.JsonCMDCheck(jd, &res))
  {
    isSend = true;
    if (!jd["setBr"].isNull())
    {
      int br = jd["setBr"].as<int>();
      if (br >= 0)
      {
        display.setBrightness(br);
      }
      res["setBr"] = br;
    }
    else if (!jd["getBr"].isNull())
    {
      Serial.printf("brightness:%d", display.getBrightness());
      res["getBr"] = display.getBrightness();
    }
  }

  if (isSend)
  {
    String ret;
    serializeJson(res, ret);
    Serial.print(ret);
  }
}
//---------------------------------------------------
void setup()
{
  Serial.begin(115200);
  setupDisplay();
  display.setBrightness(100);
  setupWiFiAndTime();

  server.begin();

  if (!aht.begin())
  {
    Serial.println("AHT20 が見つかりません。配線チェックして下さい。");
  }
  else
  {
    Serial.println("AHT20  接続確認");
  };
  unsigned status;
  status = bmp.begin(0x77, BMP280_CHIPID);
  if (!status)
  {
    Serial.println("BMP280が見つかりません。配線チェックして下さい。");
  }
  else
  {
    Serial.println("BMP280 接続確認");
    // Default settings from datasheet.
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  }
  nextTime = millis() + SCAN_MSEC;
  errorTime = millis() + SCAN_TIME;
  display.fillScreen(TFT_BLACK);
  readOwnTemp();
  PrintScrren();
}

void loop()
{
  SerialSelect();
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED)
  {
    setupWiFiAndTime();
  }
  WiFiClient client = server.available();
  bool refF = false;
  if (client)
  {
    String json = client.readStringUntil('\n'); // 改行で終端を期待
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (!err)
    {
      String id = doc["id"];
      String timeStr = doc["time"];
      float temp = doc["temp"];
      float hum = doc["hum"];
      updateClientData(id, timeStr, temp, hum);
      refF = true;
    }
    client.stop();
  }
  if (now > nextTime)
  {
    readOwnTemp();
    nextTime = now + SCAN_MSEC;
    refF = true;
  }
  if (now > errorTime)
  {
    incrementMissedCounts();
    errorTime = now + SCAN_TIME;
    refF = true;
  }
  if (now > brTime)
  {
    for (int i = SCR_BR_MAX; i >= SCR_BR_MIN; i--)
    {
      display.setBrightness(i);
      delay(10);
    }
    display.setBrightness(5);
    brTime = now + millis();
    ;
  }
  if (refF)
  {
    PrintScrren();
  }
}
