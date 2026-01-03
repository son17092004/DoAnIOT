# ⚡ QUICK SETUP GUIDE - ESP-NOW Version

## 🎯 5 Bước Setup Nhanh (< 15 phút)

---

## ✅ **Bước 1: Cài đặt Arduino IDE** (5 phút)

### 1.1. Cài ESP32 Board Manager

1. Mở Arduino IDE
2. `File → Preferences`
3. **Additional Boards Manager URLs**, paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. `Tools → Board → Boards Manager`
5. Tìm `esp32` → Install **esp32 by Espressif Systems**

### 1.2. Cài Thư Viện

`Tools → Manage Libraries`, cài:

- **LiquidCrystal I2C** (by Frank de Brabander)
- **ArduinoJson** (by Benoit Blanchon)

---

## ✅ **Bước 2: Lấy MAC Address ESP32-CAM** (2 phút)

1. Nạp file `esp32/get_mac_address/get_mac_address.ino` vào **ESP32-CAM**

   - Board: `AI Thinker ESP32-CAM`
   - Partition: `Huge APP (3MB No OTA)`
   - Upload Speed: `115200`

2. Mở Serial Monitor (115200 baud)

3. **Copy MAC Address**, ví dụ:
   ```
   Array format: {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}
   ```

---

## ✅ **Bước 3: Cấu hình ESP32 LCD** (3 phút)

1. Mở file `esp32/testlcd/testlcd.ino`

2. **Tìm dòng ~42**, paste MAC Address từ Bước 2:

   ```cpp
   uint8_t camMacAddress[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}; // ⬅️ THAY ĐỔI
   ```

3. **Tìm dòng ~11-13**, cập nhật WiFi:

   ```cpp
   const char* ssid = "TenWiFi";              // ⬅️ THAY ĐỔI
   const char* password = "MatKhauWiFi";      // ⬅️ THAY ĐỔI
   ```

4. **Lấy IP máy tính** (chạy `ipconfig` trong CMD):

   ```
   IPv4 Address: 192.168.1.105  ⬅️ Copy cái này
   ```

5. **Cập nhật dòng ~14**:

   ```cpp
   const char* serverUrlResult = "http://192.168.1.105:8080/api/result/latest";
   ```

6. **Nạp code vào ESP32 Dev Module:**
   - Board: `ESP32 Dev Module`
   - Upload Speed: `921600`
   - Nhấn **Upload**

---

## ✅ **Bước 4: Cấu hình ESP32-CAM** (2 phút)

1. Mở file `esp32/camera_client/camera_client.ino`

2. **Tìm dòng ~10-12**, cập nhật WiFi (giống Bước 3):

   ```cpp
   const char* ssid = "TenWiFi";         // ⬅️ THAY ĐỔI
   const char* password = "MatKhauWiFi"; // ⬅️ THAY ĐỔI
   const char* serverHost = "192.168.1.105"; // ⬅️ THAY ĐỔI (chỉ IP, không có http://)
   ```

3. **Nạp code vào ESP32-CAM:**
   - Board: `AI Thinker ESP32-CAM`
   - Partition: `Huge APP (3MB No OTA)`
   - Upload Speed: `115200`
   - Gắn ESP32-CAM vào đế nạp → Cắm USB
   - Nhấn **Upload**
   - Sau khi xong, nhấn nút **RST** trên ESP32-CAM

---

## ✅ **Bước 5: Setup Backend** (3 phút)

### 5.1. Cài MongoDB (nếu chưa có)

**Windows:**

```bash
# Download MongoDB Community Server từ mongodb.com
# Hoặc dùng MongoDB Atlas (cloud, free)
```

### 5.2. Cài Python Dependencies

```bash
cd backend
pip install -r ../requirements.txt
```

**Lưu ý:** Trên Windows cần:

