#include <WiFi.h>
#include <HTTPClient.h>
#include <mbedtls/aes.h>
#include "Base64.h"
#include <time.h>


const char* ssid = "WIFI_NAME";
const char* password = "WIFI_PASSWORD";
const char* serverUrl = "http://192.168.1.7:5000/webhook";

const uint8_t aesKey[16] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F
};

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("WiFi connection loading..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // Türkiye saati için GMT+3
  Serial.println("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  while (now < 100000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime synchronized.");

}

String generateSensorData() {
  float temperature = random(200, 300) / 10.0;
  int humidity = random(30, 60);
  String sensorId = "esp32_1";

  time_t now = time(nullptr);

  String jsonData = "{\"temperature\":" + String(temperature, 1) +
                    ",\"humidity\":" + String(humidity) +
                    ",\"sensor_id\":\"" + sensorId + "\"" +
                    ",\"timestamp\":" + String((unsigned long)now) + "}";
  return jsonData;
}

String encryptAES128(String plainText) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  
  size_t inputLen = plainText.length();
  size_t paddedLen = ((inputLen + 15) / 16) * 16;
  uint8_t input[paddedLen];
  uint8_t padValue = paddedLen - inputLen;
memcpy(input, plainText.c_str(), inputLen);
for (size_t i = inputLen; i < paddedLen; ++i) {
  input[i] = padValue;
}
  memcpy(input, plainText.c_str(), inputLen);

  uint8_t output[paddedLen];
  mbedtls_aes_setkey_enc(&aes, aesKey, 128);

  for (size_t i = 0; i < paddedLen; i += 16) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input + i, output + i);
  }

  mbedtls_aes_free(&aes);

  String encoded = base64::encode(output, paddedLen);
  return encoded;
}

void sendEncryptedData(const String& encryptedData) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "text/plain");

    int httpResponseCode = http.POST(encryptedData);

    Serial.print("HTTP answer: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());

    http.end();
  } else {
    Serial.println("There is no wifi connection.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  connectToWiFi();

  Serial.begin(115200);
  Serial.println(WiFi.macAddress());
  
}

void loop() {
  String sensorData = generateSensorData();
  Serial.println("Original data: " + sensorData);

  String encrypted = encryptAES128(sensorData);
  Serial.println("Crypted data: " + encrypted);

  sendEncryptedData(encrypted);

  delay(20000);
}
