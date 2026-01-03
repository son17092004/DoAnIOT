# 🎨 KIẾN TRÚC HỆ THỐNG - ESP-NOW VERSION

## 📡 Sơ Đồ Tổng Quan

```
╔═══════════════════════════════════════════════════════════════════════════╗
║                    HỆ THỐNG ĐIỂM DANH KHUÔN MẶT ESP32                    ║
╚═══════════════════════════════════════════════════════════════════════════╝

┌──────────────────────────────────────────────────────────────────────────┐
│                          PHẦN CỨNG (HARDWARE)                             │
└──────────────────────────────────────────────────────────────────────────┘

    ┌─────────────────────────┐              ┌─────────────────────────┐
    │    ESP32 LCD (Main)     │              │    ESP32-CAM (Camera)   │
    │                         │              │                         │
    │  ┌───────────────────┐  │              │  ┌───────────────────┐  │
    │  │  Radar Sensor     │  │              │  │  OV2640 Camera    │  │
    │  │  (GPIO 4)         │  │              │  │  (2MP)            │  │
    │  └───────────────────┘  │              │  └───────────────────┘  │
    │                         │              │                         │
    │  ┌───────────────────┐  │              │  ┌───────────────────┐  │
    │  │  LCD 16x2 I2C     │  │              │  │  Flash LED        │  │
    │  │  (0x27)           │  │              │  │  (GPIO 4)         │  │
    │  └───────────────────┘  │              │  └───────────────────┘  │
    │                         │              │                         │
    │  ┌───────────────────┐  │              │  ┌───────────────────┐  │
    │  │  Speaker PWM      │  │              │  │  PSRAM 4MB        │  │
    │  │  (GPIO 25)        │  │              │  │  (Buffer)         │  │
    │  └───────────────────┘  │              │  └───────────────────┘  │
    │                         │              │                         │
    └────────────┬────────────┘              └───────────┬─────────────┘
                 │                                       │
                 │                                       │
                 │                                       │

┌──────────────────────────────────────────────────────────────────────────┐
│                      GIAO TIẾP (COMMUNICATION)                            │
└──────────────────────────────────────────────────────────────────────────┘

                 │                                       │
                 │◄─────── ESP-NOW (2.4GHz) ────────────┤
                 │         Latency: <10ms                │
                 │         Payload: {0x01, 0xFF}         │
                 │                                       │
                 │                                       │
                 └──────┐                    ┌───────────┘
                        │                    │
                        │  WiFi 2.4GHz       │  WiFi 2.4GHz
                        │  (WPA2)            │  (WPA2)
                        │                    │
                        └────────┬───────────┘
                                 │
                                 │
                        ┌────────▼────────┐
                        │   WiFi Router   │
                        │   (Home/Office) │
                        └────────┬────────┘
                                 │
                                 │ LAN/WAN
                                 │
                                 │

┌──────────────────────────────────────────────────────────────────────────┐
│                         SERVER (BACKEND)                                  │
└──────────────────────────────────────────────────────────────────────────┘

                        ┌────────▼────────┐
                        │   FastAPI       │
                        │   (Port 8080)   │
                        └────────┬────────┘
                                 │
                ┌────────────────┼────────────────┐
                │                │                │
                │                │                │
         ┌──────▼──────┐  ┌──────▼──────┐  ┌────▼─────┐
         │ Anti-Spoof  │  │Face Recog.  │  │ MongoDB  │
         │  MobileNet  │  │   dlib      │  │ Database │
         │   (PyTorch) │  │ face_recog  │  │ (Local)  │
         └─────────────┘  └─────────────┘  └──────────┘

```

---

## 🔄 LUỒNG HOẠT ĐỘNG (WORKFLOW)

### **1️⃣ IDLE STATE - Chờ Người**

```
┌─────────────────┐
│   ESP32 LCD     │
│                 │
│  LCD Display:   │
│  "SAN SANG"     │
│  "DIEM DANH"    │
│                 │
│  Radar: Scan... │◄─── Polling GPIO 4
└─────────────────┘

┌─────────────────┐
│  ESP32-CAM      │
│                 │
│  Status: Sleep  │
│  Waiting...     │
│                 │
│  ESP-NOW: Ready │◄─── Callback registered
└─────────────────┘
```

---

### **2️⃣ TRIGGER - Phát Hiện Chuyển Động**

```
    ┌─────────────────┐
    │   ESP32 LCD     │
    │                 │
    │  Radar: HIGH!   │◄─── Motion detected!
    │  ↓              │
    │  LCD: "PHAT     │
    │  HIEN NGUOI"    │
    │  ↓              │
    │  esp_now_send() │──┐
    └─────────────────┘  │
                         │ Payload: {0x01, 0xFF}
                         │ Latency: <10ms
                         │ 2.4GHz Radio
                         │
                         ↓
    ┌─────────────────┐  │
    │  ESP32-CAM      │  │
    │                 │◄─┘
    │  onDataRecv()   │◄─── Callback triggered!
    │  ↓              │
    │  triggerReceived│
    │  = true         │
    │  ↓              │
    │  captureAndUpload()│
    └─────────────────┘
```

