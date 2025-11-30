#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include <vector>
#include <DHT.h>
#include <ESPmDNS.h>


#define ENABLE_SMTP
#define ENABLE_DEBUG
#include <ReadyMail.h>
#include <WiFiClientSecure.h>
#include <time.h>

// ====== CẤU HÌNH CHÂN ======
// Các định nghĩa chân kết nối với phần cứng trên bo mạch ESP32
// Input Sensors
#define PIN_BUTTON_RESET 21  // Nút nhấn Reset WiFi 
#define PIN_DHT          26  // Chân nối DHT22
#define PIN_MQ02         34  // Đầu vào analog cho cảm biến khí (MQ2/MQ135)
#define PIN_IR_SENSOR    33  // Đầu vào analog/digital cho cảm biến IR (lửa)

// Output Indicators (LED báo trạng thái mạng)
#define PIN_LED_WIFI_OK  22  // LED xanh: có mạng
#define PIN_LED_WIFI_ERR 23  // LED đỏ: mất mạng

// Output Relays
#define PIN_RELAY_FAN    17
#define PIN_RELAY_PUMP   18
#define PIN_RELAY_BUZZER 19
#define RELAY_ACTIVE     LOW 

// ====== CẤU HÌNH DHT ======
#define DHTTYPE DHT22
// Thực thể DHT để đọc nhiệt độ/độ ẩm
DHT dht(PIN_DHT, DHTTYPE);

// ====== CẤU HÌNH GỬI MAIL ======
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
// Thông tin email người gửi (ví dụ); chú ý bảo mật khi public
#define AUTHOR_EMAIL "esp32test2k4@gmail.com"
#define AUTHOR_PASSWORD "blev fslf uesq rgra"
// Email mặc định nếu chưa cấu hình (có thể bị ghi đè bởi file)
String g_recipientEmail = "lynghia736@gmail.com";

// --- BIẾN TOÀN CỤC CHO VIỆC DEBUG WIFI ---
unsigned long t_wifi_lost_start = 0;
bool wasWiFiConnected = true; // Giả định ban đầu là có mạng (hoặc cập nhật trong setup)

// ====== BIẾN TOÀN CỤC ======
// Các biến volatile vì chúng có thể truy cập từ nhiều task/ISR
volatile int g_rawFireValue = 0;   // Giá trị đọc từ cảm biến IR (lửa)
volatile int g_rawCo2Value = 0;    // Giá trị đọc từ cảm biến khí (MQ2)
volatile float g_temperature = 0.0;
volatile float g_humidity = 0.0;

volatile bool g_buzzerState = false; // Trạng thái relay/buzzer hiện tại
volatile bool g_pumpState = false;
volatile bool g_fanState = false;

// Ngưỡng cảnh báo mặc định (có thể ghi đè bởi config trên SPIFFS)
volatile int g_fireThreshold = 3000;
volatile int g_co2Threshold = 1500;
volatile float g_tempThreshold = 50.0;

const char* CONFIG_FILE = "/config.json"; // File lưu cấu hình trên SPIFFS

// Thư viện quản lý WiFi và webserver
WiFiManager wm;
AsyncWebServer server(80);

// Mutex để bảo vệ truy cập dữ liệu cảm biến/biến toàn cục giữa các task
SemaphoreHandle_t xSensorDataMutex;

// Hàng đợi để gửi yêu cầu gửi cảnh báo (alert) giữa logic và task gửi mail
QueueHandle_t xAlertQueue;
volatile unsigned long g_lastAlertSentTime = 0; // Để tránh gửi mail dồn dập
#define ALERT_COOLDOWN_MS 15000 // 15s giữa các lần gửi cảnh báo

// Lưu lịch sử sự kiện dạng JSON string trong RAM (vòng tròn)
std::vector<String> g_historyLog;
#define MAX_HISTORY_SIZE 50

// Cấu hình SMTP client (ReadyMail)
WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);

