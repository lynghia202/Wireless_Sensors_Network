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
#define PIN_BUTTON_RESET 21
#define PIN_DHT          26
#define PIN_MQ02         34
#define PIN_IR_SENSOR    33

#define PIN_LED_WIFI_OK  22
#define PIN_LED_WIFI_ERR 23

#define PIN_RELAY_FAN    17
#define PIN_RELAY_PUMP   18
#define PIN_RELAY_BUZZER 19
#define RELAY_ACTIVE     LOW

// ====== CẤU HÌNH DHT ======
#define DHTTYPE DHT22
DHT dht(PIN_DHT, DHTTYPE);

// ====== CẤU HÌNH GỬI MAIL ======
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "esp32test2k4@gmail.com"
#define AUTHOR_PASSWORD "blev fslf uesq rgra"
String g_recipientEmail = "lynghia736@gmail.com";

// ====== BIẾN TOÀN CỤC ======
volatile int g_rawFireValue = 0;
volatile int g_rawCo2Value = 0;
volatile float g_temperature = 0.0;
volatile float g_humidity = 0.0;

volatile bool g_buzzerState = false;
volatile bool g_pumpState = false;
volatile bool g_fanState = false;

volatile int g_fireThreshold = 3000;
volatile int g_co2Threshold = 1500;
volatile float g_tempThreshold = 50.0;

const char* CONFIG_FILE = "/config.json";

WiFiManager wm;
AsyncWebServer server(80);

SemaphoreHandle_t xSensorDataMutex;
QueueHandle_t xAlertQueue;
volatile unsigned long g_lastAlertSentTime = 0;
#define ALERT_COOLDOWN_MS 15000

std::vector<String> g_historyLog;
#define MAX_HISTORY_SIZE 50

WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);

void sensorReadTask(void *pvParameters);
void logicControlTask(void *pvParameters);
void alertSendTask(void *pvParameters);
void configureWebServer();
void loadSettings();
void saveSettings();
void checkResetButton();
void updateStatusLEDs();

// ====== THÔNG SỐ DATASHEET MQ2 ======
#define RL_VALUE         10.0  
#define RO_CLEAN_AIR_FACTOR 9.83 
#define MQ2_A  672  
#define MQ2_B -2.06
float g_MQ2_R0 = 20.0;

float getVoltage(int raw_adc) {
  return raw_adc * (3.3 / 4095.0);
}

float getResistance(int raw_adc) {
  float v_out = getVoltage(raw_adc);
  if (v_out == 0) return 99999; 
  return (5 - v_out) * RL_VALUE / v_out; 
}

int MQGetPPM(int mq_pin) {
  float rs = getResistance(analogRead(mq_pin));
  float ratio = rs / g_MQ2_R0;
  double ppm = MQ2_A * pow(ratio, MQ2_B);
  
  if (ppm < 0) ppm = 0;
  if (ppm > 10000) ppm = 10000;
  return (int)ppm;
}

// --- CÁC HÀM QUẢN LÝ CẤU HÌNH  ---

void loadSettings() {
  if (!SPIFFS.exists(CONFIG_FILE)) {
    Serial.println("Chưa có file cấu hình, dùng tham số mặc định.");
    return;
  }

  File file = SPIFFS.open(CONFIG_FILE, "r");
  if (!file) return;

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("Lỗi đọc file cấu hình JSON!");
    return;
  }

  if (doc["fire"].is<int>()) g_fireThreshold = doc["fire"].as<int>();
  if (doc["co2"].is<int>())  g_co2Threshold = doc["co2"].as<int>();
  if (doc["temp"].is<double>()) g_tempThreshold = doc["temp"].as<float>();
  if (doc["email"].is<const char*>()) g_recipientEmail = doc["email"].as<String>();

  Serial.printf("Đã tải Config: Fire=%d, CO2=%d, Temp=%.1f, Email=%s\n",
                g_fireThreshold, g_co2Threshold, g_tempThreshold, g_recipientEmail.c_str());
}

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

void addHistoryEvent(String message, bool isDanger) {
  String eventJson = "{";
  eventJson += String("\"id\": ") + String(millis()) + ",";
  eventJson += String("\"timestamp\": ") + String(time(nullptr)) + ",";
  eventJson += String("\"message\": \"") + message + "\",";
  eventJson += String("\"isDanger\": ") + String(isDanger ? "true" : "false");
  eventJson += "}";

  if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    g_historyLog.push_back(eventJson);
    if (g_historyLog.size() > MAX_HISTORY_SIZE) {
      g_historyLog.erase(g_historyLog.begin());
    }
    xSemaphoreGive(xSensorDataMutex);
  }
}

// --- SETUP ---

void setup() {
  Serial.begin(115200);
  while(!Serial && millis() < 100);
  
  Serial.println("\n=========================================");
  Serial.println("HỆ THỐNG BẮT ĐẦU KHỞI ĐỘNG...");

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
  digitalWrite(PIN_RELAY_BUZZER, !RELAY_ACTIVE);
  digitalWrite(PIN_RELAY_PUMP, !RELAY_ACTIVE);
  digitalWrite(PIN_RELAY_FAN, !RELAY_ACTIVE);

  dht.begin();

  if (!SPIFFS.begin(true)) {
    Serial.println("Lỗi Mount SPIFFS");
  }
  loadSettings();

  WiFi.mode(WIFI_STA);
  wm.setConfigPortalTimeout(180);
  
  Serial.println("Đang kết nối WiFi...");
  if (!wm.autoConnect("ESP32_Fire_Smart_V2")) {
    Serial.println("Chạy offline.");
  } else {
    Serial.println("WiFi Connected!");
  }

  Serial.println("=========================================\n");

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
}

