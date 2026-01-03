# 📋 TỔNG HỢP TẤT CẢ TRƯỜNG HỢP HỆ THỐNG

## 📊 BẢNG TỔNG HỢP 15 CASES

| # | Tình huống | LCD Dòng 1 | LCD Dòng 2 | Âm thanh | State |
|---|------------|------------|------------|----------|-------|
| **1** | **Idle - Sẵn sàng** | `SAN SANG DIEM` | `DANH` | 🔇 | IDLE |
| **2** | **Cooldown đang chờ** | `VUI LONG DOI` | `Con X giay...` | 🔇 | IDLE |
| **3** | **Phát hiện chuyển động** | `PHAT HIEN NGUOI` | `Dang chup anh...` | 🔊 1 beep | DETECTING |
| **4** | **Lỗi ESP-NOW gửi** | `LOI ESP-NOW!` | `Ma loi: X` | 🔊🔊🔊 3 beeps | → IDLE |
| **5** | **ESP-NOW chưa ready** | `LOI HE THONG!` | `ESP-NOW chua OK` | 🔊🔊🔊 3 beeps | → IDLE |
| **6** | **Đang xử lý** | `PHAT HIEN NGUOI` | `Dang chup anh...` | 🔇 | PROCESSING |
| **7** | **WiFi mất kết nối** | `LOI WIFI!` | `Ket noi lai...` | 🔊🔊🔊 3 beeps | → COOLDOWN |
| **8** | **Timeout xử lý (>20s)** | `QUA THOI GIAN!` | `Thu lai...` | 🔊🔊🔊 3 beeps | → COOLDOWN |
| **9** | **Không có session** | `KET QUA:` | `Chua bat dau` | 🔊 1 beep dài | SHOWING_RESULT |
| **10** | **Không phát hiện mặt** | `KET QUA:` | `Ko phat hien` | 🔊 1 beep dài | SHOWING_RESULT |
| **11** | **Lỗi trích xuất** | `KET QUA:` | `Loi trich xuat` | 🔊 1 beep dài | SHOWING_RESULT |
| **12** | **Ảnh giả (spoof)** | `KET QUA:` | `GIA MAO (FAKE)` | 🔊 1 beep dài | SHOWING_RESULT |
| **13** | **Không nhận ra** | `KET QUA:` | `Khong nhan ra` | 🔊 1 beep dài | SHOWING_RESULT |
| **14** | **Thành công** | `KET QUA:` | `[Tên người]` | 🔊🔊 2 beeps | SHOWING_RESULT |
| **15** | **Radar vẫn HIGH** | `XIN DI RA` | `KHOI SENSOR!` | 🔇 | COOLDOWN |

---

## 🔊 4 LOẠI ÂM THANH

### 1. `playBeep(80, 100)` - 1 Beep Ngắn
**Sử dụng:** Thông báo bình thường
- ✅ Đã gửi trigger qua ESP-NOW
```cpp
digitalWrite(SPEAKER_PIN, HIGH);
delay(100);
digitalWrite(SPEAKER_PIN, LOW);
```

### 2. `playSuccessSound()` - 2 Beeps Nhanh
**Sử dụng:** Nhận diện thành công ✅
- ✅ Nhận diện đúng người
```cpp
beep(100ms) → delay(50ms) → beep(100ms)
```

### 3. `playFailSound()` - 1 Beep Dài
**Sử dụng:** Nhận diện thất bại ❌
- ❌ Không phát hiện mặt
- ❌ Không nhận ra
- ❌ Ảnh giả (spoof)
- ❌ Lỗi trích xuất
- ❌ Chưa có session
```cpp
digitalWrite(SPEAKER_PIN, HIGH);
delay(400);
digitalWrite(SPEAKER_PIN, LOW);
```

### 4. `playErrorSound()` - 3 Beeps Ngắn
**Sử dụng:** Lỗi hệ thống ⚠️
- ⚠️ WiFi disconnected
- ⚠️ ESP-NOW failed
- ⚠️ Processing timeout
```cpp
for(3 lần): beep(100ms) → delay(100ms)
```

---

## 🎯 HÀM `displayResult()` - TỰ ĐỘNG PHÁT ÂM THANH