---

### **3️⃣ CAPTURE - Chụp Ảnh**

```
    ┌─────────────────┐
    │  ESP32-CAM      │
    │                 │
    │  esp_camera_    │
    │  fb_get()       │──┐
    │  ↓              │  │ Capture frame
    │  isImageDark()? │  │ 200-300ms
    │  ↓              │  │
    │  if dark:       │  │
    │   • Flash ON    │  │
    │   • Re-capture  │◄─┘
    │   • Flash OFF   │
    │  ↓              │
    │  Image ready:   │
    │  15-30KB JPEG   │
    └─────────────────┘
```

---

### **4️⃣ UPLOAD - Gửi Ảnh Lên Server**

```
    ┌─────────────────┐
    │  ESP32-CAM      │
    │                 │
    │  WiFiClient     │
    │  connect()      │──┐
    │  ↓              │  │
    │  POST /api/     │  │ HTTP Multipart
    │  recognize      │  │ 800-1500ms
    │  ↓              │  │
    │  Upload chunks  │  │
    │  1024 bytes     │  │
    └────────┬────────┘  │
             │           │
             │ WiFi      │
             ↓           │
    ┌────────▼────────┐  │
    │   SERVER        │◄─┘
    │  FastAPI        │
    │  Port 8080      │
    └────────┬────────┘
             │
```

---

### **5️⃣ RECOGNITION - Nhận Diện Khuôn Mặt**

```
    ┌─────────────────┐
    │   SERVER        │
    │  /api/recognize │
    └────────┬────────┘
             │
             ↓
    ┌────────▼────────┐
    │  1. Anti-Spoof  │
    │  MobileNetV2    │
    │  ↓              │
    │  is_real?       │──No──┐
    │  score > 0.5?   │      │
    └────────┬────────┘      │
             │ Yes            │
             ↓                ↓
    ┌────────▼────────┐  ┌────▼──────┐
    │  2. Face Detect │  │ Return:   │
    │  face_recognition│  │ "GIA MAO" │
    │  ↓              │  │ Save spoof│
    │  bbox, landmarks│  │ image     │
    └────────┬────────┘  └───────────┘
             │
             ↓
    ┌────────▼────────┐
    │  3. Embedding   │
    │  Extract 128D   │
    │  ↓              │
    │  face_encodings │
    └────────┬────────┘
             │
             ↓
    ┌────────▼────────┐
    │  4. Compare DB  │
    │  MongoDB query  │
    │  ↓              │
    │  cosine_dist    │
    │  < 0.5?         │──No──┐
    └────────┬────────┘      │
             │ Yes            │
             ↓                ↓
    ┌────────▼────────┐  ┌────▼──────┐
    │  5. Mark        │  │ Return:   │
    │  Attendance     │  │ "Khong    │
    │  ↓              │  │  quen"    │
    │  Save to DB     │  └───────────┘
    │  Image + Name   │
    │  Timestamp      │
    └────────┬────────┘
             │
             ↓
    ┌────────▼────────┐
    │  LAST_          │
    │  RECOGNITION_   │
    │  RESULT =       │
    │  {              │
    │   "timestamp":  │
    │   "message":    │
    │      "Name"     │
    │  }              │
    └─────────────────┘
```

---

### **6️⃣ DISPLAY - Hiển Thị Kết Quả**

```
    ┌─────────────────┐
    │   ESP32 LCD     │
    │                 │
    │  Polling every  │
    │  500ms:         │
    │  ↓              │
    │  GET /api/      │──┐
    │  result/latest  │  │ HTTP GET
    └────────┬────────┘  │ 100-200ms
             │           │
             │ WiFi      │
             ↓           │
    ┌────────▼────────┐  │
    │   SERVER        │◄─┘
    │  Return JSON:   │
    │  {              │
    │   "timestamp":  │
    │   "message":    │
    │      "Name"     │
    │  }              │
    └────────┬────────┘
             │
             ↓
    ┌────────▼────────┐
    │   ESP32 LCD     │
    │                 │
    │  Parse JSON     │
    │  ↓              │
    │  LCD Display:   │
    │  "KET QUA:"     │
    │  "Name"         │
    │  ↓              │
    │  Speaker:       │
    │  🔊 Beep-Beep   │
    │  ↓              │
    │  Wait 4s        │
    │  ↓              │
    │  Back to IDLE   │
    └─────────────────┘
```

---

## ⏱️ TIMELINE COMPARISON

### **❌ Trước (HTTP Polling):**

```
0ms     Radar detect
        ↓
100ms   POST /api/trigger → Server
        ↓
        [Server: TRIGGER_PENDING = true]
        ↓
500ms   CAM polling... (sleep)
        ↓
1000ms  CAM polling... (sleep)
        ↓
1500ms  GET /api/trigger/check → Got trigger!
        ↓
1700ms  Capture photo
        ↓
3200ms  Upload to server
        ↓
3700ms  Server recognition done
        ↓
4200ms  LCD polling... Got result!
        ↓
4200ms  Display + Sound

TOTAL: 4.2s (worst case)
```

