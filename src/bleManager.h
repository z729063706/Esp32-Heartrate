#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>
#include <time.h>
#include <ArduinoJson.h>
#include "esp32RSA.h"
#include "espfs.h"
#include <ctime>


const char* deviceName;
std::string serviceUUID;
std::string characteristicUUID;
bool deviceConnected = false;
bool deviceInited = false;
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

void sendMessage(const std::string& message);
void loopBLE();
void configureBLEEncryption();
void requestHandler(const std::string &request);
void sendResultsBLE(int heartRate, float SpO2, int msgType, long timestamp);
void requestAsync(const std::string &requestBody);
void requestSetTime(const std::string &requestBody);

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
        BLEDevice::getAdvertising()->stop();
        Serial.println("Device connected");
    }

    void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
        BLEDevice::getAdvertising()->start();
        Serial.println("Device disconnected");
    }
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
private:
    std::string completeValue;  // 用于存储接收到的完整数据
    int expectedLength = 0;     // 期望接收的数据长度

public:
    void onWrite(BLECharacteristic *pCharacteristic) override {
        //Serial.println("onWrite callback triggered");

        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            //Serial.print("Received Value: ");
            // for (int i = 0; i < rxValue.length(); i++) {
            //     Serial.print(rxValue[i]);
            // }
            // Serial.println();

            // 如果还没有接收到长度信息，则从前几个字节中提取长度信息
            if (expectedLength == 0) {
                if (rxValue.length() >= 4) {
                    expectedLength = (rxValue[0] << 24) | (rxValue[1] << 16) | (rxValue[2] << 8) | rxValue[3];
                    rxValue = rxValue.substr(4);  // 去掉长度字段
                } else {
                    Serial.println("Error: Length field is incomplete.");
                    return;
                }
            }

            // 累加接收到的数据
            completeValue += rxValue;

            // 检查是否接收到完整的数据包
            if (completeValue.length() >= expectedLength) {
                requestHandler(completeValue);
                completeValue.clear();  // 清空缓存的字符串，为下一个数据包做准备
                expectedLength = 0;     // 重置期望长度
            }
        } else {
            Serial.println("Received an empty value or failed to read the value.");
        }
    }
};

class MyBLESecurityCallbacks : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() override {
        Serial.println("PassKeyRequest");
        return 123456; // default passkey
    }

    void onPassKeyNotify(uint32_t pass_key) override {
        Serial.println("PassKeyNotify");
        Serial.print("PassKey: ");
        Serial.println(pass_key);
    }

    bool onSecurityRequest() override {
        Serial.println("SecurityRequest");
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override {
        if (cmpl.success) {
            Serial.println("Authentication success");
        } else {
            Serial.print("Authentication failed, reason: ");
            Serial.println(cmpl.fail_reason);
        }
    }

    bool onConfirmPIN(uint32_t pin) override {
        Serial.print("ConfirmPINRequest: ");
        Serial.println(pin);
        return true;
    }
};

void beginBLE(const char* devName, const std::string& servUUID, const std::string& charUUID) {
    deviceName = devName;
    serviceUUID = servUUID;
    characteristicUUID = charUUID;

    BLEDevice::init(deviceName);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(serviceUUID);

    pCharacteristic = pService->createCharacteristic(
                        characteristicUUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );

    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new CharacteristicCallbacks());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("Waiting for a client connection to notify...");
}

void sendMessage(const std::string& message) {
    if (deviceConnected) {
        const size_t chunkSize = 20;
        size_t messageLength = message.length();
        for (size_t i = 0; i < messageLength; i += chunkSize) {
            std::string chunk = message.substr(i, chunkSize);
            pCharacteristic->setValue(chunk);
            pCharacteristic->notify();
            delay(10);  // 确保每块消息有足够时间发送
        }
        Serial.print("Sent Value: ");
        Serial.println(message.c_str());
    }
}

void loopBLE() {
    // 这里可以添加你需要在主循环中执行的代码
}

void configureBLEEncryption() {
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLEDevice::setSecurityCallbacks(new MyBLESecurityCallbacks());
    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
}

void requestHandler(const std::string &request) {
    Serial.print("Request:");
    Serial.println(request.c_str());
    std::string decryptedData = std::string(decryptData(request.c_str()).c_str());
    std::string messageType = decryptedData.substr(0, 2);
    std::string requestBody = decryptedData.substr(3);
    Serial.print("Message Type: ");
    Serial.println(messageType.c_str());
    if (messageType == "00") {
       requestSetTime(requestBody);
       deviceInited = true;
    } else if (messageType == "01") {
        requestAsync(requestBody);
    } else {
        Serial.println("Unknown message type");
    }
}


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
            //Serial.println("Time: " + String(time) + ", HR: " + String(heartRate) + ", SpO2: " + String(SpO2));
            delay(1000);
            // delete line
            //file.seek(file.position() - line.length() - 1);
            //file.print('\n');
        }
    }
    file.close();    

}

void sendResultsBLE(int heartRate, float SpO2, int msgType, long timestamp) {
    if (timestamp == 0) {
        timestamp = time(nullptr);
    }
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["msgType"] = msgType;
    jsonDoc["timestamp"] = timestamp;
    jsonDoc["heartRate"] = heartRate;
    jsonDoc["SpO2"] = SpO2;

    char jsonBuffer[200];
    serializeJson(jsonDoc, jsonBuffer);

    sendMessage(jsonBuffer);
}

#endif // BLE_MANAGER_H