// Khai báo prototype các hàm để sắp xếp file
void sensorReadTask(void *pvParameters);
void logicControlTask(void *pvParameters);
void alertSendTask(void *pvParameters);
void configureWebServer();
void loadSettings();
void saveSettings();
void checkResetButton();
void updateStatusLEDs();
// ====== THÔNG SỐ DATASHEET MQ2 ======
// Điện trở tải trên bo mạch 
#define RL_VALUE         10.0  
// Hệ số Rs/R0 trong không khí sạch 
#define RO_CLEAN_AIR_FACTOR 9.83 

// Hằng số cho đường LPG/Smoke (Tính từ Fig 1)
#define MQ2_A  672  
#define MQ2_B -2.06
float g_MQ2_R0 = 20.0; // Giá trị R0 hiệu chuẩn (đo trong không khí sạch)

// Hàm 1: Đổi giá trị ADC (0-4095) sang Điện áp (V)
float getVoltage(int raw_adc) {
  return raw_adc * (3.3 / 4095.0);
}

// Hàm 2: Tính điện trở cảm biến (Rs)
float getResistance(int raw_adc) {
  float v_out = getVoltage(raw_adc);
  if (v_out == 0) return 99999; 
  return (5 - v_out) * RL_VALUE / v_out; 
}
// Hàm 3: Tính nồng độ khí PPM từ giá trị ADC
int MQGetPPM(int mq_pin) {
  float rs = getResistance(analogRead(mq_pin));
  float ratio = rs / g_MQ2_R0; // Tính tỷ số Rs/R0
    // Công thức hàm mũ: PPM = a * (ratio)^b
  double ppm = MQ2_A * pow(ratio, MQ2_B);
  
  if (ppm < 0) ppm = 0;
  if (ppm > 10000) ppm = 10000; // Giới hạn max của Datasheet là 10000
  return (int)ppm;
}

// --- CÁC HÀM QUẢN LÝ CẤU HÌNH  ---

// loadSettings: đọc file cấu hình JSON từ SPIFFS và cập nhật ngưỡng, email
// Lưu ý: hàm này chỉ đọc và gán các biến toàn cục; không dùng mutex vì
// đang trong setup trước khi task song song bắt đầu (an toàn ở đây).
void loadSettings() {
  if (!SPIFFS.exists(CONFIG_FILE)) {
    Serial.println("Chưa có file cấu hình, dùng tham số mặc định.");
    return;
  }

  File file = SPIFFS.open(CONFIG_FILE, "r");
  if (!file) return;

  // Dùng JsonDocument (DynamicJsonDocument cũ bị deprecated, nhưng dùng tùy
  // theo phiên bản ArduinoJson). 512 bytes thường đủ cho config nhỏ.
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("Lỗi đọc file cấu hình JSON!");
    return;
  }

  // Kiểm tra kiểu và tồn tại trước khi gán (an toàn và tránh exception)
  if (doc["fire"].is<int>()) g_fireThreshold = doc["fire"].as<int>();
  if (doc["co2"].is<int>())  g_co2Threshold = doc["co2"].as<int>();
  if (doc["temp"].is<double>()) g_tempThreshold = doc["temp"].as<float>();
  if (doc["email"].is<const char*>()) g_recipientEmail = doc["email"].as<String>();

  Serial.printf("Đã tải Config: Fire=%d, CO2=%d, Temp=%.1f, Email=%s\n",
                g_fireThreshold, g_co2Threshold, g_tempThreshold, g_recipientEmail.c_str());
}

// saveSettings: lưu các ngưỡng và email hiện tại vào file JSON trên SPIFFS
void saveSettings() {
  DynamicJsonDocument doc(512);
  doc["fire"] = g_fireThreshold;
  doc["co2"] = g_co2Threshold;
  doc["temp"] = g_tempThreshold;
  doc["email"] = g_recipientEmail;

  File file = SPIFFS.open(CONFIG_FILE, "w");
  if (!file) {
    Serial.println("Lỗi mở file config để ghi!");
    return;
  }
  serializeJson(doc, file);
  file.close();
  Serial.println("Đã lưu cấu hình mới vào SPIFFS.");
}

