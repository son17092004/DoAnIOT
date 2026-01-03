/*
 * ============================================================
 * HELPER: LẤY MAC ADDRESS CỦA ESP32
 * ============================================================
 * 
 * Chương trình đơn giản để lấy MAC Address của ESP32
 * Cần thiết để cấu hình ESP-NOW giữa 2 ESP32
 * 
 * CÁCH SỬ DỤNG:
 * 1. Nạp code này vào ESP32 (LCD hoặc CAM)
 * 2. Mở Serial Monitor (115200 baud)
 * 3. Sao chép MAC Address hiển thị
 * 4. Cập nhật MAC Address vào code của ESP32 còn lại
 * 
 * VÍ DỤ OUTPUT:
 * ESP32 MAC Address: 24:6F:28:AA:BB:CC
 * Array format: {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}
 * 
 * Files liên quan:
 * - esp32/testlcd/testlcd.ino (cần MAC của ESP32-CAM)
 * - esp32/camera_client/camera_client.ino (có thể thêm MAC của ESP32-LCD nếu cần 2-way)
 * ============================================================
 */

#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("ESP32 MAC Address Finder");
  Serial.println("========================================\n");
  
  // Set WiFi to Station mode để lấy MAC address
  WiFi.mode(WIFI_STA);
  delay(100);
  
  // Lấy MAC Address
  String macAddress = WiFi.macAddress();
  
  Serial.println("ESP32 MAC Address:");
  Serial.println(macAddress);
  Serial.println();
  
  // Chuyển đổi sang array format để dễ copy vào code
  Serial.println("Array format (copy vào code ESP-NOW):");
  Serial.print("uint8_t macAddress[] = {");
  
  // Parse MAC address string thành hex array
  for (int i = 0; i < macAddress.length(); i++) {
    if (macAddress[i] == ':') continue;
    
    Serial.print("0x");
    Serial.print(macAddress[i]);
    i++;
    if (i < macAddress.length()) {
      Serial.print(macAddress[i]);
    }
    
    if (i < macAddress.length() - 1) {
      Serial.print(", ");
    }
  }
  
  Serial.println("};");
  Serial.println();
  Serial.println("========================================");
  Serial.println("Hướng dẫn sử dụng:");
  Serial.println("1. Nạp code này vào ESP32-CAM");
  Serial.println("2. Copy MAC Address ở trên");
  Serial.println("3. Paste vào file testlcd.ino:");
  Serial.println("   uint8_t camMacAddress[] = {...};");
  Serial.println("========================================");
}

void loop() {
  // Hiển thị lại MAC mỗi 5 giây
  delay(5000);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

