# 📡 Hướng Dẫn ESP32 - Hệ Thống Điểm Danh Khuôn Mặt

## 🎯 Tổng Quan Hệ Thống

Hệ thống gồm 2 ESP32 giao tiếp qua **ESP-NOW** (peer-to-peer, latency <10ms):

1. **ESP32 LCD** (`testlcd/`) - Main Controller

   - Cảm biến radar phát hiện chuyển động
   - LCD 16x2 hiển thị kết quả
   - Speaker phản hồi âm thanh
   - Gửi trigger qua ESP-NOW → ESP32-CAM

2. **ESP32-CAM** (`camera_client/`) - Camera Module
   - Nhận trigger qua ESP-NOW
   - Tự động chụp ảnh (bật flash nếu tối)
   - Upload ảnh lên server nhận diện

---

## 🚀 Quick Start

### **1. Lấy MAC Address ESP32-CAM**

```bash
# Nạp file này vào ESP32-CAM
esp32/get_mac_address/get_mac_address.ino
```

Copy MAC Address từ Serial Monitor, ví dụ: `{0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}`

### **2. Cấu hình ESP32 LCD**

Mở `testlcd/testlcd.ino`, cập nhật MAC Address:

```cpp
uint8_t camMacAddress[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}; // ⬅️ Paste MAC ở đây
```

### **3. Cấu hình WiFi**

Cả 2 file (`testlcd.ino` và `camera_client.ino`):

```cpp
const char* ssid = "TenWiFi";          // ⬅️ Đổi WiFi của bạn
const char* password = "MatKhau";      // ⬅️ Đổi mật khẩu
const char* serverHost = "192.168.X.X"; // ⬅️ IP server (dùng ipconfig)
```

### **4. Nạp Code**

1. Nạp `testlcd/testlcd.ino` vào **ESP32 Dev Module**
2. Nạp `camera_client/camera_client.ino` vào **ESP32-CAM**

---

## 📚 Tài Liệu Chi Tiết

- 📖 **[README_ESP_NOW.md](README_ESP_NOW.md)** - Hướng dẫn chi tiết ESP-NOW
- 🔧 **Xử lý lỗi, debug, tối ưu hóa**
- 📊 **So sánh HTTP vs ESP-NOW**

---

## 🔧 Cài Đặt Arduino IDE

### **1. Cài đặt ESP32 Board Manager**

1. Mở Arduino IDE
2. **File → Preferences**
3. **Additional Boards Manager URLs**, thêm:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. **Tools → Board → Boards Manager**
5. Tìm `esp32` → Install **esp32 by Espressif Systems**

### **2. Cài Đặt Thư Viện**

**Tools → Manage Libraries**, cài:

- ✅ **LiquidCrystal I2C** (by Frank de Brabander)
- ✅ **ArduinoJson** (by Benoit Blanchon)

**Thư viện tự động có:**

- ✅ `WiFi.h`, `HTTPClient.h`, `esp_now.h`, `esp_camera.h`

---

## ⚙️ Cấu Hình Board

### **ESP32 LCD (Dev Module):**

```
Board: ESP32 Dev Module
CPU Frequency: 240MHz
Flash Mode: QIO
Partition Scheme: Default 4MB
Upload Speed: 921600
```

### **ESP32-CAM (AI Thinker):**

```
Board: AI Thinker ESP32-CAM
CPU Frequency: 240MHz
Flash Mode: QIO
Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
Upload Speed: 115200
```

---

## 🔌 Kết Nối Phần Cứng

### **ESP32 LCD:**

```
GPIO 4  → Radar OUT
GPIO 25 → Speaker (PWM)
GPIO 21 → LCD SDA (I2C)
GPIO 22 → LCD SCL (I2C)
```

### **ESP32-CAM:**

```
GPIO 4  → Flash LED (built-in)
Camera Module (built-in)
```

---

## 🎬 Nạp Code cho ESP32-CAM (Dùng đế nạp Micro USB)

1. **Gắn ESP32-CAM vào đế nạp** (camera hướng ra ngoài)
2. **Cắm cáp Micro USB** vào máy tính
3. Chọn **Port** (COM) trong Arduino IDE
4. **KHÔNG CẦN nối GPIO 0** (đế tự động vào boot mode)
5. Nhấn **Upload**
6. Sau khi nạp xong, nhấn nút **RST** trên ESP32-CAM