// addHistoryEvent: thêm một sự kiện (dạng JSON string) vào lịch sử trong RAM
// Ghi chú: Hàm này dùng mutex vì có thể được gọi từ nhiều task.
void addHistoryEvent(String message, bool isDanger) {
  // Tạo chuỗi JSON ngắn cho sự kiện (id, timestamp, message, isDanger)
  String eventJson = "{";
  eventJson += String("\"id\": ") + String(millis()) + ",";
  eventJson += String("\"timestamp\": ") + String(time(nullptr)) + ",";
  eventJson += String("\"message\": \"") + message + "\",";
  eventJson += String("\"isDanger\": ") + String(isDanger ? "true" : "false");
  eventJson += "}";

  if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    g_historyLog.push_back(eventJson);
    // Giữ lại kích thước lịch sử không vượt quá MAX_HISTORY_SIZE
    if (g_historyLog.size() > MAX_HISTORY_SIZE) {
      g_historyLog.erase(g_historyLog.begin());
    }
    xSemaphoreGive(xSensorDataMutex);
  }
}

// --- SETUP ---

void setup() {
  // [MỐC 0] Bắt đầu cấp nguồn (gần như 0ms)
  unsigned long t_boot_start = millis(); 
  
  Serial.begin(115200);
  // Đợi Serial ổn định một chút để không mất chữ đầu
  while(!Serial && millis() < 100); 
  
  Serial.println("\n=========================================");
  Serial.println("[BOOT] HỆ THỐNG BẮT ĐẦU KHỞI ĐỘNG...");
  Serial.print("[TIME] T0 (Power On): "); Serial.println(t_boot_start);

  // 1. KHỞI TẠO PHẦN CỨNG (GPIO, DHT)
  pinMode(PIN_BUTTON_RESET, INPUT);
  pinMode(PIN_LED_WIFI_OK, OUTPUT);
  pinMode(PIN_LED_WIFI_ERR, OUTPUT);
  pinMode(PIN_IR_SENSOR, INPUT);
  pinMode(PIN_MQ02, INPUT);
  pinMode(PIN_RELAY_BUZZER, OUTPUT);
  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_FAN, OUTPUT);

  digitalWrite(PIN_LED_WIFI_OK,  HIGH);
  digitalWrite(PIN_LED_WIFI_ERR, HIGH);
  // Quan trọng: Tắt Relay ngay lập tức để an toàn (Safe State)
  digitalWrite(PIN_RELAY_BUZZER, !RELAY_ACTIVE);
  digitalWrite(PIN_RELAY_PUMP, !RELAY_ACTIVE);
  digitalWrite(PIN_RELAY_FAN, !RELAY_ACTIVE);

  dht.begin();
  
  // [MỐC 1] Xong phần cứng cơ bản
  unsigned long t_hw_init_done = millis();
  Serial.print("[TIME] T1 (Hardware Init): "); Serial.println(t_hw_init_done);

  // 2. KHỞI TẠO FILE SYSTEM & CẤU HÌNH
  if (!SPIFFS.begin(true)) {
    Serial.println("[ERR] Lỗi Mount SPIFFS");
  }
  loadSettings();

  // [MỐC 2] Xong File System
  unsigned long t_fs_done = millis();
  Serial.print("[TIME] T2 (SPIFFS & Settings): "); Serial.println(t_fs_done);

  // 3. KẾT NỐI WIFI (Giai đoạn lâu nhất)
  WiFi.mode(WIFI_STA);
  wm.setConfigPortalTimeout(180);
  
  Serial.println("[BOOT] Đang kết nối WiFi...");
  if (!wm.autoConnect("ESP32_Fire_Smart_V2")) {
    Serial.println("[WARN] Chạy offline.");
  } else {
    Serial.println("[INFO] WiFi Connected!");
  }

  // [MỐC 3] Xong WiFi - Hệ thống sẵn sàng
  unsigned long t_wifi_done = millis();
  
  // --- TỔNG KẾT ---
  Serial.println("-----------------------------------------");
  Serial.println("[PERF] THỐNG KÊ THỜI GIAN BOOT:");
  Serial.print("  1. Init Phần cứng: "); Serial.print(t_hw_init_done - t_boot_start); Serial.println(" ms");
  Serial.print("  2. Load Dữ liệu:   "); Serial.print(t_fs_done - t_hw_init_done); Serial.println(" ms");
  Serial.print("  3. Kết nối WiFi:   "); Serial.print(t_wifi_done - t_fs_done); Serial.println(" ms");
  Serial.print("  => TỔNG CỘNG:      "); Serial.print(t_wifi_done); Serial.println(" ms");
  Serial.println("=========================================\n");

  // Khởi tạo các dịch vụ sau khi đã có mạng
  if (MDNS.begin("wsn6")) {
    MDNS.addService("http", "tcp", 80);
  }
  configureWebServer();
  server.begin();
  
  xSensorDataMutex = xSemaphoreCreateMutex();
  xAlertQueue = xQueueCreate(5, sizeof(bool));
  configTime(7 * 3600, 0, "pool.ntp.org");

  xTaskCreatePinnedToCore(sensorReadTask, "SensorReadTask", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(logicControlTask, "LogicControlTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(alertSendTask, "AlertSendTask", 8192, NULL, 0, NULL, 0);
  
  // Cập nhật trạng thái mạng ban đầu cho hàm debug
  wasWiFiConnected = (WiFi.status() == WL_CONNECTED);
}

// --- WEB SERVER ---

// configureWebServer: khai báo các route API của máy (REST-like)
void configureWebServer() {
  // Cho phép CORS để frontend có thể request từ origin khác
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  // API /data: trả về trạng thái cảm biến + cài đặt hiện tại (JSON string)
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      // Sensor Data
      json += String("\"fire\":") + String(g_rawFireValue);
      json += String(", \"co2\":") + String(g_rawCo2Value);
      json += String(", \"temp\":") + String(g_temperature, 1);
      json += String(", \"hum\":") + String(g_humidity, 1);

      // Relay Status
      json += String(", \"buzzer\":") + String(g_buzzerState ? "true" : "false");
      json += String(", \"pump\":") + String(g_pumpState ? "true" : "false");
      json += String(", \"fan\":") + String(g_fanState ? "true" : "false");

      // Settings Data (Để Frontend đồng bộ)
      json += String(", \"recipientEmail\":\"") + g_recipientEmail + "\"";
      json += String(", \"ssid\":\"") + WiFi.SSID() + "\"";
      json += String(", \"fireThresh\":") + String(g_fireThreshold);
      json += String(", \"co2Thresh\":") + String(g_co2Threshold);
      json += String(", \"tempThresh\":") + String(g_tempThreshold, 1);

      xSemaphoreGive(xSensorDataMutex);
      json += "}";
      request->send(200, "application/json", json);
    } else {
      // Nếu bận (mutex không available) trả lỗi 503
      request->send(503, "application/json", "{\"error\":\"Busy\"}");
    }
  });

  // API /settings: nhận POST JSON để cập nhật cài đặt
  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/settings", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();
    bool changed = false;

    // Khi cập nhật các biến chung, cần lock mutex
    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (jsonObj["fireThreshold"].is<int>()) {
            g_fireThreshold = jsonObj["fireThreshold"].as<int>();
            changed = true;
        }
        if (jsonObj["co2Threshold"].is<int>()) {
            g_co2Threshold = jsonObj["co2Threshold"].as<int>();
            changed = true;
        }
        if (jsonObj["tempThreshold"].is<double>()) {
            g_tempThreshold = jsonObj["tempThreshold"].as<float>();
            changed = true;
        }
        if (jsonObj["recipientEmail"].is<const char*>()) {
            g_recipientEmail = jsonObj["recipientEmail"].as<String>();
            changed = true;
        }

        // Nếu có thay đổi, lưu vào SPIFFS
        if (changed) {
            saveSettings();
        }

        xSemaphoreGive(xSensorDataMutex);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(500, "application/json", "{\"status\":\"fail\"}");
    }
  });
  server.addHandler(handler);

  // API /history: trả về danh sách sự kiện đã lưu trong RAM
  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    String jsonResponse = "[";
    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      for (size_t i = 0; i < g_historyLog.size(); ++i) {
        jsonResponse += g_historyLog[i];
        if (i < g_historyLog.size() - 1) jsonResponse += ",";
      }
      xSemaphoreGive(xSensorDataMutex);
    }
    jsonResponse += "]";
    request->send(200, "application/json", jsonResponse);
  });

  // Phục vụ file tĩnh từ SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
}

