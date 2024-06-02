#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <string>
#include <Arduino.h>
#include <ctime>
#include <bleManager.h>
#include "esp32RSA.h"
#include "espfs.h"


void requestSetTime(const std::string &requestBody) {
    long timestamp = std::stol(requestBody);
    struct tm timeinfo;
    gmtime_r(&timestamp, &timeinfo);
    timeval tv = {timestamp, 0};
    settimeofday(&tv, NULL);
    Serial.println("Time: " + String(timestamp));
}

void requestAsync(const std::string &requestBody) {
    long timestamp = std::stol(requestBody);
    // open spiffs data.csv
    File file = SPIFFS.open("/data.csv", FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }
    // from 2nd line read time, heartRate, SpO2
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.length() == 0) {
            continue;
        }
        if (line[0] == 'T') {
            continue;
        }
        // split line by comma
        int firstComma = line.indexOf(',');
        int secondComma = line.indexOf(',', firstComma + 1);
        long time = line.substring(0, firstComma).toInt();
        int heartRate = line.substring(firstComma + 1, secondComma).toInt();
        float SpO2 = line.substring(secondComma + 1).toFloat();
        // if time > timestamp, send message
        if (time > timestamp) {
            sendResultsBLE(heartRate, SpO2, 2, time);
            Serial.println("Time: " + String(time) + ", HR: " + String(heartRate) + ", SpO2: " + String(SpO2));
            delay(100);
            // delete line
            //file.seek(file.position() - line.length() - 1);
            //file.print('\n');
        }
    }
    file.close();    

}


#endif // BLE_HANDLER_H
