#ifndef ESP32RSA_H
#define ESP32RSA_H

#include <Arduino.h>
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/base64.h"
#include "nvsPUF.h"


String decryptData(const char* encryptedData);

String decryptData(const char* encryptedData) {
    setupNVS();
    Serial.println("Reading private key from NVS");
    String privateKeyStr = readPrivateKeyFromNVS();
    if (privateKeyStr.length() == 0) {
        Serial.println("Private key not found");
        return "";
    }

    const char* privateKey = privateKeyStr.c_str();

    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char* pers = "rsa_decrypt";
    unsigned char decrypted[512];
    size_t decrypted_len = 0;

    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Seed the random number generator
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)pers, strlen(pers));
    if (ret != 0) {
        Serial.print("Failed to seed the random number generator, error code: ");
        Serial.println(ret);
        return "";
    }

    // Parse the private key
    ret = mbedtls_pk_parse_key(&pk, (const unsigned char*)privateKey, privateKeyStr.length() + 1, NULL, 0);
    if (ret != 0) {
        Serial.print("Failed to parse private key, error code: ");
        Serial.println(ret);
        return "";
    }

    // Decode base64 encoded encrypted data
    size_t encryptedDataLen = strlen(encryptedData);
    unsigned char* encryptedBytes = (unsigned char*)malloc(encryptedDataLen);
    if (encryptedBytes == NULL) {
        Serial.println("Failed to allocate memory for encrypted bytes");
        return "";
    }

    size_t decodedLen = 0;
    ret = mbedtls_base64_decode(encryptedBytes, encryptedDataLen, &decodedLen, (const unsigned char*)encryptedData, encryptedDataLen);
    if (ret != 0) {
        Serial.print("Failed to decode base64 encoded data, error code: ");
        Serial.println(ret);
        free(encryptedBytes);
        return "";
    }

    // Decrypt the data
    ret = mbedtls_pk_decrypt(&pk, encryptedBytes, decodedLen, decrypted, &decrypted_len, sizeof(decrypted), mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        Serial.print("Failed to decrypt data, error code: ");
        Serial.println(ret);
        free(encryptedBytes);
        return "";
    }

    free(encryptedBytes);
    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);

    // Return the decrypted data as a String
    return String((char*)decrypted).substring(0, decrypted_len);
}


#endif // ESP32RSA_H
