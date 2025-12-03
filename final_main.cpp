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

// ====== BIẾN TOÀN CỤC SENSOR ======
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

// ====== BIẾN ĐĂNG NHẬP & BẢO MẬT ======
String g_adminPass = "admin";
bool g_isDefaultPass = true;

// Quản lý phiên đăng nhập
IPAddress g_loggedInIP(0,0,0,0);
unsigned long g_lastActivityTime = 0;
#define SESSION_TIMEOUT 300000 // 5 phút

// Quản lý khóa đăng nhập (Brute-force protection)
#define MAX_LOGIN_ATTEMPTS 5
#define LOCKOUT_DURATION_BASE 30000  // 30 giây
#define LOCKOUT_MULTIPLIER 3         // Nhân lên mỗi lần bị khóa

struct LoginAttemptTracker {
    int failedCount = 0;
    unsigned long lockoutEndTime = 0;
    unsigned long lockoutDuration = LOCKOUT_DURATION_BASE;
    unsigned long lastAttemptTime = 0;
} g_loginTracker;

// Chống spam request đăng nhập
#define MIN_REQUEST_INTERVAL 1000  // 1 giây giữa các request
unsigned long g_lastLoginRequestTime = 0;

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

// ====== FORWARD DECLARATIONS ======
void sensorReadTask(void *pvParameters);
void logicControlTask(void *pvParameters);
void alertSendTask(void *pvParameters);
void configureWebServer();
void loadSettings();
void saveSettings();
void checkResetButton();
void updateStatusLEDs();
void addHistoryEvent(String message, bool isDanger);
bool isLockedOut();
void recordFailedLogin();
void recordSuccessLogin();
bool isAuthenticated(AsyncWebServerRequest *request);

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

// ====== HÀM QUẢN LÝ CẤU HÌNH ======

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

void loadSettings() {
  if (!SPIFFS.exists(CONFIG_FILE)) return;
  File file = SPIFFS.open(CONFIG_FILE, "r");
  if (!file) return;

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, file);
  file.close();

  if (doc["fire"].is<int>()) g_fireThreshold = doc["fire"];
  if (doc["co2"].is<int>())  g_co2Threshold = doc["co2"];
  if (doc["temp"].is<float>()) g_tempThreshold = doc["temp"];
  if (doc["email"].is<String>()) g_recipientEmail = doc["email"].as<String>();
  
  // Load Security Info
  if (doc.containsKey("adminPass")) g_adminPass = doc["adminPass"].as<String>();
  if (doc.containsKey("isDefault")) g_isDefaultPass = doc["isDefault"];
}

void saveSettings() {
  DynamicJsonDocument doc(1024);
  doc["fire"] = g_fireThreshold;
  doc["co2"] = g_co2Threshold;
  doc["temp"] = g_tempThreshold;
  doc["email"] = g_recipientEmail;
  doc["adminPass"] = g_adminPass;
  doc["isDefault"] = g_isDefaultPass;

  File file = SPIFFS.open(CONFIG_FILE, "w");
  serializeJson(doc, file);
  file.close();
}

// ====== HÀM BẢO MẬT ======

bool isLockedOut() {
    if (millis() < g_loginTracker.lockoutEndTime) {
        return true;  // Vẫn đang bị khóa
    }
    
    // Hết thời gian khóa → Reset
    if (g_loginTracker.lockoutEndTime > 0 && millis() >= g_loginTracker.lockoutEndTime) {
        g_loginTracker.failedCount = 0;
        g_loginTracker.lockoutEndTime = 0;
        Serial.println("[AUTH] Lockout expired. Reset counter.");
    }
    
    return false;
}

void recordFailedLogin() {
    g_loginTracker.failedCount++;
    g_loginTracker.lastAttemptTime = millis();
    
    Serial.printf("[AUTH] Failed login attempt #%d\n", g_loginTracker.failedCount);
    
    if (g_loginTracker.failedCount >= MAX_LOGIN_ATTEMPTS) {
        // Kích hoạt khóa
        g_loginTracker.lockoutEndTime = millis() + g_loginTracker.lockoutDuration;
        
        Serial.printf("[AUTH] ACCOUNT LOCKED for %lu seconds\n", 
                     g_loginTracker.lockoutDuration / 1000);
        
        // Tăng thời gian khóa cho lần sau (exponential backoff)
        g_loginTracker.lockoutDuration *= LOCKOUT_MULTIPLIER;
        
        // Log event (đã có hàm addHistoryEvent ở trên rồi)
        addHistoryEvent("CẢNH BÁO: Tài khoản bị khóa do đăng nhập sai quá nhiều", true);
    }
}

void recordSuccessLogin() {
    // Reset tất cả khi đăng nhập thành công
    g_loginTracker.failedCount = 0;
    g_loginTracker.lockoutEndTime = 0;
    g_loginTracker.lockoutDuration = LOCKOUT_DURATION_BASE;
    Serial.println("[AUTH] Login successful. Counter reset.");
}