// --- TASKS (Giữ nguyên logic cũ) ---

// sensorReadTask: đọc các cảm biến
// - đọc nhanh các cảm biến analog (IR, MQ) ~100ms
// - đọc chậm DHT22 ~2000ms để tránh đọc quá nhanh
// Cập nhật giá trị vào biến toàn cục có bảo vệ mutex
void sensorReadTask(void *pvParameters) {
  unsigned long lastDHTRead = 0;

  for (;;) {
    unsigned long t_start_read = micros();
    // 1. ĐỌC CẢM BIẾN NHANH (MQ2, IR) - cập nhật liên tục mỗi ~100ms
    int rawFire = 4095 - analogRead(PIN_IR_SENSOR); // đảo chiều nếu cần
    int rawMq = MQGetPPM(PIN_MQ02);
    unsigned long t_end_read = micros();
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000) {
        Serial.print("[TEST] Thời gian đọc Sensor (Analog): ");
        Serial.print(t_end_read - t_start_read);
        Serial.println(" us (micro-seconds)");
        lastPrint = millis();
    }

    // Cập nhật các biến toàn cục (dùng mutex)
    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      g_rawFireValue = rawFire;
      g_rawCo2Value = rawMq;
      xSemaphoreGive(xSensorDataMutex);
    }

    // 2. ĐỌC CẢM BIẾN CHẬM (DHT22) - tối thiểu 2s giữa các lần đọc
    if (millis() - lastDHTRead > 2000) {
        lastDHTRead = millis();
        float t = dht.readTemperature();
        float h = dht.readHumidity();

        if (!isnan(t) && !isnan(h)) {
             if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                g_temperature = t;
                g_humidity = h;
                xSemaphoreGive(xSensorDataMutex);
             }
        }
    }

    // 3. Nghỉ 100ms giúp giảm tải CPU và cho phản ứng nhanh với thay đổi
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// logicControlTask: đọc giá trị từ biến chung, so sánh với ngưỡng, điều khiển relay
// - Xử lý logic kết hợp giữa IR, nhiệt độ, gas
// - Gửi yêu cầu alert qua queue khi phát hiện nguy hiểm
void logicControlTask(void *pvParameters) {
  static bool lastAlarmState = false;

  for (;;) {
    int irVal, gasVal;
    float tempVal;
    int irThresh, gasThresh;
    float tempThresh;

    // Lấy snapshot dữ liệu dưới mutex để xử lý an toàn
    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      irVal = g_rawFireValue;
      gasVal = g_rawCo2Value;
      tempVal = g_temperature;
      irThresh = g_fireThreshold;
      gasThresh = g_co2Threshold;
      tempThresh = g_tempThreshold;
      xSemaphoreGive(xSensorDataMutex);
    } else {
      vTaskDelay(100); continue;
    }
    unsigned long t_start_logic = micros();
    // Logic kết hợp để xác định Fire / GasLeak
    bool isFire = false;
    bool isGasLeak = false;

    if (irVal > irThresh && tempVal > 45.0) isFire = true;
    else if (tempVal > tempThresh) isFire = true;

    if (gasVal > gasThresh) {
        isGasLeak = true;
        if (tempVal > 40.0) isFire = true; // gas + nhiệt cao -> có thể cháy
    }

    // Quyết định relay
    bool buzzer = false, pump = false, fan = false;
    if (isFire) {
        buzzer = true; pump = true; fan = false;
    } else if (isGasLeak) {
        buzzer = true; pump = false; fan = true;
    }
    unsigned long t_end_logic = micros();
    static unsigned long lastPrintLogic = 0;
    if (millis() - lastPrintLogic > 2000) {
       Serial.print("[TEST] Thời gian xử lý Logic: ");
       Serial.print(t_end_logic - t_start_logic);
       Serial.println(" us");
       lastPrintLogic = millis();
    }
    // Nếu mới phát hiện cảnh báo, push queue để task gửi mail xử lý
    bool currentAlarmState = (isFire || isGasLeak);
    if (currentAlarmState && !lastAlarmState) {
        if (millis() - g_lastAlertSentTime > ALERT_COOLDOWN_MS) {
             bool cmd = true;
             xQueueSend(xAlertQueue, &cmd, 0);
             g_lastAlertSentTime = millis();
             String msg = isFire ? "CHÁY! Nhiệt độ cao + Lửa" : "Rò rỉ khí Gas/Khói";
             addHistoryEvent(msg, true);
        }
    } else if (!currentAlarmState && lastAlarmState) {
        // Ghi log khi trở về trạng thái an toàn
        addHistoryEvent("An toàn. Các chỉ số đã ổn định.", false);
    }
    lastAlarmState = currentAlarmState;

    // Cập nhật trạng thái relay trong biến chung
    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      g_buzzerState = buzzer;
      g_pumpState = pump;
      g_fanState = fan;
      xSemaphoreGive(xSensorDataMutex);
    }

    // Ghi ra chân điều khiển relay (không cần mutex vì thao tác I/O độc lập)
    digitalWrite(PIN_RELAY_BUZZER, buzzer ? RELAY_ACTIVE : !RELAY_ACTIVE);
    digitalWrite(PIN_RELAY_PUMP, pump ? RELAY_ACTIVE : !RELAY_ACTIVE);
    digitalWrite(PIN_RELAY_FAN, fan ? RELAY_ACTIVE : !RELAY_ACTIVE);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// alertSendTask: nhận yêu cầu từ queue và gửi email cảnh báo (nếu có mạng)
