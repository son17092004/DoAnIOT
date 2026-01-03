/*
 * ============================================================
 * ESP32-CAM - CAMERA CLIENT với ESP-NOW
 * ============================================================
 * Chức năng:
 * - Nhận trigger từ ESP32 LCD qua ESP-NOW
 * - Chụp ảnh khi nhận trigger (không còn polling!)
 * - Bật flash LED khi chụp trong môi trường tối
 * - Upload ảnh lên server qua HTTP
 * 
 * Files liên quan:
 * - esp32/testlcd/testlcd.ino (ESP32 LCD sender)
 * - backend/main.py (Server nhận diện)
 * 
 * Hardware:
 * - ESP32-CAM AI Thinker
 * - Flash LED: GPIO 4
 * ============================================================
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_now.h>

// ==========================================
// CONFIGURATION
// ==========================================
const char* ssid = "conmeo";
const char* password = "meomeomeo";
const char* serverHost = "192.168.252.107";
const int serverPort = 8080;

// Pin definition for AI THINKER Model
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_LED_PIN     4

// State
volatile bool triggerReceived = false;
bool cameraReady = false;
unsigned long lastCaptureTime = 0;
const unsigned long CAPTURE_COOLDOWN = 5000; // 5 giây cooldown giữa các lần chụp

// ============================================================
// ESP-NOW Callback: Nhận trigger từ ESP32 LCD
// ============================================================
// ESP32 Core v3.x compatible
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
  if (len >= 1 && data[0] == 0x01) { // 0x01 = Trigger command
    Serial.println("ESP-NOW: Trigger received from LCD!");
    triggerReceived = true;
    
    // Gửi ACK về ESP32 LCD (dùng src_addr từ recv_info)
    uint8_t ack[2] = {0xAA, 0xBB}; // Header xác nhận
    esp_now_send(recv_info->src_addr, ack, sizeof(ack));
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32-CAM Starting ===");
  
  // Flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  
  // Khởi tạo WiFi (STA mode cho ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Hiển thị MAC Address (quan trọng để cấu hình ESP32 LCD)
  Serial.print("ESP32-CAM MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Kết nối WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
    // Có thể tiếp tục với ESP-NOW, nhưng không thể upload ảnh
  }

  // Khởi tạo ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while(1) {
      digitalWrite(FLASH_LED_PIN, HIGH);
      delay(200);
      digitalWrite(FLASH_LED_PIN, LOW);
      delay(200);
    }
  }
  
  Serial.println("ESP-NOW initialized successfully");
  
  // Đăng ký callback nhận data
  esp_now_register_recv_cb(onDataRecv);

  // Cấu hình camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Cấu hình frame size dựa trên PSRAM - TĂNG TỐC
  if(psramFound()){
    config.frame_size = FRAMESIZE_CIF;  // 400x296 (tối ưu: nhỏ hơn nhưng đủ cho face recognition)
    config.jpeg_quality = 12;           // 0-63, tăng lên 12 để giảm dung lượng
    config.fb_count = 2;
    Serial.println("PSRAM found - Using CIF resolution (optimized)");
  } else {
    config.frame_size = FRAMESIZE_CIF;  // 400x296 (an toàn cho cả không PSRAM)
    config.jpeg_quality = 15;
    config.fb_count = 1;
    Serial.println("No PSRAM - Using CIF resolution (optimized)");
  }

  // Khởi tạo camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    cameraReady = false;
    
    // Blink LED để báo lỗi
    while(1) {
      for(int i = 0; i < 3; i++) {
        digitalWrite(FLASH_LED_PIN, HIGH);
        delay(100);
        digitalWrite(FLASH_LED_PIN, LOW);
        delay(100);
      }
      delay(1000);
    }
  }
  
  Serial.println("Camera initialized successfully!");
  cameraReady = true;
  
  // Cải thiện chất lượng ảnh
  sensor_t * s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_sharpness(s, 0);      // -2 to 2
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  }
  
  Serial.println("=== System Ready - Waiting for ESP-NOW trigger ===");
  
  // Blink LED để báo sẵn sàng
  for(int i = 0; i < 2; i++) {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(100);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(100);
  }
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  // Chỉ chụp ảnh khi:
  // 1. Nhận được trigger từ ESP-NOW
  // 2. Camera ready
  // 3. Đã qua thời gian cooldown (tránh spam)
  
  if (triggerReceived && cameraReady) {
    unsigned long now = millis();
    
    // Kiểm tra cooldown
    if (now - lastCaptureTime >= CAPTURE_COOLDOWN) {
      triggerReceived = false; // Reset flag
      lastCaptureTime = now;   // Cập nhật thời gian
      
      Serial.println("=== Processing trigger ===");
      captureAndUpload();
    } else {
      // Còn trong cooldown period
      triggerReceived = false; // Reset flag nhưng không chụp
      unsigned long remaining = (CAPTURE_COOLDOWN - (now - lastCaptureTime)) / 1000;
      Serial.printf("Cooldown active, wait %lu more seconds\n", remaining);
    }
  }
  
  // ESP-NOW callback xử lý trigger, không cần polling
  delay(5);  // Giảm CPU usage (tối ưu: 10ms → 5ms)
}

// ============================================================
// Helper: Kiểm tra độ sáng ảnh
// ============================================================
bool isImageDark(camera_fb_t *fb) {
  if (!fb || fb->len == 0) return false;
  
  // Lấy mẫu 100 pixel từ ảnh để tính độ sáng trung bình
  uint32_t brightness = 0;
  int sampleCount = 0;
  int step = fb->len / 100; // Lấy mẫu 100 điểm
  
  for (size_t i = 0; i < fb->len && sampleCount < 100; i += step) {
    brightness += fb->buf[i];
    sampleCount++;
  }
  
  uint8_t avgBrightness = brightness / sampleCount;
  Serial.printf("Average brightness: %d\n", avgBrightness);
  
  return avgBrightness < 60; // Threshold: dưới 60 = tối
}

// ============================================================
// Chụp ảnh và upload lên server
// ============================================================
void captureAndUpload() {
  unsigned long startTime = millis();
  
  // Thử chụp ảnh lần đầu
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed!");
    return;
  }
  
  // Kiểm tra độ sáng, nếu tối thì bật flash và chụp lại
  bool needFlash = isImageDark(fb);
  
  if (needFlash) {
    Serial.println("Image too dark, retaking with flash...");
    
    // Trả về buffer ảnh cũ
    esp_camera_fb_return(fb);
    
    // Bật flash
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(100); // Chờ flash ổn định (tối ưu: giảm từ 200ms xuống 100ms)
    
    // Chụp lại
    fb = esp_camera_fb_get();
    
    // Tắt flash
    digitalWrite(FLASH_LED_PIN, LOW);
    
    if(!fb) {
      Serial.println("Camera capture with flash failed!");
      return;
    }
  }
  
  Serial.printf("Image captured: %d bytes in %lu ms\n", fb->len, millis() - startTime);
  
  // Upload lên server
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected! Cannot upload.");
    esp_camera_fb_return(fb);
    return;
  }
  
  WiFiClient client;
  if (client.connect(serverHost, serverPort)) {
    Serial.println("Uploading to server...");
    
    String boundary = "----ESP32CAMBoundary1234567890";
    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"file\"; filename=\"cam.jpg\"\r\n";
    head += "Content-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";
    
    uint32_t totalLen = head.length() + fb->len + tail.length();
    
    // HTTP Headers
    client.println("POST /api/recognize HTTP/1.1");
    client.println("Host: " + String(serverHost));
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println("Connection: close");
    client.println();
    
    // Body
    client.print(head);
    
    // Gửi image data theo chunks (TĂNG TỐC)
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    size_t chunkSize = 2048;  // Tăng từ 1024 → 2048 bytes (upload nhanh hơn)
    
    for (size_t i = 0; i < fbLen; i += chunkSize) {
      size_t toSend = (fbLen - i < chunkSize) ? (fbLen - i) : chunkSize;
      size_t sent = client.write(fbBuf + i, toSend);
      
      if (sent != toSend) {
        Serial.println("Upload failed: connection error");
        break;
      }
    }
    
    client.print(tail);
    
    // Đọc response từ server (TĂNG TỐC)
    Serial.println("Waiting for server response...");
    unsigned long timeout = millis();
    bool headersPassed = false;
    String response = "";
    
    while (client.connected() && millis() - timeout < 8000) {  // Giảm từ 10s → 8s
      if (client.available()) {
        String line = client.readStringUntil('\n');
        
        if (!headersPassed) {
          if (line == "\r") {
            headersPassed = true;
          } else if (line.startsWith("HTTP/1.1")) {
            Serial.println("Server response: " + line);
          }
        } else {
          response += line;
        }
        
        timeout = millis(); // Reset timeout khi nhận data
      }
    }
    
    if (response.length() > 0) {
      Serial.println("Response body: " + response);
    }
    
    client.stop();
    Serial.printf("Upload completed in %lu ms\n", millis() - startTime);
    
  } else {
    Serial.println("Failed to connect to server!");
  }
  
  // Giải phóng buffer
  esp_camera_fb_return(fb);
}
