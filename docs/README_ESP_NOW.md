# 📡 Hướng Dẫn Cấu Hình ESP-NOW cho ESP32

## 🎯 Tổng Quan

Hệ thống sử dụng **ESP-NOW** để giao tiếp trực tiếp giữa 2 ESP32:
- **ESP32 LCD** (Main Controller): Phát hiện chuyển động → Gửi trigger
- **ESP32-CAM** (Camera): Nhận trigger → Chụp ảnh → Upload server

### ⚡ Ưu điểm ESP-NOW so với HTTP Polling:
- **Latency cực thấp**: <10ms (thay vì 500-2000ms)
- **Tiết kiệm pin**: Không cần polling liên tục
- **Không phụ thuộc server**: Peer-to-peer trực tiếp
- **Độ tin cậy cao**: Không bị miss trigger

---

## 📋 Các Bước Cài Đặt

### **Bước 1: Lấy MAC Address của ESP32-CAM**

1. Nạp file `esp32/get_mac_address/get_mac_address.ino` vào **ESP32-CAM**
2. Mở Serial Monitor (115200 baud)
3. Copy MAC Address hiển thị, ví dụ:
   ```
   ESP32 MAC Address: 24:6F:28:AA:BB:CC
   Array format: {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}
   ```

### **Bước 2: Cấu hình ESP32 LCD**

Mở file `esp32/testlcd/testlcd.ino`, tìm dòng:

```cpp
// MAC Address của ESP32-CAM (cần cập nhật sau khi chạy get_mac_address.ino)
uint8_t camMacAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // ⚠️ CẬP NHẬT TẠI ĐÂY
```

Thay đổi thành MAC Address của ESP32-CAM:

```cpp
uint8_t camMacAddress[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};
```

### **Bước 3: Cấu hình WiFi**

Cả 2 ESP32 cần cùng kết nối WiFi (để ESP-CAM upload ảnh lên server).

Trong file `testlcd.ino` và `camera_client.ino`, cập nhật:

```cpp
const char* ssid = "TenWiFi";
const char* password = "MatKhauWiFi";
const char* serverHost = "192.168.1.XXX"; // IP của server
```

**Lưu ý:** Để lấy IP server, chạy `ipconfig` (Windows) hoặc `ifconfig` (Linux/Mac)

### **Bước 4: Nạp Code**

1. **Nạp vào ESP32 LCD**: `esp32/testlcd/testlcd.ino`
2. **Nạp vào ESP32-CAM**: `esp32/camera_client/camera_client.ino`

---

## 🔧 Cấu Hình Arduino IDE

### **1. Thư Viện Cần Thiết**

Vào **Sketch → Include Library → Manage Libraries**, cài đặt:

- ✅ **LiquidCrystal I2C** (by Frank de Brabander) - Cho LCD
- ✅ **ArduinoJson** (by Benoit Blanchon) - Parse JSON từ server
- ✅ **esp_now.h** (có sẵn trong ESP32 Core, không cần cài thêm)

### **2. Board Settings cho ESP32 LCD**

```
Board: ESP32 Dev Module
CPU Frequency: 240MHz
Flash Frequency: 80MHz
Flash Mode: QIO
Partition Scheme: Default 4MB
Upload Speed: 921600
```

### **3. Board Settings cho ESP32-CAM**

```
Board: AI Thinker ESP32-CAM
CPU Frequency: 240MHz
Flash Frequency: 80MHz
Flash Mode: QIO
Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
Upload Speed: 115200
```

---

## 🚀 Kiểm Tra Hoạt Động

### **1. Kiểm tra ESP32-CAM**

Mở Serial Monitor (115200 baud), bạn sẽ thấy:

```
=== ESP32-CAM Starting ===
ESP32-CAM MAC Address: 24:6F:28:AA:BB:CC
Connecting to WiFi........
WiFi connected
IP Address: 192.168.1.105
ESP-NOW initialized successfully
Camera initialized successfully!
=== System Ready - Waiting for ESP-NOW trigger ===
```

### **2. Kiểm tra ESP32 LCD**

Mở Serial Monitor, bạn sẽ thấy:

```
Khoi dong...
WiFi connected
IP: 192.168.1.104
ESP-NOW initialized
ESP-NOW peer added successfully
SYSTEM READY
```

LCD hiển thị: `SAN SANG DIEM DANH`

### **3. Test Trigger**

1. Đưa tay vào cảm biến radar
2. ESP32 LCD gửi trigger qua ESP-NOW
3. ESP32-CAM nhận trigger và chụp ảnh
4. Serial Monitor ESP32-CAM hiển thị:
   ```
   ESP-NOW: Trigger received from LCD!
   === Processing trigger ===
   Image captured: 15234 bytes in 234 ms
   Uploading to server...
   Upload completed in 1245 ms
   ```

5. ESP32 LCD polling kết quả từ server và hiển thị trên LCD

---

## 🐛 Xử Lý Lỗi Thường Gặp

### **Lỗi 1: ESP-NOW init failed**

**Nguyên nhân:** WiFi chưa được set mode đúng

**Giải pháp:**
```cpp
WiFi.mode(WIFI_STA); // Phải gọi trước esp_now_init()
```