// - Kết nối SMTP, soạn email HTML và gửi
void alertSendTask(void *pvParameters) {
  bool req;
  for (;;) {
    if (xQueueReceive(xAlertQueue, &req, portMAX_DELAY) == pdPASS) {
        if (!req) continue;
        if (WiFi.status() != WL_CONNECTED) continue; // chỉ gửi khi có mạng

        int ir, gas; float t, h; String emailTo;
        if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
             ir = g_rawFireValue; gas = g_rawCo2Value;
             t = g_temperature; h = g_humidity;
             emailTo = g_recipientEmail;
             xSemaphoreGive(xSensorDataMutex);
        } else continue;
        Serial.println("[TEST] Bắt đầu quy trình gửi Email...");
        unsigned long t_start_email = millis(); // Bắt đầu bấm giờ
        // Kết nối SMTP và gửi mail
        ssl_client.setInsecure(); // không verify cert (đơn giản)
        if (!smtp.connect(SMTP_HOST, SMTP_PORT)) continue;
        if (!smtp.authenticate(AUTHOR_EMAIL, AUTHOR_PASSWORD, readymail_auth_password)) continue;

        SMTPMessage msg;
        msg.headers.add(rfc822_from, String("ESP32 FireAlarm <") + AUTHOR_EMAIL + ">");
        msg.headers.add(rfc822_to, emailTo);
        msg.headers.add(rfc822_subject, "[KHẨN CẤP] Cảnh báo An toàn!");

        // Soạn thân email dạng HTML
        String html = "<div style='border:2px solid red; padding:20px;'>";
        html += "<h2 style='color:red;'>PHÁT HIỆN NGUY HIỂM!</h2>";
        html += "<p>Hệ thống ghi nhận các chỉ số vượt ngưỡng:</p><ul>";
        html += "<li><b>WiFi kết nối:</b> " + WiFi.SSID() + "</li>";
        html += "<li><b>Nhiệt độ:</b> " + String(t, 1) + " C</li>";
        html += "<li><b>Độ ẩm:</b> " + String(h, 1) + " %</li>";
        html += "<li><b>Gas:</b> " + String(gas) + "</li>";
        html += "<li><b>Lửa (IR):</b> " + String(ir) + "</li></ul></div>";

        msg.html.body(html);
        msg.timestamp = time(nullptr);
        smtp.send(msg);
        smtp.stop();
        Serial.println("Đã gửi mail.");
        unsigned long t_end_email = millis(); // Kết thúc bấm giờ
        Serial.print("[TEST] Tổng thời gian gửi Email (SMTP): ");
        Serial.print(t_end_email - t_start_email);
        Serial.println(" ms");
    }
  }
}

