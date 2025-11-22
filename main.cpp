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

// Bật các tính năng: SMTP để gửi mail, DEBUG để in log (nếu dùng)
#define ENABLE_SMTP
#define ENABLE_DEBUG
#include <ReadyMail.h>
#include <WiFiClientSecure.h>
#include <time.h>

// ====== CẤU HÌNH CHÂN ======
// Các định nghĩa chân kết nối với phần cứng trên bo mạch ESP32
// Input Sensors
#define PIN_BUTTON_RESET 21  // Nút nhấn Reset WiFi (kéo xuống GND qua trở 10k)
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
#define RELAY_ACTIVE     LOW // Nhiều module relay là active-low

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
  Serial.begin(115200);

  // Khởi tạo chế độ các chân I/O
  pinMode(PIN_BUTTON_RESET, INPUT);
  pinMode(PIN_LED_WIFI_OK, OUTPUT);
  pinMode(PIN_LED_WIFI_ERR, OUTPUT);
  pinMode(PIN_IR_SENSOR, INPUT);
  pinMode(PIN_MQ02, INPUT);
  pinMode(PIN_RELAY_BUZZER, OUTPUT);
  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_FAN, OUTPUT);

  // Đặt trạng thái ban đầu cho LED/Relay (tắt)
  digitalWrite(PIN_LED_WIFI_OK,  HIGH);
  digitalWrite(PIN_LED_WIFI_ERR, HIGH);

  digitalWrite(PIN_RELAY_BUZZER, !RELAY_ACTIVE);
  digitalWrite(PIN_RELAY_PUMP, !RELAY_ACTIVE);
  digitalWrite(PIN_RELAY_FAN, !RELAY_ACTIVE);

  dht.begin();

  // Mount SPIFFS (tạo nếu chưa có) để lưu/đọc file cấu hình
  if (!SPIFFS.begin(true)) {
    Serial.println("Lỗi Mount SPIFFS");
    return;
  }

  // Tải cấu hình từ SPIFFS khi khởi động
  loadSettings();

  // Cấu hình WiFi (station) và portal nếu cần
  WiFi.mode(WIFI_STA);
  wm.setConfigPortalTimeout(180); // timeout portal (s)
  digitalWrite(PIN_LED_WIFI_ERR, LOW);
  digitalWrite(PIN_LED_WIFI_OK,  HIGH);

  // autoConnect sẽ mở captive portal nếu không có kết nối
  if (!wm.autoConnect("ESP32_Fire_Smart_V2")) {
    Serial.println("Chưa kết nối WiFi, chạy offline.");
  } else {
    Serial.println("Đã kết nối WiFi: " + WiFi.SSID());
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    digitalWrite(PIN_LED_WIFI_ERR, HIGH);
    digitalWrite(PIN_LED_WIFI_OK, LOW);
  }

  // Khởi tạo mDNS để truy cập bằng tên local (wsn6.local)
  if (!MDNS.begin("wsn6")) {
    Serial.println("Lỗi khởi tạo mDNS!");
  } else {
    Serial.println("mDNS đã khởi động: http://wsn6.local");
    MDNS.addService("http", "tcp", 80);
  }

  delay(500);
  configureWebServer();
  server.begin();

  // Tạo mutex và queue
  xSensorDataMutex = xSemaphoreCreateMutex();
  xAlertQueue = xQueueCreate(5, sizeof(bool));

  // Đồng bộ thời gian từ NTP (GMT+7)
  configTime(7 * 3600, 0, "pool.ntp.org");

  // Tạo các FreeRTOS tasks: đọc cảm biến, logic, gửi alert
  xTaskCreatePinnedToCore(sensorReadTask, "SensorReadTask", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(logicControlTask, "LogicControlTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(alertSendTask, "AlertSendTask", 8192, NULL, 0, NULL, 0);
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
    // 1. ĐỌC CẢM BIẾN NHANH (MQ2, IR) - cập nhật liên tục mỗi ~100ms
    int rawFire = 4095 - analogRead(PIN_IR_SENSOR); // đảo chiều nếu cần
    int rawMq = analogRead(PIN_MQ02);

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

// loop chính: chạy ở task chính, chỉ kiểm tra nút và LED
void loop() {
  checkResetButton();
  updateStatusLEDs();
  vTaskDelay(pdMS_TO_TICKS(100));
}