// --- WEB SERVER ---

void configureWebServer() {
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      json += String("\"fire\":") + String(g_rawFireValue);
      json += String(", \"co2\":") + String(g_rawCo2Value);
      json += String(", \"temp\":") + String(g_temperature, 1);
      json += String(", \"hum\":") + String(g_humidity, 1);

      json += String(", \"buzzer\":") + String(g_buzzerState ? "true" : "false");
      json += String(", \"pump\":") + String(g_pumpState ? "true" : "false");
      json += String(", \"fan\":") + String(g_fanState ? "true" : "false");

      json += String(", \"recipientEmail\":\"") + g_recipientEmail + "\"";
      json += String(", \"ssid\":\"") + WiFi.SSID() + "\"";
      json += String(", \"fireThresh\":") + String(g_fireThreshold);
      json += String(", \"co2Thresh\":") + String(g_co2Threshold);
      json += String(", \"tempThresh\":") + String(g_tempThreshold, 1);

      xSemaphoreGive(xSensorDataMutex);
      json += "}";
      request->send(200, "application/json", json);
    } else {
      request->send(503, "application/json", "{\"error\":\"Busy\"}");
    }
  });

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/settings", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();
    bool changed = false;

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

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
}

// --- TASKS ---

void sensorReadTask(void *pvParameters) {
  unsigned long lastDHTRead = 0;

  for (;;) {
    int rawFire = 4095 - analogRead(PIN_IR_SENSOR);
    int rawMq = MQGetPPM(PIN_MQ02);

    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      g_rawFireValue = rawFire;
      g_rawCo2Value = rawMq;
      xSemaphoreGive(xSensorDataMutex);
    }

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

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void logicControlTask(void *pvParameters) {
  static bool lastAlarmState = false;

  for (;;) {
    int irVal, gasVal;
    float tempVal;
    int irThresh, gasThresh;
    float tempThresh;

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

    bool isFire = false;
    bool isGasLeak = false;

    if (irVal > irThresh && tempVal > 45.0) isFire = true;
    else if (tempVal > tempThresh) isFire = true;

    if (gasVal > gasThresh) {
        isGasLeak = true;
        if (tempVal > 40.0) isFire = true;
    }

    bool buzzer = false, pump = false, fan = false;
    if (isFire) {
        buzzer = true; pump = true; fan = false;
    } else if (isGasLeak) {
        buzzer = true; pump = false; fan = true;
    }

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
        addHistoryEvent("An toàn. Các chỉ số đã ổn định.", false);
    }
    lastAlarmState = currentAlarmState;

    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      g_buzzerState = buzzer;
      g_pumpState = pump;
      g_fanState = fan;
      xSemaphoreGive(xSensorDataMutex);
    }

    digitalWrite(PIN_RELAY_BUZZER, buzzer ? RELAY_ACTIVE : !RELAY_ACTIVE);
    digitalWrite(PIN_RELAY_PUMP, pump ? RELAY_ACTIVE : !RELAY_ACTIVE);
    digitalWrite(PIN_RELAY_FAN, fan ? RELAY_ACTIVE : !RELAY_ACTIVE);

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void alertSendTask(void *pvParameters) {
  bool req;
  for (;;) {
    if (xQueueReceive(xAlertQueue, &req, portMAX_DELAY) == pdPASS) {
        if (!req) continue;
        if (WiFi.status() != WL_CONNECTED) continue;

        int ir, gas; float t, h; String emailTo;
        if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
             ir = g_rawFireValue; gas = g_rawCo2Value;
             t = g_temperature; h = g_humidity;
             emailTo = g_recipientEmail;
             xSemaphoreGive(xSensorDataMutex);
        } else continue;

        ssl_client.setInsecure();
        if (!smtp.connect(SMTP_HOST, SMTP_PORT)) continue;
        if (!smtp.authenticate(AUTHOR_EMAIL, AUTHOR_PASSWORD, readymail_auth_password)) continue;

        SMTPMessage msg;
        msg.headers.add(rfc822_from, String("ESP32 FireAlarm <") + AUTHOR_EMAIL + ">");
        msg.headers.add(rfc822_to, emailTo);
        msg.headers.add(rfc822_subject, "[KHẨN CẤP] Cảnh báo An toàn!");

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

void checkResetButton() {
  static unsigned long pressStartTime = 0;
  static bool isPressing = false;

  if (digitalRead(PIN_BUTTON_RESET) == HIGH) {
    if (!isPressing) {
      isPressing = true;
      pressStartTime = millis();
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

void updateStatusLEDs() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(PIN_LED_WIFI_OK, LOW);
    digitalWrite(PIN_LED_WIFI_ERR, HIGH);
  } else {
    digitalWrite(PIN_LED_WIFI_OK, HIGH);
    digitalWrite(PIN_LED_WIFI_ERR, LOW);
  }
}

void loop() {
  checkResetButton();
  updateStatusLEDs();
  vTaskDelay(pdMS_TO_TICKS(100));
}