// checkResetButton: phát hiện giữ nút reset >5s để reset WiFiManager settings
void checkResetButton() {
  static unsigned long pressStartTime = 0;
  static bool isPressing = false;

  if (digitalRead(PIN_BUTTON_RESET) == HIGH) {
    if (!isPressing) {
      isPressing = true;
      pressStartTime = millis();
      Serial.println("Nút Reset được nhấn (High)...");
    }
    if (millis() - pressStartTime > 5000) {
      Serial.println("Đang Reset WifiManager...");
      digitalWrite(PIN_LED_WIFI_OK, HIGH);
      digitalWrite(PIN_LED_WIFI_ERR, LOW);
      wm.resetSettings();
      delay(1000);
      ESP.restart();
    }
  } else {
    isPressing = false;
  }
}

// updateStatusLEDs: cập nhật LED báo trạng thái WiFi theo kết nối
void updateStatusLEDs() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(PIN_LED_WIFI_OK, LOW);  // Active Low => LOW = sáng
    digitalWrite(PIN_LED_WIFI_ERR, HIGH);  // Tắt
  } else {
    digitalWrite(PIN_LED_WIFI_OK, HIGH); // Tắt
    digitalWrite(PIN_LED_WIFI_ERR, LOW);  // Sáng (Active Low)
  }
}