bool isAuthenticated(AsyncWebServerRequest *request) {
    if (request->client()->remoteIP() == g_loggedInIP) {
        if (millis() - g_lastActivityTime < SESSION_TIMEOUT) {
            g_lastActivityTime = millis(); // Gia hạn session
            return true;
        } else {
            // Session hết hạn
            Serial.println("[AUTH] Session expired.");
            g_loggedInIP = IPAddress(0,0,0,0);
        }
    }
    return false;
}

// ====== CẤU HÌNH WEB SERVER ======

void configureWebServer() {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404);
        }
    });

    //API LOGIN
    AsyncCallbackJsonWebHandler* loginHandler = new AsyncCallbackJsonWebHandler("/login", 
  [](AsyncWebServerRequest *request, JsonVariant &json) {
    
    // Rate limiting
    if (millis() - g_lastLoginRequestTime < MIN_REQUEST_INTERVAL) {
        Serial.println("[AUTH] Rate limit exceeded");
        request->send(429, "application/json", 
                     "{\"error\": \"Too many requests. Please wait.\"}");
        return;
    }
    g_lastLoginRequestTime = millis();
    
    // Lockout check
    if (isLockedOut()) {
        long remainingSeconds = (g_loginTracker.lockoutEndTime - millis()) / 1000;
        
        String response = "{";
        response += "\"authenticated\": false,";
        response += "\"locked\": true,";
        response += "\"remaining\": " + String(remainingSeconds) + ",";
        response += "\"failedAttempts\": " + String(g_loginTracker.failedCount) + ",";  
        response += "\"message\": \"Account locked due to too many failed attempts\"";
        response += "}";
        
        Serial.printf("[AUTH] Login blocked. %ld seconds remaining.\n", remainingSeconds);
        request->send(403, "application/json", response);
        return;
    }
    
    // Xác thực
    JsonObject jsonObj = json.as<JsonObject>();
    String pass = jsonObj["password"].as<String>();
    
    if (pass == g_adminPass) {
        // THÀNH CÔNG
        g_loggedInIP = request->client()->remoteIP();
        g_lastActivityTime = millis();
        recordSuccessLogin();
        
        String response = "{";
        response += "\"authenticated\": true,";
        response += "\"isDefault\": " + String(g_isDefaultPass ? "true" : "false") + ",";
        response += "\"failedAttempts\": 0,";  //  Reset về 0
        response += "\"locked\": false";
        response += "}";
        
        Serial.printf("[AUTH] ✓ Login successful from IP: %s\n", 
                     g_loggedInIP.toString().c_str());
        
        addHistoryEvent("Đăng nhập thành công", false);
        request->send(200, "application/json", response);
        
    } else {
        // SAI MẬT KHẨU
        recordFailedLogin();
        
        bool nowLocked = isLockedOut();
        long remainingSeconds = nowLocked ? 
            (g_loginTracker.lockoutEndTime - millis()) / 1000 : 0;
        
        String response = "{";
        response += "\"authenticated\": false,";
        response += "\"locked\": " + String(nowLocked ? "true" : "false") + ",";
        response += "\"remaining\": " + String(remainingSeconds) + ",";
        response += "\"failedAttempts\": " + String(g_loginTracker.failedCount) + ",";  
        response += "\"message\": \"Invalid password\"";
        response += "}";
        
        Serial.printf("[AUTH] Invalid password. Attempts: %d/%d\n", 
                     g_loginTracker.failedCount, MAX_LOGIN_ATTEMPTS);
        
        request->send(401, "application/json", response);
    }
});
server.addHandler(loginHandler);

    
    //  API LOGOUT 
   
    server.on("/logout", HTTP_POST, [](AsyncWebServerRequest *request) {
        IPAddress clientIP = request->client()->remoteIP();
        
        // Chỉ cho phép logout nếu đang đăng nhập
        if (clientIP == g_loggedInIP) {
            // XÓA SESSION
            g_loggedInIP = IPAddress(0,0,0,0);
            g_lastActivityTime = 0;
            
            Serial.printf("[AUTH] Logout successful from IP: %s\n", 
                         clientIP.toString().c_str());
            
            addHistoryEvent("Đăng xuất", false);
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(401, "application/json", "{\"error\":\"Not logged in\"}");
        }
    });

   
    //  API AUTH CHECK
    server.on("/auth-check", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        
        // 1. Kiểm tra lockout
        bool locked = isLockedOut();
        long remainingSeconds = locked ? 
            (g_loginTracker.lockoutEndTime - millis()) / 1000 : 0;
        
        json += "\"locked\": " + String(locked ? "true" : "false") + ",";
        json += "\"remaining\": " + String(remainingSeconds) + ",";
        
        // 2. Kiểm tra session
        bool isAuth = false;
        IPAddress clientIP = request->client()->remoteIP();
        
        // Chỉ check session khi KHÔNG BỊ KHÓA
        if (!locked && clientIP == g_loggedInIP) {
            if (millis() - g_lastActivityTime < SESSION_TIMEOUT) {
                isAuth = true;
            } else {
                // Session hết hạn → Xóa
                Serial.println("[AUTH] Session expired in auth-check.");
                g_loggedInIP = IPAddress(0,0,0,0);
            }
        }
        
        json += "\"authenticated\": " + String(isAuth ? "true" : "false");
        json += "}";
        
        request->send(200, "application/json", json);
    });

   
    //  API DATA (Cần Auth)
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthenticated(request)) { 
            request->send(401, "application/json", "{\"error\": \"Unauthorized\"}"); 
            return; 
        }

        String json = "{";
        if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
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
            request->send(503, "application/json", "{\"error\": \"Service unavailable\"}");
        }
    });

  
    //  API SETTINGS (Cần Auth)

    AsyncCallbackJsonWebHandler* settingsHandler = new AsyncCallbackJsonWebHandler("/settings", 
    [](AsyncWebServerRequest *request, JsonVariant &json) {
        if (!isAuthenticated(request)) { 
            request->send(401, "application/json", "{\"error\": \"Unauthorized\"}"); 
            return; 
        }

        JsonObject jsonObj = json.as<JsonObject>();
        bool changed = false;
        bool passwordChanged = false;

        if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            if (jsonObj.containsKey("fireThreshold")) { 
                g_fireThreshold = jsonObj["fireThreshold"]; 
                changed = true; 
            }
            if (jsonObj.containsKey("co2Threshold")) { 
                g_co2Threshold = jsonObj["co2Threshold"]; 
                changed = true; 
            }
            if (jsonObj.containsKey("tempThreshold")) { 
                g_tempThreshold = jsonObj["tempThreshold"]; 
                changed = true; 
            }
            if (jsonObj.containsKey("recipientEmail")) { 
                g_recipientEmail = jsonObj["recipientEmail"].as<String>(); 
                changed = true; 
            }
            
            if (jsonObj.containsKey("password")) {
                String newPass = jsonObj["password"].as<String>();
                if (newPass.length() >= 4) {
                    g_adminPass = newPass;
                    g_isDefaultPass = false;
                    changed = true;
                    passwordChanged = true;
                    Serial.println("[AUTH] Password changed successfully.");
                }
            }

            xSemaphoreGive(xSensorDataMutex);
        } else {
            request->send(500, "application/json", "{\"error\": \"Internal error\"}");
            return;
        }

        if (changed) {
            saveSettings();
            if (passwordChanged) {
                addHistoryEvent("Đã thay đổi mật khẩu", false);
            }
        }

        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    server.addHandler(settingsHandler);

  
    //  API HISTORY (Cần Auth)
    server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthenticated(request)) { 
            request->send(401, "application/json", "{\"error\": \"Unauthorized\"}"); 
            return; 
        }
        
        String jsonResponse = "[";
        if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
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