- [CMake](https://cmake.org/download/)
- [Visual C++ Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)

### 5.3. Tạo file `.env` (nếu cần custom MongoDB)

```bash
# backend/.env
MONGO_URI=mongodb://localhost:27017
DB_NAME=face_attendance_db
```

### 5.4. Chạy Server

```bash
python main.py
```

Bạn sẽ thấy:

```
============================================================
ESP32 Face Attendance System - Backend Server
============================================================
Starting server on http://0.0.0.0:8080
API Docs: http://localhost:8080/docs
============================================================
```

---

## 🧪 **KIỂM TRA HỆ THỐNG**

### ✅ Test 1: Kiểm tra ESP32-CAM

1. Mở Serial Monitor (ESP32-CAM, 115200 baud)
2. Bạn phải thấy:
   ```
   ESP32-CAM MAC Address: 24:6F:28:AA:BB:CC
   WiFi connected
   ESP-NOW initialized successfully
   Camera initialized successfully!
   === System Ready ===
   ```

### ✅ Test 2: Kiểm tra ESP32 LCD

1. Mở Serial Monitor (ESP32 LCD, 115200 baud)
2. Bạn phải thấy:
   ```
   WiFi connected
   ESP-NOW initialized
   ESP-NOW peer added successfully
   SYSTEM READY
   ```
3. LCD hiển thị: **`SAN SANG DIEM DANH`**

### ✅ Test 3: Kiểm tra Server

1. Mở browser: `http://localhost:8080/`
2. Bạn phải thấy JSON:
   ```json
   { "message": "Face Attendance API Running", "docs": "/docs" }
   ```

### ✅ Test 4: Test Trigger

1. Đưa tay vào cảm biến radar
2. **Quan sát LCD:**

   - Hiển thị: `PHAT HIEN NGUOI`
   - Dòng 2: `Dang chup anh...`
   - Speaker phát beep ngắn

3. **Quan sát Serial Monitor ESP32-CAM:**

   ```
   ESP-NOW: Trigger received from LCD!
   === Processing trigger ===
   Image captured: 15234 bytes in 234 ms
   Uploading to server...
   Server response: HTTP/1.1 200 OK
   Upload completed in 1245 ms
   ```

4. **Quan sát Serial Monitor ESP32 LCD:**

   ```
   Motion detected!
   ESP-NOW: Trigger sent successfully!
   ```

5. **Sau vài giây, LCD hiển thị kết quả:**
   - Nếu chưa đăng ký: `Khong quen`
   - Nếu đã đăng ký: `KET QUA: [Tên]`
   - Speaker phát âm thanh tương ứng

---

## 🎓 **ĐĂNG KÝ SINH VIÊN**

### Cách 1: Dùng Web UI

1. Mở: `http://localhost:8080/static/index.html`
2. Upload ảnh khuôn mặt
3. Nhập tên và mã sinh viên
4. Nhấn **Register**

### Cách 2: Dùng API Docs

1. Mở: `http://localhost:8080/docs`
2. Endpoint: `POST /api/register`
3. Upload ảnh, điền form
4. Nhấn **Execute**

### Cách 3: Dùng Python

```python
import requests

url = "http://localhost:8080/api/register"
files = {"file": open("my_photo.jpg", "rb")}
data = {"name": "Nguyen Van A", "student_id": "SV001"}

response = requests.post(url, files=files, data=data)
print(response.json())
```

---

## 📊 **BẮT ĐẦU BUỔI HỌC (SESSION)**

1. Mở: `http://localhost:8080/docs`
2. Endpoint: `POST /api/session/start`
3. Nhập tên buổi học: `"Buoi hoc ngay 03/01/2026"`
4. **Execute**

Sau đó, mỗi lần có người đi qua → Hệ thống tự động điểm danh!

---

## 🐛 **XỬ LÝ LỖI THƯỜNG GẶP**

### ❌ Lỗi: `ESP-NOW init failed`

**Fix:**

```cpp
WiFi.mode(WIFI_STA); // Phải có dòng này trước esp_now_init()
```

### ❌ Lỗi: `Failed to add peer`

**Fix:** MAC Address sai hoặc format sai

- Chạy lại `get_mac_address.ino`
- Copy đúng format: `{0x24, 0x6F, ...}`

### ❌ Lỗi: `Camera init failed` / `Brownout detector`

**Fix:** Nguồn điện yếu

- Dùng nguồn **5V/2A** ổn định
- KHÔNG dùng USB máy tính (yếu)

### ❌ Lỗi: ESP32-CAM không nhận trigger

**Fix:**

- Verify MAC Address trên Serial Monitor
- Cả 2 ESP32 phải có message "ESP-NOW initialized successfully"
- Reset cả 2 ESP32

### ❌ Lỗi: `WiFi connected` nhưng không upload được

**Fix:**

- Verify server IP đúng (`ipconfig`)
- Server phải chạy (`python main.py`)
- Tắt Windows Firewall tạm thời

### ❌ Lỗi: `pip install face_recognition` thất bại (Windows)

**Fix:**

1. Cài CMake: https://cmake.org/download/
2. Cài Visual C++ Build Tools: https://visualstudio.microsoft.com/visual-cpp-build-tools/
3. Chạy lại: `pip install cmake dlib face-recognition`

### ❌ Lỗi: MongoDB connection failed

**Fix:**

- Cài MongoDB: https://www.mongodb.com/try/download/community
- Hoặc dùng MongoDB Atlas (free cloud)
- Hoặc comment code liên quan DB để test trước

---

## 📂 **CẤU TRÚC THƯ MỤC**

```
DoAnIOT/
├── esp32/
│   ├── camera_client/camera_client.ino   ⬅️ Nạp vào ESP32-CAM
│   ├── testlcd/testlcd.ino               ⬅️ Nạp vào ESP32 LCD
│   ├── get_mac_address/get_mac_address.ino ⬅️ Tool lấy MAC
│   ├── README.md                         📖 Overview
│   └── README_ESP_NOW.md                 📖 Chi tiết ESP-NOW
│
├── backend/
│   ├── main.py                           ⬅️ Chạy server này
│   ├── face_utils.py
│   ├── database.py
│   └── static/index.html                 🌐 Web UI
│
├── requirements.txt                      📦 Python dependencies
├── CHANGELOG_ESP_NOW.md                  📝 Chi tiết thay đổi
└── SETUP_GUIDE.md                        ⬅️ File này
```

---

## 🎉 **HOÀN TẤT!**

Sau khi hoàn thành 5 bước trên, hệ thống của bạn đã sẵn sàng!

### **Luồng hoạt động:**

```
Radar phát hiện → ESP32 LCD gửi ESP-NOW (<10ms)
→ ESP32-CAM chụp ảnh → Upload server
→ Server nhận diện → ESP32 LCD hiển thị kết quả
```

### **Next steps:**

1. ✅ Đăng ký sinh viên (qua Web UI)
2. ✅ Start session (qua API /api/session/start)
3. ✅ Test điểm danh (đưa tay vào radar)
4. ✅ Xem kết quả trong MongoDB

---

## 📞 **HỖ TRỢ**

- 📖 Chi tiết ESP-NOW: [README_ESP_NOW.md](esp32/README_ESP_NOW.md)
- 📝 Changelog: [CHANGELOG_ESP_NOW.md](CHANGELOG_ESP_NOW.md)
- 🔧 Troubleshooting: Xem phần "Xử lý lỗi" ở trên

**Good luck! 🚀**