void debugWiFiRecovery() {
  // Lấy trạng thái hiện tại
  bool isConnected = (WiFi.status() == WL_CONNECTED);

  // TRƯỜNG HỢP 1: Đang có mạng -> Mất mạng
  if (wasWiFiConnected && !isConnected) {
    t_wifi_lost_start = millis(); // Bấm giờ ngay lập tức
    wasWiFiConnected = false;
    
    Serial.println("------------------------------------------------");
    Serial.println("[EVENT] PHÁT HIỆN MẤT KẾT NỐI WIFI!");
    Serial.print("[TIME] Timestamp: ");
    Serial.println(t_wifi_lost_start);
    Serial.println("[LED] Đèn báo lỗi (Đỏ) đã bật."); 
    // Lúc này trong hàm updateStatusLEDs() đèn đỏ sẽ sáng
  }

  // TRƯỜNG HỢP 2: Đang mất mạng -> Có mạng lại
  else if (!wasWiFiConnected && isConnected) {
    unsigned long t_reconnect_end = millis(); // Bấm giờ kết thúc
    unsigned long duration = t_reconnect_end - t_wifi_lost_start;
    
    wasWiFiConnected = true;

    Serial.println("[EVENT] ĐÃ KẾT NỐI LẠI THÀNH CÔNG!");
    Serial.print("[METRIC] Thời gian gián đoạn (Recovery Time): ");
    Serial.print(duration);
    Serial.println(" ms");
    Serial.print("[INFO] IP Mới được cấp: ");
    Serial.println(WiFi.localIP());
    Serial.println("------------------------------------------------");
  }
}
void loop() {
  checkResetButton();
  updateStatusLEDs();
  debugWiFiRecovery();
  vTaskDelay(pdMS_TO_TICKS(100));
}
