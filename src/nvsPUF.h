#ifndef NVSPUF_H
#define NVSPUF_H

#include <Preferences.h>

Preferences preferences;

void setupNVS() {
    // 初始化NVS
    preferences.begin("storage", false);
}

void closeNVS() {
    // 关闭NVS
    preferences.end();
}

void writePrivateKeyToNVS(const char* privateKey) {
    preferences.putString("private_key", privateKey);
}

void writePublicKeyToNVS(const char* publicKey) {
    preferences.putString("public_key", publicKey);
}

String readPrivateKeyFromNVS() {
    return preferences.getString("private_key", "");
}

String readPublicKeyFromNVS() {
    return preferences.getString("public_key", "");
}

#endif // NVSPUF_H