```cpp
void displayResult(String message) {
  // 1. Hiển thị LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("KET QUA:");
  lcd.setCursor(0, 1);
  lcd.print(message); // Auto truncate nếu > 16 chars
  
  // 2. Phát âm thanh thông minh dựa trên keywords
  String lowerMsg = message;
  lowerMsg.toLowerCase();
  
  if (message.indexOf("GIA MAO") >= 0 || 
      message.indexOf("FAKE") >= 0 ||
      lowerMsg.indexOf("khong") >= 0 || 
      lowerMsg.indexOf("ko ") >= 0 ||
      lowerMsg.indexOf("loi") >= 0 ||
      lowerMsg.indexOf("chua") >= 0) {
    playFailSound();  // ❌ Thất bại
  } else {
    playSuccessSound(); // ✅ Thành công
  }
}
```

**Keywords thất bại:**
- `GIA MAO`, `FAKE`
- `khong`, `ko`
- `loi`
- `chua`

**Mặc định:** Thành công (2 beeps nhanh)

---

## 🔄 STATE MACHINE

```
        ┌──────────────┐
        │     IDLE     │ ← Sẵn sàng / Cooldown hết
        └──────┬───────┘
               │ Radar HIGH + debounce
               ↓
        ┌──────────────┐
        │  DETECTING   │ → ESP-NOW send trigger
        └──────┬───────┘
               │ Send OK
               ↓
        ┌──────────────┐
        │  PROCESSING  │ → Poll server mỗi 500ms
        └──────┬───────┘
               │ Got result / timeout / error
               ↓
        ┌──────────────┐
        │SHOWING_RESULT│ → Hiển thị 4s
        └──────┬───────┘
               │
               ↓
        ┌──────────────┐
        │   COOLDOWN   │ → Chờ radar LOW + 3s cooldown
        └──────┬───────┘
               │
               └────────→ IDLE
```

### Error Transitions
```
DETECTING ──(ESP-NOW fail)──→ IDLE
PROCESSING ──(WiFi error)──→ COOLDOWN
PROCESSING ──(Timeout)─────→ COOLDOWN
```

---

## 📝 TESTING CHECKLIST

### ✅ Test Nhận Diện

#### Test 1: Không có session đang chạy
**Chuẩn bị:**
- Không start session trên server
- Đứng vào vùng radar

**Kỳ vọng:**
```
LCD: "KET QUA:"
     "Chua bat dau"
Âm thanh: 1 beep dài (400ms)
```

---

#### Test 2: Chỉ có nền, không có người
**Chuẩn bị:**
- Start session trên server
- Để camera nhìn vào nền, không có người

**Kỳ vọng:**
```
LCD: "KET QUA:"
     "Ko phat hien"
Âm thanh: 1 beep dài (400ms)
```

---

#### Test 3: Người lạ chưa đăng ký
**Chuẩn bị:**
- Start session trên server
- Người chưa đăng ký đứng trước camera

**Kỳ vọng:**
```
LCD: "KET QUA:"
     "Khong nhan ra"
Âm thanh: 1 beep dài (400ms)
```

---

#### Test 4: Người đã đăng ký
**Chuẩn bị:**
- Start session trên server
- Người đã đăng ký đứng trước camera

**Kỳ vọng:**
```
LCD: "KET QUA:"
     "[Tên người]"
Âm thanh: 2 beeps nhanh (100ms + 50ms + 100ms)
```

---

#### Test 5: Dùng ảnh giả (spoof)
**Chuẩn bị:**
- Start session trên server
- Dùng ảnh in hoặc điện thoại hiển thị ảnh

**Kỳ vọng:**
```
LCD: "KET QUA:"
     "GIA MAO (FAKE)"
Âm thanh: 1 beep dài (400ms)
```

---

### ⚠️ Test Lỗi Hệ Thống

#### Test 6: WiFi mất kết nối
**Chuẩn bị:**
- Start session
- Tắt WiFi router giữa chừng

**Kỳ vọng:**
```
LCD: "LOI WIFI!"
     "Ket noi lai..."
Âm thanh: 3 beeps ngắn (100ms × 3)
State: → COOLDOWN
```

---

#### Test 7: Timeout > 20s
**Chuẩn bị:**
- Start session
- Server xử lý chậm hoặc tắt server

**Kỳ vọng:**
```
LCD: "QUA THOI GIAN!"
     "Thu lai..."
Âm thanh: 3 beeps ngắn (100ms × 3)
State: → COOLDOWN
```

---

#### Test 8: ESP-NOW lỗi
**Chuẩn bị:**
- Tắt ESP32-CAM
- Hoặc sai MAC address

**Kỳ vọng:**
```
LCD: "LOI ESP-NOW!"
     "Ma loi: X"
Âm thanh: 3 beeps ngắn (100ms × 3)
State: → IDLE
Serial: Error code number
```

