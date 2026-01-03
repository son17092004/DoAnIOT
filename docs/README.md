# 📸 HỆ THỐNG ĐIỂM DANH KHUÔN MẶT ESP32 - ESP-NOW

[![ESP32](https://img.shields.io/badge/ESP32-Espressif-red)](https://www.espressif.com/)
[![Python](https://img.shields.io/badge/Python-3.8+-blue)](https://www.python.org/)
[![FastAPI](https://img.shields.io/badge/FastAPI-0.115+-green)](https://fastapi.tiangolo.com/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

> Hệ thống điểm danh tự động sử dụng nhận diện khuôn mặt với ESP32, ESP-NOW và AI Anti-Spoofing

---

## 🎯 Giới Thiệu

Hệ thống điểm danh thông minh sử dụng:
- **ESP32-CAM** để chụp ảnh khuôn mặt
- **ESP32 LCD** với cảm biến radar phát hiện chuyển động
- **ESP-NOW** giao tiếp real-time (<10ms latency) giữa 2 ESP32
- **FastAPI Backend** xử lý nhận diện khuôn mặt
- **Anti-Spoofing Model** ngăn chặn gian lận bằng ảnh/video
- **MongoDB** lưu trữ dữ liệu sinh viên và điểm danh

### ✨ Tính Năng Nổi Bật

✅ **Real-time Communication** - ESP-NOW với latency <10ms  
✅ **Anti-Spoofing** - Phát hiện ảnh giả bằng MobileNetV2  
✅ **Face Alignment** - Tăng độ chính xác nhận diện  
✅ **Auto Flash LED** - Tự động bật đèn khi chụp trong môi trường tối  
✅ **LCD Feedback** - Hiển thị kết quả real-time trên LCD 16x2  
✅ **Audio Alert** - Phản hồi âm thanh cho người dùng  
✅ **Session Management** - Quản lý buổi học và lưu lịch sử  
✅ **Web Dashboard** - Giao diện web quản lý sinh viên  

---

## 🏗️ Kiến Trúc Hệ Thống

```
┌─────────────────┐     ESP-NOW      ┌──────────────────┐
│   ESP32 LCD     │◄────(<10ms)─────►│   ESP32-CAM      │
│  • Radar        │                  │  • Camera        │
│  • LCD Display  │                  │  • Flash LED     │
│  • Speaker      │                  │                  │
└────────┬────────┘                  └────────┬─────────┘
         │                                    │
         │          WiFi (HTTP)               │
         └──────────────┬─────────────────────┘
                        │
                ┌───────▼────────┐
                │   FastAPI      │
                │   • Face Recog │
                │   • Anti-Spoof │
                │   • MongoDB    │
                └────────────────┘
```

**Xem chi tiết:** [ARCHITECTURE.md](ARCHITECTURE.md)

---

## 📦 Yêu Cầu Hệ Thống

### **Phần Cứng:**
- 1x **ESP32 Dev Module** (cho LCD controller)
- 1x **ESP32-CAM AI Thinker** (camera module)
- 1x **LCD 16x2 I2C** (địa chỉ 0x27)
- 1x **Radar/PIR Sensor** (cảm biến chuyển động)
- 1x **Speaker/Buzzer** (phản hồi âm thanh)
- Nguồn **5V/2A** (quan trọng cho ESP32-CAM)

### **Phần Mềm:**
- **Arduino IDE** (1.8.19+) hoặc **PlatformIO**
- **Python 3.8+**
- **MongoDB** (local hoặc Atlas)
- **CMake** và **Visual C++ Build Tools** (Windows)

---

## ⚡ Quick Start (5 Phút)

### **1. Clone Repository**
```bash
git clone <your-repo-url>
cd DoAnIOT
```

### **2. Setup ESP32 (3 phút)**

#### a) Lấy MAC Address ESP32-CAM:
```
1. Nạp: esp32/get_mac_address/get_mac_address.ino
2. Copy MAC từ Serial Monitor
```

#### b) Cấu hình ESP32 LCD (`testlcd.ino`):
```cpp
// Dòng ~42: Paste MAC Address ESP32-CAM
uint8_t camMacAddress[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

// Dòng ~11-14: WiFi settings
const char* ssid = "YourWiFi";
const char* password = "YourPassword";
const char* serverUrlResult = "http://192.168.X.X:8080/api/result/latest";
```

#### c) Cấu hình ESP32-CAM (`camera_client.ino`):
```cpp
// Dòng ~10-12
const char* ssid = "YourWiFi";
const char* password = "YourPassword";
const char* serverHost = "192.168.X.X"; // IP server (không có http://)
```

#### d) Nạp code:
```
- ESP32 LCD: Board = "ESP32 Dev Module"
- ESP32-CAM: Board = "AI Thinker ESP32-CAM"
```

### **3. Setup Backend (2 phút)**

```bash
# Cài dependencies
cd backend
pip install -r ../requirements.txt

# Chạy server
python main.py
```

Server sẽ chạy tại: `http://localhost:8080`

---

## 📖 Tài Liệu Chi Tiết

| File | Mục đích | Độ ưu tiên |
|------|----------|-----------|
| [SETUP_GUIDE.md](SETUP_GUIDE.md) | Hướng dẫn setup từng bước | ⭐⭐⭐⭐⭐ |
| [esp32/README_ESP_NOW.md](esp32/README_ESP_NOW.md) | Chi tiết ESP-NOW | ⭐⭐⭐⭐⭐ |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Kiến trúc hệ thống | ⭐⭐⭐⭐ |
| [CHANGELOG_ESP_NOW.md](CHANGELOG_ESP_NOW.md) | Lịch sử thay đổi | ⭐⭐⭐ |
| [SUMMARY.md](SUMMARY.md) | Tóm tắt dự án | ⭐⭐⭐⭐ |

---

## 🎓 Sử Dụng

### **1. Đăng Ký Sinh Viên**

#### Via Web UI:
```
1. Mở: http://localhost:8080/static/index.html
2. Upload ảnh khuôn mặt (chụp thẳng, đủ ánh sáng)
3. Nhập tên và mã sinh viên
4. Nhấn "Register"
```

#### Via API:
```bash
curl -X POST "http://localhost:8080/api/register" \
  -F "file=@photo.jpg" \
  -F "name=Nguyen Van A" \
  -F "student_id=SV001"
```

### **2. Bắt Đầu Buổi Học**

```bash
curl -X POST "http://localhost:8080/api/session/start" \
  -d "session_name=Buoi hoc ngay 03/01/2026"
```

### **3. Điểm Danh Tự Động**

1. Sinh viên đứng trước camera
2. Radar phát hiện chuyển động
3. LCD hiển thị: `PHAT HIEN NGUOI` → `Dang chup anh...`
4. ESP32-CAM chụp ảnh (tự động bật flash nếu tối)
5. Server nhận diện khuôn mặt
6. LCD hiển thị kết quả: `KET QUA: [Tên]`
7. Phát âm thanh phản hồi

### **4. Xem Kết Quả**

```bash
# Lấy thông tin session
curl http://localhost:8080/api/session/{session_id}

# Danh sách sinh viên đã điểm danh
# (Hoặc xem trong MongoDB Compass)
```

---

## 🧪 Test & Debug

### **Kiểm tra ESP32-CAM:**
```
Serial Monitor (115200 baud):
✅ ESP32-CAM MAC Address: XX:XX:XX:XX:XX:XX
✅ WiFi connected
✅ ESP-NOW initialized successfully
✅ Camera initialized successfully!
✅ === System Ready ===
```

### **Kiểm tra ESP32 LCD:**
```
Serial Monitor (115200 baud):
✅ WiFi connected
✅ ESP-NOW initialized
✅ ESP-NOW peer added successfully
✅ SYSTEM READY

LCD Display:
✅ "SAN SANG DIEM DANH"
```

### **Test Trigger:**
```
1. Đưa tay vào radar
2. LCD: "PHAT HIEN NGUOI" + beep
3. ESP32-CAM Serial: "Trigger received!" → "Upload completed"
4. LCD: "KET QUA: [Tên]" + beep-beep
```

---

## 🐛 Troubleshooting

### **ESP-NOW không hoạt động?**
```cpp
// Check MAC Address đúng chưa
uint8_t camMacAddress[] = {0x24, 0x6F, ...}; // ⬅️ Verify này

// Check WiFi mode
WiFi.mode(WIFI_STA); // ⬅️ Phải có trước esp_now_init()
```

### **Camera init failed?**
```
Nguyên nhân: Nguồn điện yếu
Giải pháp: Dùng nguồn 5V/2A ổn định, KHÔNG dùng USB máy tính
```

### **Không upload được ảnh?**
```
1. Check server IP đúng chưa (ipconfig/ifconfig)
2. Verify server đang chạy: http://localhost:8080/
3. Tắt firewall Windows tạm thời
4. Check WiFi cả 2 ESP32 cùng subnet với server
```

### **face_recognition install lỗi (Windows)?**
```bash
# Cài CMake trước
https://cmake.org/download/

# Cài Visual C++ Build Tools
https://visualstudio.microsoft.com/visual-cpp-build-tools/

# Thử lại
pip install cmake dlib face-recognition
```

**Xem thêm:** [esp32/README_ESP_NOW.md](esp32/README_ESP_NOW.md) → Phần "Xử Lý Lỗi"

---

## 📊 Hiệu Năng

| Metric | Trước (HTTP Polling) | Sau (ESP-NOW) | Cải thiện |
|--------|----------------------|---------------|-----------|
| **Trigger latency** | 500-2000ms | <10ms | 99.5% ⚡ |
| **Total response** | 1.5-3.7s | 1-1.7s | 54% ⚡ |
| **WiFi requests/day** | 172,800 | ~100 | 99.9% 🔋 |
| **Tiêu thụ pin** | Cao | Thấp | ~70% 🔋 |

---

## 🔐 Bảo Mật

### **Anti-Spoofing:**
- ✅ MobileNetV2 model phát hiện ảnh giả
- ✅ Liveness detection với threshold 0.5
- ✅ Tự động lưu ảnh spoof vào `/static/spoofs/`

### **Face Recognition:**
- ✅ 128D embedding vector
- ✅ Cosine distance matching
- ✅ Threshold < 0.5 để xác nhận

### **Network:**
- ✅ WPA2 WiFi encryption
- ✅ Local network (không public internet)
- ✅ MongoDB authentication (optional)

---

## 🛠️ Cấu Trúc Dự Án

```
DoAnIOT/
├── esp32/
│   ├── camera_client/          # ESP32-CAM code
│   │   └── camera_client.ino
│   ├── testlcd/                # ESP32 LCD code
│   │   └── testlcd.ino
│   ├── get_mac_address/        # Helper tool
│   │   └── get_mac_address.ino
│   ├── README.md               # ESP32 overview
│   └── README_ESP_NOW.md       # ESP-NOW chi tiết
│
├── backend/
│   ├── main.py                 # FastAPI server
│   ├── face_utils.py           # Face recognition utils
│   ├── database.py             # MongoDB connection
│   └── static/
│       ├── index.html          # Web UI
│       ├── captures/           # Ảnh điểm danh
│       └── spoofs/             # Ảnh giả mạo
│
├── training_anti_spoof/
│   ├── antispoof_model.pth     # PyTorch model
│   └── AntiSpoofing_MobileNetV2.ipynb
│
├── requirements.txt            # Python dependencies
├── SETUP_GUIDE.md              # Quick setup guide
├── ARCHITECTURE.md             # Kiến trúc hệ thống
├── CHANGELOG_ESP_NOW.md        # Lịch sử thay đổi
├── SUMMARY.md                  # Tóm tắt dự án
└── README.md                   # File này
```

---

## 🎓 Pipeline Nhận Diện Khuôn Mặt

Theo yêu cầu của đồ án:

### **Bước 1: Face Detection (Phát hiện khuôn mặt)**
```python
# Sử dụng face_recognition (HOG/CNN)
face_locations = face_recognition.face_locations(image)
```

### **Bước 2: Face Alignment (Căn chỉnh - Quan trọng)**
```python
# Căn chỉnh dựa trên landmarks
landmarks = face_recognition.face_landmarks(image)
aligned_face = align_face(image, landmarks)
```

### **Bước 3: Feature Extraction (Trích xuất đặc trưng)**
```python
# Extract 128D embedding
embedding = face_recognition.face_encodings(aligned_face)
```

### **Bước 4: Anti-Spoofing (Phát hiện giả mạo)**
```python
# MobileNetV2 classifier
is_real, score = check_liveness(image, model)
```

### **Bước 5: Face Matching (So sánh)**
```python
# Cosine distance
distances = face_recognition.face_distance(known_embeddings, input_embedding)
match = distances < threshold
```

---

## 🌐 API Endpoints

### **Session Management:**
```bash
POST   /api/session/start        # Bắt đầu buổi học
POST   /api/session/stop         # Kết thúc buổi học
GET    /api/session/current      # Session hiện tại
GET    /api/session/{id}         # Chi tiết session
```

### **Student Management:**
```bash
POST   /api/register             # Đăng ký sinh viên
GET    /api/students             # Danh sách sinh viên (TODO)
```

### **Recognition:**
```bash
POST   /api/recognize            # Nhận diện khuôn mặt (từ ESP32-CAM)
GET    /api/result/latest        # Kết quả nhận diện mới nhất
```

### **ESP-NOW Bridge (Deprecated):**
```bash
POST   /api/trigger              # [Deprecated] HTTP trigger mode
GET    /api/trigger/check        # [Deprecated] HTTP polling mode
```

### **Security:**
```bash
GET    /api/spoofs               # Danh sách ảnh giả mạo
```

**API Docs:** http://localhost:8080/docs

---

## 🚀 Roadmap

### **v1.0 (Current) - ESP-NOW Version** ✅
- [x] ESP-NOW communication
- [x] Auto flash LED
- [x] Anti-spoofing
- [x] Face alignment
- [x] Session management
- [x] Web UI

### **v1.1 (Planned)**
- [ ] Multiple ESP32-CAM support
- [ ] Real-time dashboard
- [ ] Export Excel reports
- [ ] Email notifications

### **v2.0 (Future)**
- [ ] Mobile app (React Native)
- [ ] Cloud deployment (AWS/GCP)
- [ ] HTTPS/SSL support
- [ ] Advanced analytics

---

## 👨‍💻 Tác Giả

**Dự án IoT Điểm Danh Khuôn Mặt**
- ESP32 + ESP-NOW + FastAPI + Face Recognition
- Ngày hoàn thành: 03/01/2026
- Version: 1.0 (ESP-NOW)

---

## 📄 License

MIT License - Xem file [LICENSE](LICENSE) để biết thêm chi tiết

---

## 🙏 Acknowledgments

- [Espressif ESP32](https://www.espressif.com/) - ESP32 platform
- [FastAPI](https://fastapi.tiangolo.com/) - Web framework
- [face_recognition](https://github.com/ageitgey/face_recognition) - Face recognition library
- [PyTorch](https://pytorch.org/) - Deep learning framework

---

## 📞 Hỗ Trợ

- 📖 **Docs:** Xem các file README trong project
- 🐛 **Issues:** Check Troubleshooting sections
- 💡 **Tips:** [esp32/README_ESP_NOW.md](esp32/README_ESP_NOW.md)

---

## ⭐ Star History

Nếu project này hữu ích, đừng quên star ⭐ để support nhé!

---

**Made with ❤️ using ESP32, Python, and AI**

