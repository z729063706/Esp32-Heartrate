#ifndef MAX30102_H
#define MAX30102_H
#include <Wire.h>               //I2C library
#include "MAX30105.h"           //MAX3010x library
#include "heartRate.h"          //Heart rate calculating algorithm
#include "bleManager.h"
#include <Arduino.h>

const int SAMPLE_RATE = 50;

MAX30105 particleSensor;

#define FINGER_ON 12000    //紅外線最小量（判斷手指有沒有上）
#define MINIMUM_SPO2 90.0 //血氧最小量

void sendResultsBLE(int heartRate, float SpO2, int msgType, long timestamp);

void MAX30102Init() {
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("MAX30102 found!");
  byte ledBrightness = 0x7F; //亮度建議=127, Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only(心跳), 2 = Red + IR(血氧)
  int sampleRate = 200; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 215; //Options: 69, 118, 215, 411
  int adcRange = 16384; //Options: 2048, 4096, 8192, 16384
  // Set up the wanted parameters
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  particleSensor.enableDIETEMPRDY();

  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

int measureHR(int samplecount) {
  unsigned long teatimes[samplecount];
  int beatIndex = 0;
  static unsigned long beattime = 0;
  particleSensor.wakeUp();
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();
  if (irValue < FINGER_ON) {
    Serial.println("Finger not on sensor");
    particleSensor.shutDown();
    return -1;
  }
  while (beatIndex < samplecount) {
    irValue = particleSensor.getIR(); 
    if (checkForBeat(irValue)) {
      teatimes[beatIndex++] = millis();
      Serial.print(".");
    }
    else if (irValue < FINGER_ON){
      Serial.println("");
      break;
    }
  }
  Serial.println("");
  particleSensor.shutDown();
  if (beatIndex != samplecount){
    Serial.println("Not enough data");
    return -1;
  }
  long beatstime = teatimes[beatIndex - 1] - teatimes[0];
  float onebeat = (float)beatstime / (float)(beatIndex - 1);
  int heartRate = 60000 / onebeat;
  return heartRate;
}

float measureSpO2(int sampleCount) {
  long irValues[sampleCount];
  long redValues[sampleCount];
  int beatIndex = 0;
  particleSensor.wakeUp();
  long irValue = particleSensor.getIR();
  if (irValue < FINGER_ON) {
    Serial.println("Finger not on sensor");
    particleSensor.shutDown();
    return -1;
  }

  while (beatIndex < sampleCount) {
    irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();
    if (irValue < FINGER_ON) {
      break;
    }
    irValues[beatIndex] = irValue;
    redValues[beatIndex] = redValue;
    beatIndex++;
    delay(1000 / SAMPLE_RATE); // 控制采样率
  }
  particleSensor.shutDown();
  // 检查是否采集到足够的样本
  if (beatIndex != sampleCount) {
    Serial.println("Not enough data");
    return -1;
  }

  // 计算直流成分
  double redDC = 0;
  double irDC = 0;
  for (int i = 0; i < sampleCount; i++) {
    redDC += redValues[i];
    irDC += irValues[i];
  }
  redDC /= sampleCount;
  irDC /= sampleCount;

  // 计算交流成分的平方和
  double redACsqSum = 0;
  double irACsqSum = 0;
  for (int i = 0; i < sampleCount; i++) {
    redACsqSum += (redValues[i] - redDC) * (redValues[i] - redDC);
    irACsqSum += (irValues[i] - irDC) * (irValues[i] - irDC);
  }
  redACsqSum /= sampleCount;
  irACsqSum /= sampleCount;

  // 计算R值
  double R = (sqrt(redACsqSum) / redDC) / (sqrt(irACsqSum) / irDC);

  // 计算血氧饱和度
  float SpO2 = -23.3 * (R - 0.4) + 100;
  return SpO2;
}


#endif // MAX30102_H
