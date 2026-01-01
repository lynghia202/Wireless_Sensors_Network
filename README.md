# Wireless_Sensors_Network
#  HỆ THỐNG PCCC THÔNG MINH - TÀI LIỆU KỸ THUẬT CHI TIẾT

##  Mục lục

1. [Tổng quan hệ thống](#1-tổng-quan-hệ-thống)
2. [Kiến trúc hệ thống](#2-kiến-trúc-hệ-thống)
3. [Phần cứng (Hardware)](#3-phần-cứng-hardware)
4. [Backend ESP32](#4-backend-esp32)
5. [Frontend React](#5-frontend-react)
6. [Bảo mật (Security)](#6-bảo-mật-security)
7. [Real-time & Hiệu suất](#7-real-time--hiệu-suất)
8. [API Reference](#8-api-reference)
9. [Xử lý lỗi](#9-xử-lý-lỗi)
10. [Hướng dẫn triển khai](#10-hướng-dẫn-triển-khai)

---

## 1. Tổng quan hệ thống

### 1.1 Giới thiệu

**Hệ Thống PCCC Thông Minh** (Phòng Cháy Chữa Cháy) là giải pháp IoT giám sát và cảnh báo cháy/rò rỉ khí gas theo thời gian thực. Hệ thống kết hợp:

- **ESP32-S3** làm bộ điều khiển trung tâm
- **Cảm biến** đo nhiệt độ, độ ẩm, khí gas, phát hiện lửa
- **Web Dashboard** hiển thị dữ liệu real-time
- **Hệ thống cảnh báo** qua email và còi/đèn

### 1.2 Tính năng chính

| Tính năng | Mô tả |
|-----------|-------|
|  Giám sát nhiệt độ/độ ẩm | Cảm biến DHT22, cập nhật mỗi 2 giây |
|  Phát hiện lửa | Cảm biến hồng ngoại IR |
|  Phát hiện khí gas/khói | Cảm biến MQ-02, tính PPM |
|  Dashboard real-time | Giao diện web responsive, cập nhật 1s |
|  Cảnh báo email | Gửi email khi phát hiện nguy hiểm |
|  Xác thực Token | Bảo mật với JWT-like token |
|  Chống brute-force | Khóa tài khoản sau 5 lần sai |
|  Cấu hình từ xa | Thay đổi ngưỡng cảnh báo qua web |

### 1.3 Công nghệ sử dụng

**Backend:**
- ESP32-S3 (Dual-core, 240MHz)
- FreeRTOS (Real-time OS)
- ESPAsyncWebServer
- ArduinoJson
- WiFiManager
- SPIFFS (File system)

**Frontend:**
- React 18 + TypeScript
- Vite (Build tool)
- Tailwind CSS
- Recharts (Biểu đồ)
- Lucide React (Icons)

---

## 2. Kiến trúc hệ thống

### 2.1 Sơ đồ tổng quan

```
┌─────────────────────────────────────────────────────────────────┐
│                        INTERNET                                  │
│                           │                                      │
│                    ┌──────▼──────┐                              │
│                    │   Router    │                              │
│                    │  (WiFi AP)  │                              │
│                    └──────┬──────┘                              │
│                           │                                      │
│         ┌─────────────────┼─────────────────┐                   │
│         │                 │                 │                   │
│    ┌────▼────┐       ┌────▼────┐       ┌────▼────┐             │
│    │ Browser │       │ Browser │       │  SMTP   │             │
│    │ (User)  │       │ (Admin) │       │ Server  │             │
│    └────┬────┘       └────┬────┘       └────▲────┘             │
│         │                 │                 │                   │
│         └────────┬────────┘                 │                   │
│                  │ HTTP/REST                │ SMTP              │
│                  │                          │                   │
│         ┌────────▼──────────────────────────┴────┐              │
│         │              ESP32-S3                   │              │
│         │  ┌─────────────────────────────────┐   │              │
│         │  │         AsyncWebServer          │   │              │
│         │  │  ┌─────┐ ┌─────┐ ┌─────────┐   │   │              │
│         │  │  │/data│ │/login│ │/settings│   │   │              │
│         │  │  └─────┘ └─────┘ └─────────┘   │   │              │
│         │  └─────────────────────────────────┘   │              │
│         │                                         │              │
│         │  ┌─────────────────────────────────┐   │              │
│         │  │         FreeRTOS Tasks          │   │              │
│         │  │  ┌────────┐ ┌───────┐ ┌──────┐ │   │              │
│         │  │  │ Sensor │ │ Logic │ │ Alert│ │   │              │
│         │  │  │  Read  │ │Control│ │ Send │ │   │              │
│         │  │  └────────┘ └───────┘ └──────┘ │   │              │
│         │  └─────────────────────────────────┘   │              │
│         └────────────────┬───────────────────────┘              │
│                          │                                       │
│         ┌────────────────┼────────────────┐                     │
│         │                │                │                     │
│    ┌────▼────┐      ┌────▼────┐      ┌────▼────┐               │
│    │  DHT22  │      │  MQ-02  │      │   IR    │               │
│    │Temp/Hum │      │  Gas    │      │  Flame  │               │
│    └─────────┘      └─────────┘      └─────────┘               │
│                                                                  │
│                          │                                       │
│         ┌────────────────┼────────────────┐                     │
│         │                │                │                     │
│    ┌────▼────┐      ┌────▼────┐      ┌────▼────┐               │
│    │ Buzzer  │      │  Pump   │      │   Fan   │               │
│    │ (Relay) │      │ (Relay) │      │ (Relay) │               │
│    └─────────┘      └─────────┘      └─────────┘               │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Luồng dữ liệu

```
[Sensors] ──100ms──> [sensorReadTask] ──mutex──> [Global Variables]
                                                        │
                                                        ▼
[logicControlTask] <──50ms── [Global Variables] ──> [Relay Control]
        │                                                │
        │ (if alarm)                                     │
        ▼                                                ▼
[xAlertQueue] ──> [alertSendTask] ──> [SMTP Server] ──> [Email]
                                                        
[Frontend] ──1s poll──> [/data API] ──> [Global Variables]
```

---

## 3. Phần cứng (Hardware)

### 3.1 Danh sách linh kiện

| Linh kiện | Model | GPIO | Chức năng |
|-----------|-------|------|-----------|
| Vi điều khiển | ESP32-S3 | - | Bộ xử lý chính |
| Cảm biến nhiệt độ/ẩm | DHT22 | GPIO 4 | Đo nhiệt độ, độ ẩm |
| Cảm biến khí gas | MQ-02 | GPIO 1 (ADC) | Phát hiện khí gas, khói |
| Cảm biến lửa | IR Flame | GPIO 2 (ADC) | Phát hiện tia hồng ngoại |
| LED WiFi OK | LED xanh | GPIO 13 | Trạng thái kết nối |
| LED WiFi Error | LED đỏ | GPIO 14 | Trạng thái lỗi |
| Relay Buzzer | 5V Relay | GPIO 16 | Điều khiển còi |
| Relay Pump | 5V Relay | GPIO 15 | Điều khiển máy bơm |
| Relay Fan | 5V Relay | GPIO 17 | Điều khiển quạt |
| Nút Reset | Push Button | GPIO 21 | Factory reset (giữ 5s) |

### 3.2 Sơ đồ kết nối

```
                    ┌─────────────────┐
                    │    ESP32-S3     │
                    │                 │
    DHT22 ──────────┤ GPIO 4          │
    MQ-02 ──────────┤ GPIO 1 (ADC)    │
    IR Sensor ──────┤ GPIO 2 (ADC)    │
                    │                 │
    LED Green ◄─────┤ GPIO 13         │
    LED Red ◄───────┤ GPIO 14         │
                    │                 │
    Relay Buzzer ◄──┤ GPIO 16         │
    Relay Pump ◄────┤ GPIO 15         │
    Relay Fan ◄─────┤ GPIO 17         │
                    │                 │
    Reset Button ───┤ GPIO 21         │
                    └─────────────────┘
```

### 3.3 Công thức tính PPM (MQ-02)

Cảm biến MQ-02 đo nồng độ khí gas theo công thức:

```
Voltage = ADC_Value × (3.3V / 4095)
Rs = (5V - Voltage) × RL / Voltage
Ratio = Rs / R0
PPM = A × Ratio^B
```

Với các hằng số từ datasheet:
- `RL = 10kΩ` (Load Resistor)
- `R0 = 20kΩ` (Calibrated in clean air)
- `A = 672`, `B = -2.06`

---

## 4. Backend ESP32

### 4.1 FreeRTOS Tasks

Hệ thống sử dụng 3 task FreeRTOS chạy song song:

#### Task 1: sensorReadTask (Priority 2, Core 1)
```cpp
// Đọc cảm biến mỗi 100ms
void sensorReadTask(void *pvParameters) {
  for (;;) {
    // Đọc IR và MQ-02 mỗi 100ms
    int rawFire = 4095 - analogRead(PIN_IR_SENSOR);
    int rawMq = MQGetPPM(PIN_MQ02);
    
    // Đọc DHT22 mỗi 2 giây (giới hạn của cảm biến)
    if (millis() - lastDHTRead > 2000) {
      float t = dht.readTemperature();
      float h = dht.readHumidity();
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
```

#### Task 2: logicControlTask (Priority 1, Core 1)
```cpp
// Xử lý logic cảnh báo mỗi 100ms
void logicControlTask(void *pvParameters) {
  for (;;) {
    // Điều kiện phát hiện cháy
    if (irVal > irThresh && tempVal > 45.0) isFire = true;
    else if (tempVal > tempThresh) isFire = true;
    
    // Điều kiện phát hiện rò rỉ gas
    if (gasVal > gasThresh) isGasLeak = true;
    
    // Điều khiển relay
    if (isFire) {
      buzzer = true; pump = true; fan = false;
    } else if (isGasLeak) {
      buzzer = true; pump = false; fan = true;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
```

#### Task 3: alertSendTask (Priority 0, Core 0)
```cpp
// Gửi email cảnh báo (blocking, chạy trên Core 0)
void alertSendTask(void *pvParameters) {
  for (;;) {
    if (xQueueReceive(xAlertQueue, &req, portMAX_DELAY) == pdPASS) {
      // Kết nối SMTP và gửi email
      smtp.connect(SMTP_HOST, SMTP_PORT);
      smtp.send(msg);
    }
  }
}
```

### 4.2 Semaphore & Mutex

```cpp
SemaphoreHandle_t xSensorDataMutex;  // Bảo vệ biến toàn cục

// Sử dụng
if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    // Đọc/ghi biến an toàn
    g_temperature = newValue;
    xSemaphoreGive(xSensorDataMutex);
}
```

### 4.3 Cấu trúc lưu trữ

```cpp
// Token authentication
struct AuthToken {
    String token;                  // 32 ký tự random
    unsigned long expireTime;      // Thời gian hết hạn
    unsigned long lastActivityTime;
};

// Login tracking (brute-force protection)
struct LoginAttemptTracker {
    int failedCount;
    unsigned long lockoutEndTime;
    unsigned long lockoutDuration;
};
```

---

## 5. Frontend React

### 5.1 Cấu trúc thư mục

```
front_end/
├── App.tsx              # Component chính
├── constants.ts         # Cấu hình tập trung
├── types.ts             # TypeScript interfaces
├── index.tsx            # Entry point
├── components/
│   ├── Header.tsx       # Header với menu user
│   ├── StatusBanner.tsx # Banner trạng thái
│   ├── SensorGrid.tsx   # Grid hiển thị cảm biến
│   ├── LiveChart.tsx    # Biểu đồ real-time
│   ├── ActuatorPanel.tsx# Trạng thái relay
│   ├── HistoryLog.tsx   # Lịch sử sự kiện
│   ├── SettingsPanel.tsx# Panel cấu hình
│   ├── LoginScreen.tsx  # Màn hình đăng nhập
│   ├── ChangePasswordModal.tsx
│   ├── ErrorBoundary.tsx
│   └── Toast*.tsx       # Toast notifications
└── hooks/
    └── useToast.ts      # Hook quản lý toast
```

### 5.2 State Management

```typescript
// Các state chính trong App.tsx
const [authToken, setAuthToken] = useState('');           // Token xác thực
const [isAuthenticated, setIsAuthenticated] = useState(false);
const [data, setData] = useState<SensorData>(DEFAULT_DATA); // Dữ liệu sensor
const [connectionStatus, setConnectionStatus] = useState(ConnectionState.DISCONNECTED);
const [chartData, setChartData] = useState([]);           // Dữ liệu biểu đồ

// Refs để tránh re-render không cần thiết
const consecutiveFailuresRef = useRef(0);    // Đếm lỗi liên tiếp
const fetchIntervalRef = useRef(1000);       // Interval động
const isFetchingRef = useRef(false);         // Ngăn request chồng chéo
```

### 5.3 Data Fetching Logic

```typescript
// Dynamic polling interval
const fetchData = useCallback(async () => {
  // Ngăn overlapping requests
  if (isFetchingRef.current) return;
  isFetchingRef.current = true;
  
  try {
    const res = await fetchWithRetry(url, options);
    
    // Chuyển sang alert mode khi có cảnh báo
    if (jsonData.buzzer) {
      fetchIntervalRef.current = 400;  // 400ms khi có alarm
    } else {
      fetchIntervalRef.current = 1000; // 1s bình thường
    }
    
    // Debounce connection status
    if (!res) {
      consecutiveFailuresRef.current++;
      if (consecutiveFailuresRef.current >= 3) {
        setConnectionStatus(DISCONNECTED);
      }
    } else {
      consecutiveFailuresRef.current = 0;
    }
  } finally {
    isFetchingRef.current = false;
  }
}, []);
```

### 5.4 Interval Management

```typescript
// Single interval với dynamic timing
useEffect(() => {
  let lastFetchTime = Date.now();
  
  const intervalId = setInterval(() => {
    const timeSinceLastFetch = Date.now() - lastFetchTime;
    
    // Chỉ fetch khi đủ thời gian
    if (timeSinceLastFetch >= fetchIntervalRef.current) {
      lastFetchTime = Date.now();
      fetchData();
    }
  }, 200);  // Check mỗi 200ms
  
  return () => clearInterval(intervalId);
}, [fetchData]);
```

---

## 6. Bảo mật (Security)

### 6.1 Token-based Authentication

```
┌──────────┐     POST /login       ┌──────────┐
│  Client  │ ──────────────────>   │  ESP32   │
│          │  { password: "xxx" }  │          │
│          │                       │          │
│          │     200 OK            │          │
│          │ <──────────────────   │          │
│          │  { token: "abc..." }  │          │
│          │                       │          │
│          │     GET /data         │          │
│          │ ──────────────────>   │          │
│          │  Authorization:       │          │
│          │  Bearer abc...        │          │
└──────────┘                       └──────────┘
```

**Đặc điểm:**
- Token: 32 ký tự alphanumeric ngẫu nhiên
- Timeout: 5 phút không hoạt động
- Auto-refresh: Mỗi request hợp lệ gia hạn token
- Max tokens: 10 (multi-device support)

### 6.2 Brute-force Protection

```
Lần 1-4: Sai mật khẩu → Thông báo lỗi
Lần 5:   Sai mật khẩu → Khóa 30 giây
         (Nếu tiếp tục sai sau khi mở khóa)
Lần 10:  Sai mật khẩu → Khóa 90 giây (30 × 3)
Lần 15:  Sai mật khẩu → Khóa 270 giây (90 × 3)
...
```

### 6.3 Rate Limiting

```cpp
#define MIN_REQUEST_INTERVAL 1000  // 1 giây

if (millis() - g_lastLoginRequestTime < MIN_REQUEST_INTERVAL) {
    request->send(429, "application/json", 
                 "{\"error\": \"Too many requests\"}");
    return;
}
```

---

## 7. Real-time & Hiệu suất

### 7.1 Timing Configuration

| Parameter | Giá trị | Ý nghĩa |
|-----------|---------|---------|
| DATA_FETCH_NORMAL_MS | 1000ms | Polling bình thường |
| DATA_FETCH_ALERT_MS | 400ms | Polling khi có cảnh báo |
| REQUEST_TIMEOUT_MS | 5000ms | Timeout mỗi request |
| MAX_RETRIES | 2 | Số lần retry khi thất bại |
| MAX_CONSECUTIVE_FAILURES | 3 | Đánh dấu disconnect sau 3 lần fail |

### 7.2 Luồng phát hiện cháy

```
[Lửa bùng lên]
     │
     ▼ (100ms - sensorReadTask)
[ESP32 đọc IR sensor]
     │
     ▼ (100ms - logicControlTask)
[Logic: isFire = true, buzzer = true]
     │
     ▼ (0-1000ms - Frontend poll)
[Frontend nhận buzzer: true]
     │
     ▼ (ngay lập tức)
[Dashboard: Chuyển alert mode]
[Interval: 1000ms → 400ms]
     │
     ▼ (0-400ms)
[Real-time updates mỗi 400ms]
```

**Độ trễ thực tế:** 200ms - 1200ms (trung bình ~600ms)

### 7.3 Connection Stability

```typescript
// Debounce connection status
if (!res) {
  consecutiveFailuresRef.current++;
  
  // Chỉ đánh dấu disconnect sau 3 lần thất bại liên tiếp
  if (consecutiveFailuresRef.current >= 3) {
    setConnectionStatus(DISCONNECTED);
  }
} else {
  // Reset ngay khi thành công
  consecutiveFailuresRef.current = 0;
  setConnectionStatus(CONNECTED);
}
```

---

## 8. API Reference

### 8.1 Authentication APIs

#### POST /login
Đăng nhập và nhận token.

**Request:**
```json
{
  "password": "admin"
}
```

**Response (Success):**
```json
{
  "authenticated": true,
  "token": "aBcDeFgHiJkLmNoPqRsTuVwXyZ123456",
  "isDefault": true,
  "failedAttempts": 0,
  "locked": false
}
```

**Response (Locked):**
```json
{
  "authenticated": false,
  "locked": true,
  "remaining": 25,
  "failedAttempts": 5,
  "message": "Account locked due to too many failed attempts"
}
```

#### POST /logout
Xóa token và đăng xuất.

**Headers:**
```
Authorization: Bearer <token>
```

**Response:**
```json
{
  "status": "ok"
}
```

#### GET /auth-check
Kiểm tra trạng thái xác thực.

**Response:**
```json
{
  "locked": false,
  "remaining": 0,
  "authenticated": true
}
```

### 8.2 Data APIs

#### GET /data
Lấy dữ liệu sensor (yêu cầu xác thực).

**Response:**
```json
{
  "fire": 2500,
  "co2": 800,
  "temp": 28.5,
  "hum": 65.0,
  "buzzer": false,
  "pump": false,
  "fan": false,
  "recipientEmail": "user@example.com",
  "ssid": "MyWiFi",
  "fireThresh": 3000,
  "co2Thresh": 1500,
  "tempThresh": 50.0
}
```

#### GET /history
Lấy lịch sử sự kiện.

**Response:**
```json
[
  {
    "id": 123456,
    "timestamp": 1704067200,
    "message": "CHÁY! Nhiệt độ cao + Lửa",
    "isDanger": true
  },
  {
    "id": 123400,
    "timestamp": 1704067100,
    "message": "An toàn. Các chỉ số đã ổn định.",
    "isDanger": false
  }
]
```

#### POST /settings
Cập nhật cấu hình.

**Request:**
```json
{
  "fireThreshold": 3000,
  "co2Threshold": 1500,
  "tempThreshold": 50.0,
  "recipientEmail": "user@example.com",
  "password": "newPassword123"
}
```

**Response:**
```json
{
  "status": "ok"
}
```

---

## 9. Xử lý lỗi

### 9.1 Frontend Error Handling

| Lỗi | Xử lý |
|-----|-------|
| Request timeout | Retry 2 lần với exponential backoff |
| 401 Unauthorized | Auto logout, redirect login |
| 503 Service Unavailable | Hiển thị "Mất kết nối" |
| NaN từ sensor | Sanitize → 0 |
| Network error | Debounce 3 lần trước khi báo disconnect |

### 9.2 Backend Error Handling

| Lỗi | Xử lý |
|-----|-------|
| Semaphore timeout | Trả về 503 |
| DHT22 đọc NaN | Giữ giá trị cũ |
| WiFi disconnect | LED đỏ, tiếp tục hoạt động offline |
| SMTP fail | Log error, tiếp tục hoạt động |

### 9.3 ErrorBoundary Component

```tsx
<ErrorBoundary>
  <App />
</ErrorBoundary>

// Catch JavaScript errors và hiển thị fallback UI
```

---

## 10. Hướng dẫn triển khai

### 10.1 Yêu cầu

- Node.js 18+
- PlatformIO IDE
- ESP32-S3 board
- Các cảm biến theo danh sách

### 10.2 Build Frontend

```bash
cd front_end
npm install
npm run build

# Copy to ESP32 SPIFFS
cp -r dist/* ../data/
```

### 10.3 Upload ESP32

```bash
# Upload firmware
pio run --target upload

# Upload filesystem (SPIFFS)
pio run --target uploadfs
```

### 10.4 Cấu hình WiFi

1. Khi ESP32 khởi động lần đầu, nó tạo AP "ESP32_Fire_Smart_V2"
2. Kết nối vào AP này
3. Truy cập 192.168.4.1 để cấu hình WiFi
4. Sau khi kết nối, truy cập `http://wsn6.local` hoặc IP của ESP32

### 10.5 Factory Reset

Giữ nút Reset (GPIO 21) trong 5 giây để:
- Xóa cấu hình WiFi
- Xóa mật khẩu (reset về "admin")
- Khởi động lại

---





*Tài liệu này được tạo tự động và cập nhật theo phiên bản mới nhất của hệ thống.*

