/*
 * TEST ESP-NOW CALLBACKS - ESP32 Core v3.x
 * Dùng để verify callback signatures cho ESP32 Arduino Core 3.x
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Callback cho ESP32 Core v3.x
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
  Serial.print("Received ");
  Serial.print(len);
  Serial.println(" bytes");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  
  Serial.println("ESP-NOW callbacks registered successfully!");
}

void loop() {
  delay(1000);
}