// ====== SETUP ======

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
  
  xSensorDataMutex = xSemaphoreCreateMutex();
  xAlertQueue = xQueueCreate(5, sizeof(bool));
  configTime(7 * 3600, 0, "pool.ntp.org");

  configureWebServer();
  server.begin();

  xTaskCreatePinnedToCore(sensorReadTask, "SensorReadTask", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(logicControlTask, "LogicControlTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(alertSendTask, "AlertSendTask", 8192, NULL, 0, NULL, 0);
}


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
void performFactoryReset() {
  Serial.println("\n[SYSTEM] Đang thực hiện Factory Reset...");
  
  // 1. Nháy đèn báo hiệu cho người dùng biết
  for (int i = 0; i < 5; i++) {
    digitalWrite(PIN_LED_WIFI_OK, !digitalRead(PIN_LED_WIFI_OK));
    digitalWrite(PIN_LED_WIFI_ERR, !digitalRead(PIN_LED_WIFI_ERR));
    delay(200);
  }
  
  // 2. Xóa cấu hình WiFi cũ
  wm.resetSettings();
  Serial.println("- Đã xóa cấu hình WiFi.");

  // 3. Xóa file Config (Mật khẩu, Ngưỡng...)
  if (SPIFFS.exists(CONFIG_FILE)) {
    SPIFFS.remove(CONFIG_FILE);
    Serial.println("- Đã xóa file config.json (Reset mật khẩu).");
  } else {
    Serial.println("- Không tìm thấy file config, bỏ qua.");
  }

  Serial.println("[SYSTEM] Reset hoàn tất! Đang khởi động lại...");
  delay(1000);
  ESP.restart();
}

void checkResetButton() {
  // Biến static để lưu trạng thái giữa các lần gọi loop
  static unsigned long pressStartTime = 0;
  static bool isPressing = false;

  if (digitalRead(PIN_BUTTON_RESET) == HIGH) { 
    // Bắt đầu nhấn
    if (!isPressing) {
      isPressing = true;
      pressStartTime = millis();
      Serial.println("[BTN] Nút Reset đang được giữ...");
    }
    
    // Tính thời gian giữ
    unsigned long holdTime = millis() - pressStartTime;

    // Nếu giữ > 5 giây -> Kích hoạt Reset
    if (holdTime > 5000) {
      performFactoryReset();
      isPressing = false; 
    }
  } else {
    // Nhả nút
    if (isPressing) {
      isPressing = false;
      Serial.println("[BTN] Đã nhả nút (Hủy thao tác Reset).");
    }
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