---

#### Test 9: ESP-NOW chưa khởi tạo
**Chuẩn bị:**
- Lỗi khởi tạo ESP-NOW trong `setup()`

**Kỳ vọng:**
```
LCD: "LOI HE THONG!"
     "ESP-NOW chua OK"
Âm thanh: 3 beeps ngắn (100ms × 3)
State: → IDLE
```

---

### 🔄 Test State Machine

#### Test 10: Radar vẫn HIGH sau kết quả
**Chuẩn bị:**
- Hoàn thành 1 lần nhận diện
- Đứng yên không rời khỏi radar

**Kỳ vọng:**
```
LCD: "XIN DI RA"
     "KHOI SENSOR!"
Âm thanh: 🔇 (silent)
State: COOLDOWN (không chuyển về IDLE)
```

---

#### Test 11: Cooldown 3s
**Chuẩn bị:**
- Hoàn thành 1 lần nhận diện
- Đi ra khỏi radar
- Vào lại radar ngay

**Kỳ vọng:**
```
Nếu chưa đủ 3s:
LCD: "VUI LONG DOI"
     "Con X giay..."
Không trigger lại
```

---

## 🐛 DEBUG TIPS

### Serial Monitor Output
```
=== STATE: IDLE ===
Radar HIGH detected, waiting for stable...
=== STATE: DETECTING ===
ESP-NOW: Trigger sent!
=== STATE: PROCESSING ===
Checking result... (attempt 1/40)
Checking result... (attempt 2/40)
...
Got result: Nguyen Van A
=== STATE: SHOWING_RESULT ===
Displayed result: Nguyen Van A
=== STATE: COOLDOWN ===
Cooldown remaining: 3s
Cooldown remaining: 2s
Cooldown remaining: 1s
=== STATE: IDLE ===
```

### Common Issues

#### 1. Buzzer kêu liên tục
**Nguyên nhân:** GPIO 13 bị pull-up khi boot
**Giải pháp:** Đã thêm `digitalWrite(SPEAKER_PIN, LOW)` đầu tiên trong `setup()`

#### 2. ESP-NOW không gửi được
**Nguyên nhân:** Sai MAC address
**Giải pháp:** 
1. Upload `get_mac_address.ino` lên ESP32-CAM
2. Copy MAC address từ Serial Monitor
3. Cập nhật `camMacAddress[]` trong code

#### 3. Không nhận được kết quả từ server
**Nguyên nhân:** Windows Firewall chặn
**Giải pháp:**
```cmd
netsh advfirewall set allprofiles state off
```

#### 4. Timeout liên tục
**Nguyên nhân:** Server chậm xử lý
**Giải pháp:** Kiểm tra server logs, tăng `PROCESSING_TIMEOUT` nếu cần

---

## 📄 FILES LIÊN QUAN

```
esp32/
├── testlcd/
│   ├── testlcd.ino                    ← Main controller (file này)
│   ├── SYSTEM_CASES_DOCUMENTATION.md  ← Tài liệu này
│   └── PASSIVE_BUZZER_CODE.txt        ← Backup code cho passive buzzer
└── camera_client/
    └── camera_client.ino              ← ESP32-CAM client

backend/
└── main.py                            ← FastAPI server
```

---

## 🚀 UPLOAD & RUN

### Bước 1: Cập nhật MAC Address
```cpp
uint8_t camMacAddress[] = {0x80, 0xF3, 0xDA, 0x5F, 0xEC, 0x44};
//                          ↑ THAY BẰNG MAC CỦA ESP32-CAM CỦA BẠN
```

### Bước 2: Upload code
1. Chọn board: **ESP32 Dev Module**
2. Chọn port COM
3. Upload

### Bước 3: Test từng case
- Làm theo **Testing Checklist** từ Test 1 → Test 11
- Xác nhận LCD và âm thanh đúng như kỳ vọng

### Bước 4: Monitor Serial
- Baud rate: **115200**
- Theo dõi state transitions và errors

---

## ✅ DONE!

Hệ thống đã được tối ưu hoàn chỉnh với:
- ✅ 15 cases được xử lý đầy đủ
- ✅ 4 loại âm thanh phản hồi rõ ràng
- ✅ LCD messages dễ hiểu
- ✅ Error handling toàn diện
- ✅ State machine ổn định
- ✅ Logging chi tiết cho debug

**Version:** 1.0.0 - Final Optimized
**Last Updated:** 2026-01-03

