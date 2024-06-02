#ifndef ESPFS_H
#define ESPFS_H

#include <SPIFFS.h>
#include <Arduino.h>

void initSPIFFS() {
  if (!SPIFFS.begin(true)) { // 强制格式化
    Serial.println("SPIFFS Mount Failed, attempting to format");
    if (!SPIFFS.format()) {
      Serial.println("SPIFFS Format Failed");
      return;
    }
    // 尝试再次挂载
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed after format");
      return;
    }
  }
  Serial.println("SPIFFS Mount Successful");

  // 创建CSV文件并写入标题行（如果文件不存在）
  File file = SPIFFS.open("/data.csv", FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.size() == 0) {
    file.println("Time,HeartRate,SpO2");
  }
  file.close();
}

void saveMeasurementToSPIFFS(int heartRate, double SpO2) {
  File file = SPIFFS.open("/data.csv", FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.printf("%ld,%d,%f\n", time(nullptr), heartRate, SpO2);
  file.close();
}

void readDataFromSPIFFS() {
  File file = SPIFFS.open("/data.csv", FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("Reading from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void deleteDataFromSPIFFS(String filename) {
  if (SPIFFS.exists(filename)) {
    SPIFFS.remove(filename);
  }
}

#endif // ESPFS_H