**Lưu ý:** Nếu bị `Connecting...` mãi, giữ nút **IO0** (hoặc **BOOT**) trong lúc uploading

---

## ✅ Kiểm Tra Hoạt Động

### **1. ESP32-CAM Serial Monitor (115200 baud):**

```
=== ESP32-CAM Starting ===
ESP32-CAM MAC Address: 24:6F:28:AA:BB:CC
WiFi connected
ESP-NOW initialized successfully
Camera initialized successfully!
=== System Ready - Waiting for ESP-NOW trigger ===
```

### **2. ESP32 LCD Serial Monitor:**

```
WiFi connected
ESP-NOW initialized
ESP-NOW peer added successfully
SYSTEM READY
```

**LCD hiển thị:** `SAN SANG DIEM DANH`

### **3. Test Trigger:**

1. Đưa tay vào radar
2. LCD hiển thị: `PHAT HIEN NGUOI` → `Dang chup anh...`
3. ESP32-CAM chụp ảnh (flash bật nếu tối) → Upload server
4. LCD hiển thị kết quả: `KET QUA: [Tên người]`

---

## 🐛 Xử Lý Lỗi Thường Gặp

### **Lỗi: Camera init failed / Brownout detector**

**Nguyên nhân:** Nguồn điện yếu  
**Giải pháp:** Dùng nguồn **5V/2A** ổn định

### **Lỗi: ESP-NOW init failed**

**Nguyên nhân:** WiFi chưa set mode  
**Giải pháp:** Đảm bảo có `WiFi.mode(WIFI_STA);` trước `esp_now_init()`

### **Lỗi: Failed to add peer**

**Nguyên nhân:** MAC Address sai  
**Giải pháp:** Chạy lại `get_mac_address.ino` để lấy MAC chính xác

### **Lỗi: WiFi connected nhưng không upload được**

**Nguyên nhân:** Server chưa chạy hoặc IP sai  
**Giải pháp:**

- Kiểm tra server đang chạy: `http://192.168.X.X:8080/`
- Verify IP bằng `ipconfig` (Windows)
- Tắt firewall tạm thời

---

## 📊 Hiệu Năng

| Metric              | HTTP Polling (Cũ) | ESP-NOW (Mới) |
| ------------------- | ----------------- | ------------- |
| **Latency trigger** | 500-2000ms        | <10ms ⚡      |
| **Camera capture**  | ~200ms            | ~200ms        |
| **Upload ảnh**      | 800-1500ms        | 800-1500ms    |
| **Total time**      | 1.5-3.7s          | 1-1.7s ⚡     |
| **Tiêu thụ pin**    | Cao               | Thấp 🔋       |

---

## 🔗 Files Quan Trọng

```
esp32/
├── camera_client/
│   └── camera_client.ino        # ESP32-CAM (receiver)
├── testlcd/
│   └── testlcd.ino              # ESP32 LCD (sender)
├── get_mac_address/
│   └── get_mac_address.ino      # Helper lấy MAC
├── README.md                    # File này (overview)
└── README_ESP_NOW.md            # Chi tiết ESP-NOW
```

---

## 💡 Tips

### **Tăng tầm xa ESP-NOW (lên 200m):**

```cpp
esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
```

### **Cải thiện ảnh ban đêm:**

```cpp
// Trong camera_client.ino, dòng 225:
return avgBrightness < 60; // Giảm xuống 40 nếu muốn flash bật sớm hơn
```

### **Debug ESP-NOW:**

Bật Serial Monitor cả 2 ESP32 để xem log real-time

---

## 🆘 Hỗ Trợ

Nếu gặp vấn đề:

1. ✅ Kiểm tra Serial Monitor output
2. ✅ Verify MAC Address đã đúng
3. ✅ Cả 2 ESP32 connect WiFi thành công
4. ✅ Server backend đang chạy
5. ✅ Nguồn điện ổn định 5V/2A

---

**Xem thêm:** [README_ESP_NOW.md](README_ESP_NOW.md) để biết chi tiết đầy đủ!

**Chúc thành công! 🎉**
