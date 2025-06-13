#pragma once
#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <Arduino.h>

#define SERVER_PORT 12345

// 子機データ構造
struct SensorData
{
    String id;
    String timeStr; // 受信した日時（文字列）
    float temp;
    float hum;
    float pres;
    uint8_t missedCount = 0;
};

#endif