/**
 * ESP32 - Датчик температуры на DS18B20 для стояков
 * Отправляет данные температуры в SprutHub CE через MQTT
 */

#include <Arduino.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

// Configuration
#include "config.h"

// Датчик температуры
#define ONE_WIRE_BUS 4

// Интервал опроса
#define SENSING_INTERVAL 5000
#define SEND_INTERVAL 10000

// ======================== ИНИЦИАЛИЗАЦИЯ ========================

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Адреса датчиков
DeviceAddress supplyAddr = {0x28, 0x74, 0x59, 0x15, 0x00, 0x00, 0x00, 0x85};
DeviceAddress returnAddr = {0x28, 0xEE, 0xF2, 0x14, 0x00, 0x00, 0x00, 0x37};

unsigned long previousSensingTime = 0;
unsigned long previousSendTime = 0;
float tempSupply = 999.0;
float tempReturn = 999.0;
bool wifiConnected = false;

// ======================== FORWARD DECLARATIONS ========================

void readTemperature();
void sendToSprutHub();
bool connectMQTT();

// ======================== ФУНКЦИИ ========================

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  ESP32 DS18B20 - Ст01");
  Serial.println("========================================");
  
  sensors.begin();
  sensors.setResolution(12);
  
  int deviceCount = sensors.getDeviceCount();
  Serial.printf("Found %d DS18B20 devices\n\n", deviceCount);
  
  for (int i = 0; i < deviceCount; i++) {
    DeviceAddress addr;
    sensors.getAddress(addr, i);
    Serial.printf("Sensor %d: ", i);
    for (int j = 0; j < 8; j++) {
      if (addr[j] < 16) Serial.print("0");
      Serial.print(addr[j], HEX);
      if (j < 7) Serial.print(":");
    }
    float temp = sensors.getTempCByIndex(i);
    Serial.printf("  Temp: %.2f°C\n", temp);
  }
  
  Serial.println();
  
  // Подключение к Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected!");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println("\nERROR: WiFi connection failed!");
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  if (!WiFi.isConnected()) {
    WiFi.reconnect();
  }
  
  // Чтение температуры
  if (currentTime - previousSensingTime >= SENSING_INTERVAL) {
    previousSensingTime = currentTime;
    readTemperature();
  }
  
  // Отправка данных
  if (currentTime - previousSendTime >= SEND_INTERVAL) {
    previousSendTime = currentTime;
    sendToSprutHub();
  }
  
  // Поддерживаем MQTT соединение
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
  
  delay(100);
}

void readTemperature() {
  sensors.requestTemperatures();
  
  int deviceCount = sensors.getDeviceCount();
  
  if (deviceCount == 0) {
    Serial.println("ERROR: No DS18B20 sensors found!");
    return;
  }
  
  bool supplyFound = false;
  bool returnFound = false;
  
  for (int i = 0; i < deviceCount; i++) {
    float temp = sensors.getTempCByIndex(i);
    DeviceAddress addr;
    sensors.getAddress(addr, i);
    
    // Проверяем, это датчик Ст01 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++) {
      if (addr[j] != supplyAddr[j]) { isSupply = false; break; }
    }
    
    // Проверяем, это датчик Ст01 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++) {
      if (addr[j] != returnAddr[j]) { isReturn = false; break; }
    }
    
    if (isSupply) {
      if (temp == DEVICE_DISCONNECTED_C) {
        Serial.println("🔥 Ст01 Подача: ERROR");
      } else {
        tempSupply = temp;
        Serial.printf("🔥 Ст01 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn) {
      if (temp == DEVICE_DISCONNECTED_C) {
        Serial.println("❄️  Ст01 Обратка: ERROR");
      } else {
        tempReturn = temp;
        Serial.printf("❄️  Ст01 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }
  
  if (!supplyFound) Serial.println("⚠️  Ст01 Подача NOT FOUND!");
  if (!returnFound) Serial.println("⚠️  Ст01 Обратка NOT FOUND!");
}

void sendToSprutHub() {
  if (!wifiConnected) return;
  
  char supplyStr[10];
  char returnStr[10];
  sprintf(supplyStr, "%.2f", tempSupply);
  sprintf(returnStr, "%.2f", tempReturn);
  
  mqttClient.publish("SprutHub/St01-P/DS18B20/temperature", supplyStr, true);
  mqttClient.publish("SprutHub/St01-O/DS18B20/temperature", returnStr, true);
  
  Serial.printf("MQTT -> St01-P: %s  |  St01-O: %s\n", supplyStr, returnStr);
}

bool connectMQTT() {
  if (mqttClient.connected()) return true;
  
  Serial.printf("Connecting to MQTT: %s:%d\n", MQTT_SERVER, MQTT_PORT);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  
  String clientId = "ESP32-St01-";
  clientId += String(random(0xffff), HEX);
  
  bool connected;
  connected = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
  
  if (connected) {
    Serial.println("MQTT connected!");
    return true;
  } else {
    Serial.printf("MQTT failed (rc=%d)\n", mqttClient.state());
    return false;
  }
}
