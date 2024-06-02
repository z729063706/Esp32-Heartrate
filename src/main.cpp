#include <Arduino.h>
#include <Adafruit_GFX.h>    //OLED libraries
#include <Adafruit_SSD1306.h> //OLED libraries
#include "MAX30105.h"           //MAX3010x library
#include "heartRate.h"          //Heart rate calculating algorithm
#include "display_1306.h"
#include "MAX30102.h"
#include "espfs.h"
//#include "WiFi_Connection.h"
#include "bleManager.h"
#include "nvsPUF.h"
#include "esp32RSA.h"


//const char* encrypted = "XhXEYtCepW0JpbS8rG73Mpy8fbH8eEN0UTS38jdoRvkYPwZvO4WwUDKj6vSb044JIdR2bO0U9lpNL5VZhcij+v0s9EZwtRG3F2siTMzg302c6Uk2o+fDsKTof6A59tYtfd3SSViwTWtMyuDkmqSviH97l8F2RWNcZNHAaq1wIo5uo5Ll8Ue3e4HfxwcF+Yk3QQGnXiPbw+zBBd1CymvPi1i4anYrzSkDQt5Gd6c6lgc1SlswQOPG5xuIBB8zf/Mt4YtnYpw2Li5qNqw9nGA4pqcLGd7NyWoA/LEJ8/VMo29WuKLSucXugRsCkmA0lKZWKWfuQIYBdwkE0j9yMkFvlw==";
const char* encrypted = "B32QoVMIIIRkoP+mNRHI/Mk8gPyXXTPjcAT0GLCDyHkTzHf0INHHVexLOQm+i33TvS6ZXEvhz2s0xrlFTX0Oh3EHgeFqcF4w3GnscE6uXi65pikN1QNLAfQuyCexazvzHktY2BBmoizsow+00wGA0ozbsLspDhuOOi6p563ZJxMl8rtTbvxwPj9GvGyuyXhw5TmA6ztveHIg9LoRrRCkmPMFPY3ub1SLmU3gYvoeFdrvyoZbSTQm9GsS0rHP5+fDipZl27p1hB53FWTwRQanNSIQ1meSoNU8D7cjhJVLi+F59FdPkGpsgb6JEijYYKOS//WuYUPUQLrUgesQJMd6UQ==";

void setup() {
  Serial.begin(115200);
  Serial.println("System Start");
  displayInit();
  MAX30102Init();
  displayLogo();
  initSPIFFS();
  //Serial.println(decryptData(encrypted));
  beginBLE("HeartMoniter-0687", "12345678-1234-1234-1234-123456789012", "87654321-4321-4321-4321-210987654321");
  Serial.println("BLE initialized");
  configureBLEEncryption();

  while (!deviceConnected || !deviceInited) {
      delay(100);
  }
  Serial.println("Device connected, proceeding with setup...");
  displayNoFinger();
}

void loop() {
  int heartRate = measureHR(10);
  float SpO2 = measureSpO2(100);
  if (heartRate>0 && SpO2>80){
    displayHeart(heartRate);
    displaySpo2(SpO2);
    saveMeasurementToSPIFFS(heartRate, SpO2);
    sendResultsBLE(heartRate, SpO2, 1, 0);
  }
  else if (heartRate==-1){
    displayNoFinger();
  }
  delay(10000);
  loopBLE();
}

