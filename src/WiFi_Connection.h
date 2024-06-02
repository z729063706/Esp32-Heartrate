#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <WiFi.h>
#include <time.h>

const char* ssid = "PanSiAiDong";
const char* password = "808808808";
const char* ntpServer = "192.168.1.1";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

void initWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void initNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("NTP server set.");

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "Time synchronized: %Y-%m-%d %H:%M:%S");
}

#endif // WIFI_CONNECTION_H
