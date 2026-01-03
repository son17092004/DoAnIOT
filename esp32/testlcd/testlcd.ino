/*
 * ============================================================
 * ESP32 MAIN CONTROLLER - RADAR + LCD + SPEAKER
 * ============================================================
 * Chức năng:
 * - Phát hiện chuyển động bằng radar
 * - Gửi trigger qua ESP-NOW đến ESP32-CAM
 * - Nhận kết quả nhận diện qua ESP-NOW
 * - Hiển thị kết quả trên LCD 16x2
 * - Phát âm thanh phản hồi
 * 
 * Files liên quan:
 * - esp32/camera_client/camera_client.ino (ESP32-CAM receiver)
 * - backend/main.py (Server nhận diện)
 * 
 * Kết nối phần cứng:
 * - Radar: GPIO 4
 * - Speaker: GPIO 25 (PWM)
 * - LCD I2C: SDA=21, SCL=22, Address=0x27
 * ============================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <esp_wifi.h> // Thêm để có wifi_tx_info_t cho ESP32 Core v3.x

// Hardware Pins
#define RADAR_OUT   4
#define SPEAKER_PIN 25  // Buzzer nối vào GPIO 25

// WiFi Settings (cần để lấy MAC và nhận kết quả từ server)
const char* ssid = "conmeo";
const char* password = "meomeomeo";
const char* serverUrlResult = "http://192.168.252.107:8080/api/result/latest";

// MAC Address của ESP32-CAM (cần cập nhật sau khi chạy get_mac_address.ino)
// Ví dụ: {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}
uint8_t camMacAddress[] = {0x80, 0xF3, 0xDA, 0x5F, 0xEC, 0x44}; // ⚠️ CẬP NHẬT MAC ADDRESS ESP32-CAM TẠI ĐÂY

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// State Management
enum SystemState {
  IDLE,           // Đang chờ người
  DETECTING,      // Đã phát hiện, chờ radar ổn định
  PROCESSING,     // Đang chụp ảnh + nhận diện
  SHOWING_RESULT, // Đang hiển thị kết quả
  COOLDOWN        // Cooldown sau khi xử lý xong
};

SystemState currentState = IDLE;
unsigned long stateStartTime = 0;
unsigned long lastTriggerTime = 0;
bool espNowReady = false;

// Radar debounce
bool lastRadarState = LOW;
unsigned long lastRadarChangeTime = 0;
const unsigned long RADAR_DEBOUNCE = 500; // 500ms debounce cho radar

// Timing constants
const unsigned long TRIGGER_COOLDOWN = 3000;     // 3s cooldown sau mỗi lần xử lý
const unsigned long RESULT_DISPLAY_TIME = 4000;  // 4s hiển thị kết quả
const unsigned long PROCESSING_TIMEOUT = 20000;  // 20s timeout cho xử lý

// ============================================================
// ESP-NOW Callback: Nhận kết quả từ ESP32-CAM
// ============================================================
// ESP32 Core v3.x compatible
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
  if (len > 0 && data[0] == 0xAA) { // Header byte để xác nhận đúng message
    // Đã nhận được xác nhận từ CAM
    Serial.println("ESP-NOW: CAM received trigger!");
  }
}

// ============================================================
// ESP-NOW Callback: Xác nhận gửi thành công
// ============================================================
// ESP32 Core v3.x - API mới dùng wifi_tx_info_t thay vì mac address
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("ESP-NOW: Trigger sent successfully!");
  } else {
    Serial.println("ESP-NOW: Failed to send trigger!");
    lcd.setCursor(0, 1);
    lcd.print("Loi ESP-NOW!    ");
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  // TẮT LOA NGAY LẬP TỨC trước khi làm gì khác
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);
  
  Serial.begin(115200);
  delay(1000);

  // Radar
  pinMode(RADAR_OUT, INPUT);
  
  // KHÔNG dùng PWM cho Active Buzzer
  // Nếu dùng Passive Buzzer, uncomment dòng dưới:
  // ledcAttach(SPEAKER_PIN, 2000, 8);

  // LCD
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Khoi dong...");

  // WiFi Connect (STA mode cho ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  lcd.setCursor(0, 1);
  lcd.print("Ket noi WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed! Continuing without WiFi...");
  }

  // Khởi tạo ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ESP-NOW LOI!");
    while(1) delay(1000); // Stop here
  }
  
  Serial.println("ESP-NOW initialized");
  
  // Đăng ký callback
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  
  // Thêm ESP32-CAM làm peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, camMacAddress, 6);
  peerInfo.channel = 0; // Auto channel
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LOI THEM PEER!");
    espNowReady = false;
  } else {
    Serial.println("ESP-NOW peer added successfully");
    espNowReady = true;
  }
  
  // Hiển thị sẵn sàng
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM READY");
  playBeep(100, 200);
  delay(1000);
  lcd.clear();
}

// ============================================================
// Helper: Phát âm thanh beep (cho ACTIVE BUZZER)
// ============================================================
void playBeep(int dummy, int duration) {
  digitalWrite(SPEAKER_PIN, LOW);  // Đảm bảo OFF trước
  delay(10);
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(duration);
  digitalWrite(SPEAKER_PIN, LOW);
  delay(20);  // Delay thêm để ổn định
}

void playSuccessSound() {
  // 2 beep nhanh - Thành công
  digitalWrite(SPEAKER_PIN, LOW);
  delay(10);
  
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(100);
  digitalWrite(SPEAKER_PIN, LOW);
  delay(80);
  
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(100);
  digitalWrite(SPEAKER_PIN, LOW);
  delay(20);
}

void playFailSound() {
  // 1 beep dài - Thất bại
  digitalWrite(SPEAKER_PIN, LOW);
  delay(10);
  
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(400);
  digitalWrite(SPEAKER_PIN, LOW);
  delay(20);
}

void playErrorSound() {
  // 3 beep ngắn - Lỗi hệ thống
  digitalWrite(SPEAKER_PIN, LOW);
  delay(10);
  
  for(int i = 0; i < 3; i++) {
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(100);
    digitalWrite(SPEAKER_PIN, LOW);
    delay(150);
  }
  delay(20);
}

// ============================================================
// Helper: Parse JSON kết quả từ server
// ============================================================
bool parseResult(String payload, String &message) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return false;
  }
  
  if (doc.containsKey("message")) {
    message = doc["message"].as<String>();
    return true;
  }
  
  return false;
}

// ============================================================
// Helper: Hiển thị kết quả + phát âm thanh thông minh
// ============================================================
void displayResult(String message) {
  lcd.clear();
  
  // Kiểm tra nếu là lỗi hệ thống thì không hiển thị "KET QUA:"
  String lowerMsg = message;
  lowerMsg.toLowerCase();
  
  if (lowerMsg.indexOf("chua bat dau") >= 0 || 
      lowerMsg.indexOf("ko phat hien") >= 0) {
    // Các trường hợp đặc biệt - không hiển thị "KET QUA:"
    lcd.setCursor(0, 0);
    lcd.print(" KHONG THANH  ");
    lcd.setCursor(0, 1);
    lcd.print("     CONG     ");
    playFailSound();
  } 
  else if (message.indexOf("GIA MAO") >= 0 || message.indexOf("FAKE") >= 0) {
    // Phát hiện giả mạo
    lcd.setCursor(0, 0);
    lcd.print("   CANH BAO!  ");
    lcd.setCursor(0, 1);
    lcd.print("   GIA MAO!   ");
    playFailSound();
  }
  else if (lowerMsg.indexOf("khong nhan ra") >= 0 || 
           lowerMsg.indexOf("khong") >= 0) {
    // Không nhận ra
    lcd.setCursor(0, 0);
    lcd.print(" KHONG NHAN RA");
    lcd.setCursor(0, 1);
    lcd.print("  NGUOI NAY   ");
    playFailSound();
  }
  else {
    // Thành công - hiển thị tên
    lcd.setCursor(0, 0);
    lcd.print("  CHAO MUNG! ");
    lcd.setCursor(0, 1);
    
    // Truncate tên nếu quá dài
    if (message.length() > 16) {
      message = message.substring(0, 16);
    }
    
    // Center text nếu ngắn
    int padding = (16 - message.length()) / 2;
    for (int i = 0; i < padding; i++) {
      lcd.print(" ");
    }
    lcd.print(message);
    
    playSuccessSound();
  }
  
  Serial.print("Displayed result: ");
  Serial.println(message);
}

// ============================================================
// MAIN LOOP với State Machine
// ============================================================
void loop() {
  unsigned long now = millis();
  int currentRadarState = digitalRead(RADAR_OUT);
  
  // Debounce radar để tránh nhiễu
  if (currentRadarState != lastRadarState) {
    lastRadarChangeTime = now;
  }
  bool radarStable = (now - lastRadarChangeTime) > RADAR_DEBOUNCE;
  lastRadarState = currentRadarState;
  
  // State Machine
  switch (currentState) {
    
    case IDLE: {
      // Hiển thị sẵn sàng
      static unsigned long lastUpdate = 0;
      if (now - lastUpdate > 2000) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" SAN SANG DIEM ");
        lcd.setCursor(0, 1);
        lcd.print("     DANH      ");
        lastUpdate = now;
      }
      
      // Phát hiện chuyển động (sau khi radar ổn định)
      if (currentRadarState == HIGH && radarStable) {
        // Kiểm tra cooldown
        if (now - lastTriggerTime < TRIGGER_COOLDOWN) {
          // Hiển thị countdown
          unsigned long remaining = (TRIGGER_COOLDOWN - (now - lastTriggerTime)) / 1000;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("VUI LONG DOI");
          lcd.setCursor(0, 1);
          lcd.print("Con ");
          lcd.print(remaining);
          lcd.print(" giay...");
        } else {
          // Chuyển sang state DETECTING
          currentState = DETECTING;
          stateStartTime = now;
          Serial.println("=== STATE: DETECTING ===");
        }
      }
      break;
    }
    
    case DETECTING: {
      // Hiển thị 1 lần, không update liên tục
      static bool displayed = false;
      if (!displayed) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" DANG NHAN DIEN");
        lcd.setCursor(0, 1);
        lcd.print("   XIN CHO...  ");
        displayed = true;
      }
      
      // Gửi trigger qua ESP-NOW
      if (espNowReady) {
        uint8_t triggerData[2] = {0x01, 0xFF};
        esp_err_t result = esp_now_send(camMacAddress, triggerData, sizeof(triggerData));
        
        if (result == ESP_OK) {
          Serial.println("ESP-NOW: Trigger sent!");
          playBeep(80, 100);
          lastTriggerTime = now;
          currentState = PROCESSING;
          stateStartTime = now;
          displayed = false; // Reset flag
          Serial.println("=== STATE: PROCESSING ===");
        } else {
          Serial.print("ESP-NOW: Send failed! Error: ");
          Serial.println(result);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("   LOI MANG!   ");
          lcd.setCursor(0, 1);
          lcd.print("  THU LAI...   ");
          playErrorSound();
          delay(3000);
          currentState = IDLE;
          displayed = false; // Reset flag
          Serial.println("=== STATE: IDLE (ESP-NOW error) ===");
        }
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   LOI MANG!   ");
        lcd.setCursor(0, 1);
        lcd.print("  THU LAI...   ");
        playErrorSound();
        delay(3000);
        currentState = IDLE;
        displayed = false; // Reset flag
        Serial.println("=== STATE: IDLE (ESP-NOW not ready) ===");
      }
      break;
    }
    
    case PROCESSING: {
      // KHÔNG CẬP NHẬT LCD - giữ nguyên "DANG NHAN DIEN..."
      
      // Polling kết quả từ server
      static unsigned long lastCheck = 0;
      if (now - lastCheck > 500) {
        lastCheck = now;
        
        if (WiFi.status() != WL_CONNECTED) {
          // WiFi disconnected
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("   LOI WIFI!   ");
          lcd.setCursor(0, 1);
          lcd.print("  THU LAI...   ");
          playErrorSound();
          delay(2000);
          currentState = COOLDOWN;
          stateStartTime = now;
          Serial.println("=== STATE: COOLDOWN (WiFi error) ===");
          break;
        }
        
        HTTPClient http;
        http.begin(serverUrlResult);
        http.setTimeout(3000);
        
        int httpResponseCode = http.GET();
        
        if (httpResponseCode == 200) {
          String payload = http.getString();
          String message = "";
          
          if (parseResult(payload, message)) {
            Serial.print("Result message: ");
            Serial.println(message);
            
            // Bỏ qua các message tạm thời
            if (message == "Ready" || message.length() == 0) {
              // Chưa có kết quả, tiếp tục chờ
            }
            // Xử lý TẤT CẢ các trường hợp có kết quả
            else {
              displayResult(message);
              currentState = SHOWING_RESULT;
              stateStartTime = now;
              Serial.println("=== STATE: SHOWING_RESULT ===");
            }
          } else {
            // Parse JSON lỗi
            Serial.println("Failed to parse JSON");
          }
        } 
        else if (httpResponseCode == -1) {
          // Timeout hoặc không kết nối được
          Serial.println("HTTP timeout or connection failed");
        }
        else {
          // HTTP error khác
          Serial.print("HTTP Error: ");
          Serial.println(httpResponseCode);
        }
        
        http.end();
      }
      
      // Timeout check
      if (now - stateStartTime > PROCESSING_TIMEOUT) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("  QUA LAU ROI! ");
        lcd.setCursor(0, 1);
        lcd.print("  THU LAI...   ");
        playErrorSound();
        delay(2000);
        currentState = COOLDOWN;
        stateStartTime = now;
        Serial.println("=== STATE: COOLDOWN (timeout) ===");
      }
      break;
    }
    
    case SHOWING_RESULT: {
      // Hiển thị kết quả trong RESULT_DISPLAY_TIME
      if (now - stateStartTime > RESULT_DISPLAY_TIME) {
        currentState = COOLDOWN;
        stateStartTime = now;
        Serial.println("=== STATE: COOLDOWN ===");
      }
      break;
    }
    
    case COOLDOWN: {
      // Quay về màn hình "SAN SANG" ngay lập tức
      static bool displayedCooldown = false;
      if (!displayedCooldown) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" SAN SANG DIEM ");
        lcd.setCursor(0, 1);
        lcd.print("     DANH      ");
        displayedCooldown = true;
      }
      
      if (now - stateStartTime > TRIGGER_COOLDOWN) {
        // Cooldown hết, quay về IDLE
        displayedCooldown = false;
        currentState = IDLE;
        Serial.println("=== STATE: IDLE ===");
      }
      break;
    }
  }
  
  // ĐẢM BẢO BUZZER LUÔN TẮT SAU MỖI VÒNG LOOP
  digitalWrite(SPEAKER_PIN, LOW);
  
  delay(50);
}
