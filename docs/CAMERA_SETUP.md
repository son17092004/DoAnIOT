# 📹 Hướng dẫn cài đặt ESP32-CAM với Static IP

## 🎯 Thông tin cấu hình

### ESP32-CAM (Static IP)
```
IP Address: 192.168.252.200
Gateway:    192.168.252.1
Subnet:     255.255.255.0
DNS:        8.8.8.8
```

### WiFi
```
SSID:       conmeo
Password:   meomeomeo
```

### Backend Server
```
IP:   192.168.252.107
Port: 8080
```

### Camera Stream
```
URL: http://192.168.252.200:81/stream
```

---

## 🚀 Cách sử dụng

### 1️⃣ Upload code lên ESP32-CAM

```bash
Arduino IDE → Open: esp32/camera_client/camera_client.ino
→ Select Board: AI Thinker ESP32-CAM
→ Upload
```

### 2️⃣ Kiểm tra Serial Monitor (Baud 115200)

**Log mong đợi:**
```
========================================
ESP32-CAM Starting...
========================================
[WiFi] MAC Address: XX:XX:XX:XX:XX:XX
[WiFi] Connecting to: conmeo
........
[WiFi] ✓ Connected!
[WiFi] Static IP: 192.168.252.200
[WiFi] Gateway: 192.168.252.1
[WiFi] Stream URL: http://192.168.252.200:81/stream
[ESP-NOW] ✓ Initialized
[Camera] PSRAM found - Using CIF 400x296
[Camera] ✓ Initialized successfully
[HTTP] Camera stream server started on port 81
========================================
✓ System Ready!
✓ Waiting for ESP-NOW trigger...
========================================
```

### 3️⃣ Test camera stream

**Cách 1: Dùng browser**
```
Mở Chrome/Firefox → http://192.168.252.200:81/stream
```

**Cách 2: Dùng Web UI**
```
1. Mở: http://192.168.252.107:8080
2. Tab "Điểm danh" → Phần "Camera ESP32 - Livestream"
3. URL mặc định: http://192.168.252.200:81/stream
4. Click "🔗 Kết nối"
```

---

## ✅ Checklist Troubleshooting

### ❌ Không kết nối được WiFi
- [ ] Kiểm tra SSID/Password đúng không?
- [ ] Router có bật không?
- [ ] Reset ESP32-CAM (nút RST)

### ❌ Không truy cập được stream
- [ ] Ping thử: `ping 192.168.252.200`
- [ ] Máy tính/điện thoại cùng mạng WiFi `conmeo` không?
- [ ] Check Serial Monitor có log `[HTTP] Camera stream server started`?

### ❌ Stream lag/giật
- [ ] Giảm số người xem (mỗi viewer = 1 stream)
- [ ] Kiểm tra WiFi signal (đưa ESP32 gần router)
- [ ] Restart ESP32-CAM

### ❌ ESP-NOW không nhận trigger
- [ ] Kiểm tra MAC Address ESP32-CAM
- [ ] Cập nhật MAC vào code ESP32 LCD
- [ ] Kiểm tra log `[ESP-NOW] Trigger received`

---

## 📝 Thay đổi IP (nếu cần)

Edit file: `esp32/camera_client/camera_client.ino`

```cpp
// Dòng 28-31
IPAddress local_IP(192, 168, 252, 200);  // ← Thay đổi IP ở đây
IPAddress gateway(192, 168, 252, 1);     // ← Gateway router
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
```

**Sau khi thay đổi:**
1. Upload lại code
2. Đợi ESP32 reboot
3. Check Serial Monitor để xem IP mới
4. Update URL trong Web UI

---

## 🔧 Cấu trúc code

```
camera_client.ino
├── CONFIGURATION (Dòng 20-49)
│   ├── WiFi credentials
│   ├── Static IP settings
│   └── Backend server settings
│
├── Camera Stream Server (Dòng 51-158)
│   ├── stream_handler() - MJPEG streaming
│   └── startCameraServer() - HTTP server
│
├── ESP-NOW (Dòng 160-176)
│   └── onDataRecv() - Nhận trigger từ LCD
│
├── setup() (Dòng 178-320)
│   ├── WiFi + Static IP
│   ├── ESP-NOW init
│   ├── Camera init
│   └── Start HTTP server
│
├── loop() (Dòng 322-347)
│   └── Xử lý trigger từ ESP-NOW
│
└── Helper Functions (Dòng 349-503)
    ├── isImageDark() - Kiểm tra độ sáng
    └── captureAndUpload() - Chụp và upload
```

---

## 📊 Performance

| Metric | Value |
|--------|-------|
| **Resolution** | 400x296 (CIF) |
| **Frame Rate** | ~10-15 FPS |
| **Quality** | JPEG Q=12 (tốt) |
| **Latency** | 100-300ms |
| **Bandwidth** | ~200 KB/s |

---

## 🎯 Files liên quan

- `backend/main.py` - Server nhận diện face + anti-spoofing
- `backend/static/index.html` - Web UI với livestream viewer
- `esp32/testlcd/testlcd.ino` - ESP32 LCD (gửi trigger)
- `docs/README_ESP_NOW.md` - ESP-NOW protocol

---

✅ **Setup xong! Giờ có thể xem livestream và điểm danh!** 📹