### **✅ Sau (ESP-NOW):**

```
0ms     Radar detect
        ↓
5ms     ESP-NOW send {0x01, 0xFF}
        ↓
<10ms   CAM receive callback!
        ↓
210ms   Capture photo (with flash if needed)
        ↓
1210ms  Upload to server
        ↓
1710ms  Server recognition done
        ↓
1710ms  LCD polling... Got result!
        ↓
1710ms  Display + Sound

TOTAL: 1.7s (typical case)
```

**🚀 Cải thiện: 59% faster!**

---

## 📊 DATA FLOW

```
┌───────────────────────────────────────────────────────────────────┐
│                         DATA PACKETS                              │
└───────────────────────────────────────────────────────────────────┘

1. ESP-NOW Trigger Packet:
   ┌──────┬──────┐
   │ 0x01 │ 0xFF │  = 2 bytes
   └──────┴──────┘
   • Command: 0x01 = Trigger
   • Checksum: 0xFF
   • Latency: <10ms
   • Protocol: ESP-NOW proprietary

2. HTTP Image Upload:
   ┌──────────────────────────────────────────────────────┐
   │ POST /api/recognize HTTP/1.1                         │
   │ Content-Type: multipart/form-data; boundary=...      │
   │ Content-Length: 18234                                │
   │                                                      │
   │ --boundary                                           │
   │ Content-Disposition: form-data; name="file"          │
   │ Content-Type: image/jpeg                             │
   │                                                      │
   │ [JPEG Binary Data: 15000-30000 bytes]                │
   │ --boundary--                                         │
   └──────────────────────────────────────────────────────┘
   • Size: 15-30KB
   • Upload time: 800-1500ms
   • Protocol: HTTP/1.1

3. Server Response:
   ┌──────────────────────────────────────────────────────┐
   │ HTTP/1.1 200 OK                                      │
   │ Content-Type: application/json                       │
   │                                                      │
   │ {                                                    │
   │   "status": "success",                               │
   │   "match": true,                                     │
   │   "name": "Nguyen Van A",                            │
   │   "message": "Attendance marked"                     │
   │ }                                                    │
   └──────────────────────────────────────────────────────┘

4. Result Polling:
   ┌──────────────────────────────────────────────────────┐
   │ GET /api/result/latest HTTP/1.1                      │
   │                                                      │
   │ Response:                                            │
   │ {                                                    │
   │   "timestamp": 1704268800.123,                       │
   │   "message": "Nguyen Van A"                          │
   │ }                                                    │
   └──────────────────────────────────────────────────────┘
   • Polling interval: 500ms
   • Size: <100 bytes
```

---

## 🔐 SECURITY

```
┌───────────────────────────────────────────────────────────────────┐
│                      SECURITY LAYERS                              │
└───────────────────────────────────────────────────────────────────┘

Layer 1: Network Security
┌──────────────────────┐
│  WPA2 WiFi Encryption│
│  AES-CCMP            │
└──────────────────────┘

Layer 2: Anti-Spoofing
┌──────────────────────┐
│  MobileNetV2 Model   │
│  Liveness Detection  │
│  Score Threshold>0.5 │
└──────────────────────┘

Layer 3: Face Recognition
┌──────────────────────┐
│  128D Embedding      │
│  Cosine Distance     │
│  Threshold < 0.5     │
└──────────────────────┘

Layer 4: Database
┌──────────────────────┐
│  MongoDB             │
│  Secure connection   │
│  Backup daily        │
└──────────────────────┘
```

---

## 💾 DATABASE SCHEMA

```
MongoDB: face_attendance_db

Collection: students
┌─────────────┬──────────────┬─────────────────────────┐
│ _id         │ String       │ Auto ObjectId           │
│ name        │ String       │ "Nguyen Van A"          │
│ student_id  │ String       │ "SV001"                 │
│ embedding   │ Array[128]   │ [0.123, -0.456, ...]    │
│ created_at  │ DateTime     │ 2026-01-03T10:00:00Z    │
└─────────────┴──────────────┴─────────────────────────┘

Collection: sessions
┌──────────────┬──────────────┬─────────────────────────┐
│ _id          │ String       │ Auto ObjectId           │
│ session_name │ String       │ "Buoi hoc 03/01/2026"   │
│ date         │ DateTime     │ 2026-01-03T10:00:00Z    │
│ attendees    │ Array        │ [{...}, {...}]          │
│  ├─ student_id│ String      │ "SV001"                 │
│  ├─ name     │ String       │ "Nguyen Van A"          │
│  ├─ image_url│ String       │ "/static/captures/..."  │
│  └─ timestamp│ DateTime     │ 2026-01-03T10:05:23Z    │
└──────────────┴──────────────┴─────────────────────────┘
```

---

**📖 Xem thêm chi tiết trong các file:**

- `SETUP_GUIDE.md` - Hướng dẫn setup
- `esp32/README_ESP_NOW.md` - Chi tiết ESP-NOW
- `CHANGELOG_ESP_NOW.md` - Lịch sử thay đổi
