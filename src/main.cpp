/**
 * ESP32 - Датчик температуры на DS18B20 для стояков отопления
 * Отправляет данные температуры в SprutHub CE через MQTT
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

// Configuration
#include "config.h"

// Датчик температуры
#define ONE_WIRE_BUS 4

// Интервал опроса
#define SENSING_INTERVAL 10000
#define SEND_INTERVAL 15000

// Индикация качества Wi-Fi через встроенный LED (GPIO 2)
#define LED_BUILTIN_PIN 2
#define WIFI_LED_INTERVAL 3000  // Обновление индикации Wi-Fi каждые 3 сек

// ======================== ДИАГНОСТИКА (ПЕРЕДНЯЯ ОБЪЯВЛЕННАЯ) ========================

void diagnoseOneWire();

// ======================== ИНИЦИАЛИЗАЦИЯ ========================

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Адреса датчиков
DeviceAddress supplyAddr = {0x28, 0x74, 0x59, 0x15, 0x00, 0x00, 0x00, 0x85};
DeviceAddress returnAddr = {0x28, 0xEE, 0xF2, 0x14, 0x00, 0x00, 0x00, 0x37};

// ======================== ДИАГНОСТИКА ========================

void diagnoseOneWire()
{
  Serial.println("\n========== ДИАГНОСТИКА 1-WIRE ==========");
  
  // Проверка состояния линии DATA
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  delay(100);
  
  int state = digitalRead(ONE_WIRE_BUS);
  Serial.printf("Состояние линии DATA: %s\n", state ? "HIGH (открыта)" : "LOW (замкнута!)");
  
  if (state == LOW)
  {
    Serial.println("⚠️  ЛИНИЯ DATA ЗАМКНУТА НА ЗЕМЛЮ!");
    Serial.println("Проверьте:");
    Serial.println("  1. Нет ли короткого замыкания DATA-GND");
    Serial.println("  2. Правильно ли подключён резистор 4.7 кОм");
    Serial.println("  3. Не оборван ли провод");
  }
  else
  {
    Serial.println("✅ Линия DATA открыта (резистор подтяжки работает)");
  }
  
  // Пробуем найти устройства
  sensors.begin();
  
  Serial.println("\nПоиск датчиков...");
  int deviceCount = 0;
  
  for (int attempt = 0; attempt < 10; attempt++)
  {
    deviceCount = sensors.getDeviceCount();
    if (deviceCount > 0)
      break;
    Serial.printf("  Попытка %d/10: не найдено\n", attempt + 1);
    delay(1500);
  }
  
  if (deviceCount > 0)
  {
    Serial.printf("\n✅ НАЙДЕНО %d ДАТЧИКОВ!\n\n", deviceCount);
    sensors.setResolution(10);
    
    for (int i = 0; i < deviceCount; i++)
    {
      DeviceAddress addr;
      sensors.getAddress(addr, i);
      Serial.printf("Датчик %d: ", i);
      for (int j = 0; j < 8; j++)
      {
        if (addr[j] < 16)
          Serial.print("0");
        Serial.print(addr[j], HEX);
        if (j < 7)
          Serial.print(":");
      }
      float temp = sensors.getTempCByIndex(i);
      Serial.printf("  Температура: %.2f°C\n", temp);
    }
  }
  else
  {
    Serial.println("\n❌ ДАТЧИКИ НЕ НАЙДЕНЫ!");
    Serial.println("\nЧТО ПРОВЕРИТЬ:");
    Serial.println("  1. Резистор 4.7 кОм подключён к DATA и 3.3V");
    Serial.println("  2. Резистор должен быть НА СТОРОНЕ ESP32 (не на датчиках!)");
    Serial.println("  3. Проверьте подключение провода DATA к GPIO 4");
    Serial.println("  4. Проверьте подключение VCC и GND");
    Serial.println("  5. Для 30м провода попробуйте резистор 2.2 кОм");
    Serial.println("  6. Проверьте что датчики не сгорели");
  }
  
  Serial.println("\n========================================\n");
}

unsigned long previousSensingTime = 0;
unsigned long previousSendTime = 0;
float tempSupply = 0.0;
float tempReturn = 0.0;
bool wifiConnected = false;
int currentNetwork = 0;  // Текущая сеть (0, 1 или 2)

// Массив WiFi сетей для перебора
struct WiFiNetwork
{
  const char *ssid;
  const char *password;
};

WiFiNetwork networks[3] = {
    {WIFI_SSID_1, WIFI_PASSWORD_1},
    {WIFI_SSID_2, WIFI_PASSWORD_2},
    {WIFI_SSID_3, WIFI_PASSWORD_3}};

// Таймер индикации Wi-Fi
unsigned long wifiLedLastUpdate = 0;

// ======================== FORWARD DECLARATIONS ========================

void readTemperature();
void sendToSprutHub();
bool connectMQTT();
void updateWifiLed(int rssi);
void initOTA();

// ======================== ФУНКЦИИ ========================

void setup()
{
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("========================================");
  Serial.println("  ESP32 DS18B20 - Ст01");
  Serial.println("========================================");

  // Запускаем диагностику 1-Wire
  diagnoseOneWire();

  Serial.println();

  // ==================== ПОДКЛЮЧЕНИЕ К WI-FI (ТРИ СЕТИ) ====================
  Serial.println("Подключение к Wi-Fi...");
  Serial.printf("  1: %s\n", WIFI_SSID_1);
  Serial.printf("  2: %s\n", WIFI_SSID_2);
  Serial.printf("  3: %s\n", WIFI_SSID_3);

  int currentNetwork = 0;
  WiFi.mode(WIFI_STA);
  bool connected = false;

  // Пробуем каждую сеть по очереди
  for (int i = 0; i < 3; i++)
  {
    Serial.printf("\n  Попытка %d/3: %s...\n", i + 1, networks[i].ssid);
    WiFi.disconnect();
    delay(500);
    WiFi.begin(networks[i].ssid, networks[i].password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 5000))
    {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      connected = true;
      currentNetwork = i;
      break;
    }
    else
    {
      Serial.println("\n  Не удалось подключиться");
    }
  }

  if (connected)
  {
    wifiConnected = true;
    Serial.println("\n✅ WiFi подключено!");
    Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("  Сеть: %s\n", networks[currentNetwork].ssid);

    if (currentNetwork == 0)
      Serial.println("  ✅ ОСНОВНАЯ сеть");
    else if (currentNetwork == 1)
      Serial.println("  ⚠️  РЕЗЕРВНАЯ сеть #1");
    else
      Serial.println("  ⚠️  РЕЗЕРВНАЯ сеть #2");
  }
  else
  {
    Serial.println("\n❌ ERROR: Не удалось подключиться ни к одной сети!");
  }

  // Инициализация встроенного LED для индикации
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, LOW);
  Serial.println("LED indicator initialized (GPIO 2)");

  // Инициализация OTA (ТОЛЬКО после подключения к Wi-Fi!)
  if (wifiConnected)
  {
    initOTA();
  }
  else
  {
    Serial.println("OTA skipped - WiFi not connected");
  }
}

void loop()
{
  unsigned long currentTime = millis();

  // ==================== ПЕРЕПОДКЛЮЧЕНИЕ К WI-FI (3 СЕТИ) ====================
  if (!WiFi.isConnected())
  {
    Serial.println("\n⚠️  Wi-Fi отключён! Переподключение...");
    wifiConnected = false;

    // Пробуем следующую сеть в цикле
    currentNetwork = (currentNetwork + 1) % 3;
    Serial.printf("  Пробуем сеть %d/3: %s\n", currentNetwork + 1, networks[currentNetwork].ssid);

    WiFi.disconnect();
    delay(1000);
    WiFi.begin(networks[currentNetwork].ssid, networks[currentNetwork].password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < 8000))
    {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      wifiConnected = true;
      Serial.println("\n  ✅ Переподключено!");
      Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
      Serial.printf("  Сеть: %s\n", networks[currentNetwork].ssid);
    }
    else
    {
      Serial.println("\n  ❌ Не удалось переподключиться!");
    }
  }

  // Чтение температуры
  if (currentTime - previousSensingTime >= SENSING_INTERVAL)
  {
    previousSensingTime = currentTime;
    readTemperature();
  }

  // Отправка данных
  if (currentTime - previousSendTime >= SEND_INTERVAL)
  {
    previousSendTime = currentTime;
    sendToSprutHub();
  }

  // Поддерживаем MQTT соединение
  if (!mqttClient.connected())
  {
    connectMQTT();
  }
  mqttClient.loop();

  // Индикация Wi-Fi
  if (wifiConnected)
  {
    int wifiRSSI = WiFi.RSSI();
    updateWifiLed(wifiRSSI);
  }
  else
  {
    digitalWrite(LED_BUILTIN_PIN, LOW);
  }

  // Обработка OTA обновлений
  ArduinoOTA.handle();

  delay(100);
}

void readTemperature()
{
  sensors.requestTemperatures();

  int deviceCount = sensors.getDeviceCount();

  if (deviceCount == 0)
  {
    Serial.println("ERROR: No DS18B20 sensors found!");
    return;
  }

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors.getTempCByIndex(i);
    DeviceAddress addr;
    sensors.getAddress(addr, i);

    // Проверяем, это датчик Ст01 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != supplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст01 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != returnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст01 Подача: ERROR");
      }
      else
      {
        tempSupply = temp;
        Serial.printf("🔥 Ст01 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст01 Обратка: ERROR");
      }
      else
      {
        tempReturn = temp;
        Serial.printf("❄️  Ст01 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
    Serial.println("⚠️  Ст01 Подача NOT FOUND!");
  if (!returnFound)
    Serial.println("⚠️  Ст01 Обратка NOT FOUND!");
}

void sendToSprutHub()
{
  if (!wifiConnected)
    return;

  // Не отправляем, если температура не прочитана
  if (tempSupply == 0.0 && tempReturn == 0.0)
    return;

  char supplyStr[10];
  char returnStr[10];
  sprintf(supplyStr, "%.2f", tempSupply);
  sprintf(returnStr, "%.2f", tempReturn);

  mqttClient.publish("SprutHub/St01-P/DS18B20/temperature", supplyStr, true);
  mqttClient.publish("SprutHub/St01-O/DS18B20/temperature", returnStr, true);

  Serial.printf("MQTT -> St01-P: %s  |  St01-O: %s\n", supplyStr, returnStr);
}

bool connectMQTT()
{
  if (mqttClient.connected())
    return true;

  Serial.printf("Connecting to MQTT: %s:%d\n", MQTT_SERVER, MQTT_PORT);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  String clientId = "ESP32-St01-";
  clientId += String(random(0xffff), HEX);

  bool connected;
  connected = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);

  if (connected)
  {
    Serial.println("MQTT connected!");
    return true;
  }
  else
  {
    Serial.printf("MQTT failed (rc=%d)\n", mqttClient.state());
    return false;
  }
}

// ======================== ИНДИКАЦИЯ ========================

// Индикация качества Wi-Fi через встроенный LED
void updateWifiLed(int rssi)
{
  unsigned long now = millis();
  if (now - wifiLedLastUpdate < WIFI_LED_INTERVAL)
    return;
  wifiLedLastUpdate = now;

  int blinkDuration = 0;
  int pauseDuration = 0;

  if (rssi > -50)
  {
    // Отлично (-30...-50) — горит постоянно
    digitalWrite(LED_BUILTIN_PIN, HIGH);
    return;
  }
  else if (rssi > -60)
  {
    // Хорошо (-50...-60) — мигает 1 раз в 2 сек
    blinkDuration = 500;
    pauseDuration = 1500;
  }
  else if (rssi > -70)
  {
    // Средне (-60...-70) — мигает 1 раз в 3 сек
    blinkDuration = 300;
    pauseDuration = 1700;
  }
  else if (rssi > -80)
  {
    // Слабо (-70...-80) — мигает 1 раз в 5 сек
    blinkDuration = 200;
    pauseDuration = 2800;
  }
  else
  {
    // Очень слабо (<-80) — не мигает
    digitalWrite(LED_BUILTIN_PIN, LOW);
    return;
  }

  digitalWrite(LED_BUILTIN_PIN, HIGH);
  delay(blinkDuration);
  digitalWrite(LED_BUILTIN_PIN, LOW);
  delay(pauseDuration);
}

// ======================== OTA (Over-The-Air) ========================

void initOTA()
{
  // Настройка OTA
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  // События OTA
  ArduinoOTA.onStart([]()
                     {
                       Serial.println("\nOTA: Начало обновления");
                       Serial.println("Остановить MQTT и датчики...");
                       
                       // Мигание LED во время обновления
                       pinMode(LED_BUILTIN_PIN, OUTPUT);
                       for (int i = 0; i < 10; i++)
                       {
                         digitalWrite(LED_BUILTIN_PIN, HIGH);
                         delay(100);
                         digitalWrite(LED_BUILTIN_PIN, LOW);
                         delay(100);
                       }
                     })
      .onEnd([]()
             {
              Serial.println("\nOTA: Обновление завершено!");
              Serial.println("Перезагрузка через 1 сек...");
              delay(1000);
             })
      .onProgress([](unsigned int progress, unsigned int total)
                  {
                    Serial.printf("OTA: %u%%\r", (progress / (total / 100)));
                  })
      .onError([](ota_error_t error)
               {
                Serial.printf("OTA: Ошибка [%d]\n", error);
                if (error == OTA_AUTH_ERROR)
                  Serial.println("  Ошибка аутентификации");
                else if (error == OTA_BEGIN_ERROR)
                  Serial.println("  Ошибка начала обновления");
                else if (error == OTA_CONNECT_ERROR)
                  Serial.println("  Ошибка подключения");
                else if (error == OTA_RECEIVE_ERROR)
                  Serial.println("  Ошибка приёма данных");
                else if (error == OTA_END_ERROR)
                  Serial.println("  Ошибка завершения");
               });

  // Запуск mDNS
  if (MDNS.begin(OTA_HOSTNAME))
  {
    Serial.println("mDNS started: " + String(OTA_HOSTNAME));
  }
  else
  {
    Serial.println("mDNS failed!");
  }

  // Запуск OTA
  ArduinoOTA.begin();

  Serial.println("OTA готова. Обновляйте по Wi-Fi:");
  Serial.printf("  Имя: %s\n", OTA_HOSTNAME);
  Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("  Порт: 3232\n");
  Serial.printf("  Пароль: %s\n", OTA_PASSWORD);
}