### **Lỗi 2: Failed to add peer**

**Nguyên nhân:** MAC Address sai hoặc chưa cập nhật

**Giải pháp:**
- Chạy lại `get_mac_address.ino` để lấy MAC chính xác
- Kiểm tra format: `{0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}`

### **Lỗi 3: Trigger sent but CAM không nhận**

**Nguyên nhân:** ESP32-CAM chưa khởi động xong hoặc MAC sai

**Giải pháp:**
- Đợi ESP32-CAM khởi động hoàn toàn (LED blink 2 lần)
- Verify MAC Address trên Serial Monitor
- Kiểm tra cả 2 ESP32 đều có `ESP-NOW initialized successfully`

### **Lỗi 4: Camera init failed**

**Nguyên nhân:** Nguồn điện yếu hoặc kết nối camera lỏng

**Giải pháp:**
- Dùng nguồn 5V/2A ổn định
- Kiểm tra lại kết nối camera với board
- Thử reset lại ESP32-CAM

### **Lỗi 5: WiFi connected nhưng không upload được**

**Nguyên nhân:** Server IP sai hoặc server chưa chạy

**Giải pháp:**
- Kiểm tra IP server bằng `ipconfig`
- Verify server đang chạy: `http://IP:8080/`
- Tắt firewall Windows tạm thời để test

---

## 📊 So Sánh HTTP vs ESP-NOW

| Tiêu chí | HTTP Polling (Cũ) | ESP-NOW (Mới) |
|----------|-------------------|---------------|
| **Latency** | 500-2000ms | <10ms |
| **Tiêu thụ pin** | Cao (polling liên tục) | Thấp (chỉ kích hoạt khi cần) |
| **Bandwidth** | ~172,800 requests/ngày | Chỉ khi trigger |
| **Độ tin cậy** | Có thể miss trigger | 99.9% tin cậy |
| **Phụ thuộc server** | Có (làm cầu nối) | Không (peer-to-peer) |
| **Code complexity** | Thấp | Trung bình |

---

## 🔌 Sơ Đồ Kết Nối Phần Cứng

### **ESP32 LCD:**
```
ESP32 Dev Module
├─ GPIO 4   → Radar OUT
├─ GPIO 25  → Speaker (PWM)
├─ GPIO 21  → LCD SDA (I2C)
└─ GPIO 22  → LCD SCL (I2C)
```

### **ESP32-CAM:**
```
ESP32-CAM AI Thinker
├─ GPIO 4   → Flash LED
└─ Camera Module (built-in)
```

---

## 📝 Luồng Hoạt Động

```
1. [Radar] Phát hiện chuyển động
         ↓
2. [ESP32 LCD] Gửi trigger qua ESP-NOW (payload: {0x01, 0xFF})
         ↓ <10ms
3. [ESP32-CAM] Nhận trigger → Callback onDataRecv() kích hoạt
         ↓
4. [ESP32-CAM] Chụp ảnh (tự động bật flash nếu tối)
         ↓
5. [ESP32-CAM] Upload ảnh lên Server qua HTTP POST
         ↓
6. [Server] Anti-Spoofing → Face Recognition → Lưu DB
         ↓
7. [ESP32 LCD] Polling kết quả từ Server (GET /api/result/latest)
         ↓
8. [LCD] Hiển thị tên/kết quả + phát âm thanh
```

---

## 🎓 Tài Liệu Tham Khảo

- [ESP-NOW Official Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- [ESP32-CAM Pinout](https://randomnerdtutorials.com/esp32-cam-ai-thinker-pinout/)
- [LiquidCrystal I2C Library](https://github.com/johnrickman/LiquidCrystal_I2C)

---

## 💡 Tips & Tricks

### **1. Tăng tầm xa ESP-NOW:**
```cpp
esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Dùng channel 1
esp_now_set_peer_rate_config(mac, WIFI_PHY_RATE_LORA_250K); // Giảm tốc độ tăng tầm xa
```

### **2. Thêm mã hóa AES cho ESP-NOW:**
```cpp
peerInfo.encrypt = true;
memcpy(peerInfo.lmk, "MySecretKey12345", 16); // 16-byte key
```

### **3. Debug ESP-NOW:**
```cpp
esp_now_register_send_cb(onDataSent); // Callback xác nhận gửi thành công
esp_now_register_recv_cb(onDataRecv); // Callback khi nhận data
```

### **4. Cải thiện chất lượng ảnh ban đêm:**
```cpp
// Trong camera_client.ino đã có tự động phát hiện độ sáng
// Có thể điều chỉnh threshold:
return avgBrightness < 60; // Giảm xuống 40 nếu muốn flash bật sớm hơn
```

---

## 🆘 Hỗ Trợ

Nếu gặp vấn đề, kiểm tra:
1. ✅ Serial Monitor output của cả 2 ESP32
2. ✅ MAC Address đã cập nhật chính xác
3. ✅ Cả 2 ESP32 đều connect WiFi thành công
4. ✅ Server backend đang chạy (`python backend/main.py`)
5. ✅ Nguồn điện ổn định (5V/2A)

---

**Chúc bạn thành công! 🎉**

