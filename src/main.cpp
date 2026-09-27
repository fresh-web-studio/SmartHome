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

// Датчики температуры
#define ONE_WIRE_BUS_1 4
#define ONE_WIRE_BUS_2 16  // D16 для Ст08

OneWire oneWire1(ONE_WIRE_BUS_1);
OneWire oneWire2(ONE_WIRE_BUS_2);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);

// Интервал опроса
#define SENSING_INTERVAL 10000
#define SEND_INTERVAL 15000

// Индикация качества Wi-Fi через встроенный LED (GPIO 2)
#define LED_BUILTIN_PIN 2
#define WIFI_LED_INTERVAL 3000  // Обновление индикации Wi-Fi каждые 3 сек

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Адреса датчиков St01
DeviceAddress st01SupplyAddr = ST01_SUPPLY_ADDR;
DeviceAddress st01ReturnAddr = ST01_RETURN_ADDR;

// Адреса датчиков St02 (с реальными адресами!)
DeviceAddress st02SupplyAddr = ST02_SUPPLY_ADDR;
DeviceAddress st02ReturnAddr = ST02_RETURN_ADDR;

// Адреса датчиков St03
DeviceAddress st03SupplyAddr = ST03_SUPPLY_ADDR;
DeviceAddress st03ReturnAddr = ST03_RETURN_ADDR;

// Адреса датчиков St04
DeviceAddress st04SupplyAddr = ST04_SUPPLY_ADDR;
DeviceAddress st04ReturnAddr = ST04_RETURN_ADDR;

// Адреса датчиков St05
DeviceAddress st05SupplyAddr = ST05_SUPPLY_ADDR;
DeviceAddress st05ReturnAddr = ST05_RETURN_ADDR;

// Адреса датчиков St08 (GPIO16)
DeviceAddress st08SupplyAddr = ST08_SUPPLY_ADDR;
DeviceAddress st08ReturnAddr = ST08_RETURN_ADDR;

// Адреса датчиков St07 (GPIO16)
DeviceAddress st07SupplyAddr = ST07_SUPPLY_ADDR;
DeviceAddress st07ReturnAddr = ST07_RETURN_ADDR;

// Адреса датчиков St06 (GPIO16)
DeviceAddress st06SupplyAddr = ST06_SUPPLY_ADDR;
DeviceAddress st06ReturnAddr = ST06_RETURN_ADDR;

// ======================== ДИАГНОСТИКА ========================

void scanAllSensors()
{
  Serial.println("\n========== СКАНИРОВАНИЕ ВСЕХ ДАТЧИКОВ ==========");
  Serial.println("Ищем все датчики DS18B20 на шине 1 (GPIO4)...\n");
  
  sensors1.begin();
  sensors1.setResolution(12);
  
  int deviceCount1 = sensors1.getDeviceCount();
  
  Serial.printf("Шина 1 (GPIO4): %d датчиков\n\n", deviceCount1);
  
  for (int i = 0; i < deviceCount1; i++)
  {
    DeviceAddress addr;
    sensors1.getAddress(addr, i);
    
    Serial.printf("Датчик %d: ", i);
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] < 16)
        Serial.print("0");
      Serial.print(addr[j], HEX);
      if (j < 7)
        Serial.print(":");
    }
    
    float temp = sensors1.getTempCByIndex(i);
    if (temp == DEVICE_DISCONNECTED_C)
    {
      Serial.println("  [НЕ ОТВЕЧАЕТ]");
    }
    else
    {
      Serial.printf("  Температура: %.2f°C", temp);
    }
  }
  
  Serial.println("\n----------------------------------------\n");
  Serial.println("Ищем все датчики DS18B20 на шине 2 (GPIO16)...\n");
  
  sensors2.begin();
  sensors2.setResolution(12);
  
  int deviceCount2 = sensors2.getDeviceCount();
  
  Serial.printf("Шина 2 (GPIO16): %d датчиков\n\n", deviceCount2);
  
  for (int i = 0; i < deviceCount2; i++)
  {
    DeviceAddress addr;
    sensors2.getAddress(addr, i);
    
    Serial.printf("Датчик %d: ", i);
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] < 16)
        Serial.print("0");
      Serial.print(addr[j], HEX);
      if (j < 7)
        Serial.print(":");
    }
    
    float temp = sensors2.getTempCByIndex(i);
    if (temp == DEVICE_DISCONNECTED_C)
    {
      Serial.println("  [НЕ ОТВЕЧАЕТ]");
    }
    else
    {
      Serial.printf("  Температура: %.2f°C", temp);
    }
  }
  
  Serial.println("\n========================================");
  Serial.println("\n📋 СКОПИРУЙТЕ АДРЕСА И ВСТАВЬТЕ В config.h:");
  Serial.println("\n// Адреса датчиков St08:");
  Serial.println("#define ST08_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST08_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n========================================\n");
}

void diagnoseOneWire()
{
  Serial.println("\n========== ДИАГНОСТИКА 1-WIRE ==========");
  
  // Проверка состояния линии DATA1 (GPIO4)
  pinMode(ONE_WIRE_BUS_1, INPUT_PULLUP);
  delay(100);
  
  int state1 = digitalRead(ONE_WIRE_BUS_1);
  Serial.printf("Шина 1 (GPIO4): %s\n", state1 ? "HIGH (открыта)" : "LOW (замкнута!)");
  
  if (state1 == LOW)
  {
    Serial.println("⚠️  ЛИНИЯ DATA1 ЗАМКНУТА НА ЗЕМЛЮ!");
  }
  else
  {
    Serial.println("✅ Шина 1 открыта (резистор подтяжки работает)");
  }
  
  // Проверка состояния линии DATA2 (GPIO16)
  pinMode(ONE_WIRE_BUS_2, INPUT_PULLUP);
  delay(100);
  
  int state2 = digitalRead(ONE_WIRE_BUS_2);
  Serial.printf("Шина 2 (GPIO16): %s\n", state2 ? "HIGH (открыта)" : "LOW (замкнута!)");
  
  if (state2 == LOW)
  {
    Serial.println("⚠️  ЛИНИЯ DATA2 ЗАМКНУТА НА ЗЕМЛЮ!");
  }
  else
  {
    Serial.println("✅ Шина 2 открыта (резистор подтяжки работает)");
  }
  
  // Пробуем найти устройства
  sensors1.begin();
  sensors2.begin();
  
  Serial.println("\nПоиск датчиков...");
  int deviceCount1 = sensors1.getDeviceCount();
  int deviceCount2 = sensors2.getDeviceCount();
  
  if (deviceCount1 > 0)
  {
    Serial.printf("\n✅ ШИНА 1: НАЙДЕНО %d ДАТЧИКОВ!\n\n", deviceCount1);
    sensors1.setResolution(10);
    
    for (int i = 0; i < deviceCount1; i++)
    {
      DeviceAddress addr;
      sensors1.getAddress(addr, i);
      Serial.printf("Датчик %d: ", i);
      for (int j = 0; j < 8; j++)
      {
        if (addr[j] < 16)
          Serial.print("0");
        Serial.print(addr[j], HEX);
        if (j < 7)
          Serial.print(":");
      }
      float temp = sensors1.getTempCByIndex(i);
      Serial.printf("  Температура: %.2f°C\n", temp);
    }
  }
  else
  {
    Serial.println("\n❌ ШИНА 1: ДАТЧИКИ НЕ НАЙДЕНЫ!");
  }
  
  if (deviceCount2 > 0)
  {
    Serial.printf("\n✅ ШИНА 2: НАЙДЕНО %d ДАТЧИКОВ!\n\n", deviceCount2);
    sensors2.setResolution(10);
    
    for (int i = 0; i < deviceCount2; i++)
    {
      DeviceAddress addr;
      sensors2.getAddress(addr, i);
      Serial.printf("Датчик %d: ", i);
      for (int j = 0; j < 8; j++)
      {
        if (addr[j] < 16)
          Serial.print("0");
        Serial.print(addr[j], HEX);
        if (j < 7)
          Serial.print(":");
      }
      float temp = sensors2.getTempCByIndex(i);
      Serial.printf("  Температура: %.2f°C\n", temp);
    }
  }
  else
  {
    Serial.println("\n❌ ШИНА 2: ДАТЧИКИ НЕ НАЙДЕНЫ!");
  }
  
  Serial.println("\n========================================\n");
}

unsigned long previousSensingTime = 0;
unsigned long previousSendTime = 0;
float tempSupply = 0.0;
float tempReturn = 0.0;
float tempSupply2 = 0.0;  // St02 Подача
float tempReturn2 = 0.0;  // St02 Обратка
float tempSupply3 = 0.0;  // St03 Подача
float tempReturn3 = 0.0;  // St03 Обратка
float tempSupply4 = 0.0;  // St04 Подача
float tempReturn4 = 0.0;  // St04 Обратка
float tempSupply5 = 0.0;  // St05 Подача
float tempReturn5 = 0.0;  // St05 Обратка
float tempSupply8 = 0.0;  // St08 Подача
float tempReturn8 = 0.0;  // St08 Обратка
float tempSupply7 = 0.0;  // St07 Подача
float tempReturn7 = 0.0;  // St07 Обратка
float tempSupply6 = 0.0;  // St06 Подача
float tempReturn6 = 0.0;  // St06 Обратка
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
void readTemperatureSt02();
void readTemperatureSt03();
void readTemperatureSt04();
void readTemperatureSt05();
void readTemperatureSt08();
void readTemperatureSt07();
void readTemperatureSt06();
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

  // Запускаем сканирование всех датчиков
  scanAllSensors();

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
    sensors1.requestTemperatures();
    delay(500);  // Задержка между запросами к разным шинам
    sensors2.requestTemperatures();
    readTemperature();
    readTemperatureSt02();
    readTemperatureSt03();
    readTemperatureSt04();
    readTemperatureSt05();
    readTemperatureSt08();
    readTemperatureSt07();
    readTemperatureSt06();
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

  // Запускаем сканирование один раз при загрузке
  // (уже запущено в setup())

  delay(100);
}

void readTemperature()
{
  int deviceCount = sensors1.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors1.getTempCByIndex(i);
    DeviceAddress addr;
    sensors1.getAddress(addr, i);

    // Проверяем, это датчик Ст01 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st01SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст01 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st01ReturnAddr[j])
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
  {
    Serial.println("⚠️  Ст01 Подача NOT FOUND!");
    tempSupply = 0.0;  // Сбрасываем если не найден
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст01 Обратка NOT FOUND!");
    tempReturn = 0.0;  // Сбрасываем если не найден
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply == tempReturn)
  {
    Serial.println("⚠️  Ст01: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply = 0.0;
    tempReturn = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St02 ========================

void readTemperatureSt02()
{

  int deviceCount = sensors1.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors1.getTempCByIndex(i);
    DeviceAddress addr;
    sensors1.getAddress(addr, i);

    // Проверяем, это датчик Ст02 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st02SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст02 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st02ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст02 Подача: ERROR");
      }
      else
      {
        tempSupply2 = temp;
        Serial.printf("🔥 Ст02 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст02 Обратка: ERROR");
      }
      else
      {
        tempReturn2 = temp;
        Serial.printf("❄️  Ст02 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст02 Подача NOT FOUND!");
    tempSupply2 = 0.0;  // Сбрасываем если не найден
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст02 Обратка NOT FOUND!");
    tempReturn2 = 0.0;  // Сбрасываем если не найден
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply2 == tempReturn2)
  {
    Serial.println("⚠️  Ст02: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply2 = 0.0;
    tempReturn2 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St03 ========================

void readTemperatureSt03()
{
  

  int deviceCount = sensors1.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors1.getTempCByIndex(i);
    DeviceAddress addr;
    sensors1.getAddress(addr, i);

    // Проверяем, это датчик Ст03 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st03SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст03 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st03ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст03 Подача: ERROR");
      }
      else
      {
        tempSupply3 = temp;
        Serial.printf("🔥 Ст03 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст03 Обратка: ERROR");
      }
      else
      {
        tempReturn3 = temp;
        Serial.printf("❄️  Ст03 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст03 Подача NOT FOUND!");
    tempSupply3 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст03 Обратка NOT FOUND!");
    tempReturn3 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply3 == tempReturn3)
  {
    Serial.println("⚠️  Ст03: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply3 = 0.0;
    tempReturn3 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St04 ========================

void readTemperatureSt04()
{
  

  int deviceCount = sensors1.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors1.getTempCByIndex(i);
    DeviceAddress addr;
    sensors1.getAddress(addr, i);

    // Проверяем, это датчик Ст04 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st04SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст04 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st04ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст04 Подача: ERROR");
      }
      else
      {
        tempSupply4 = temp;
        Serial.printf("🔥 Ст04 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст04 Обратка: ERROR");
      }
      else
      {
        tempReturn4 = temp;
        Serial.printf("❄️  Ст04 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст04 Подача NOT FOUND!");
    tempSupply4 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст04 Обратка NOT FOUND!");
    tempReturn4 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply4 == tempReturn4)
  {
    Serial.println("⚠️  Ст04: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply4 = 0.0;
    tempReturn4 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St05 ========================

void readTemperatureSt05()
{
  

  int deviceCount = sensors1.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors1.getTempCByIndex(i);
    DeviceAddress addr;
    sensors1.getAddress(addr, i);

    // Проверяем, это датчик Ст05 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st05SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст05 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st05ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст05 Подача: ERROR");
      }
      else
      {
        tempSupply5 = temp;
        Serial.printf("🔥 Ст05 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст05 Обратка: ERROR");
      }
      else
      {
        tempReturn5 = temp;
        Serial.printf("❄️  Ст05 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст05 Подача NOT FOUND! (адрес: 28:E6:31:15:00:00:00:75)");
    tempSupply5 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст05 Обратка NOT FOUND! (адрес: 28:9F:21:15:00:00:00:07)");
    tempReturn5 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply5 == tempReturn5)
  {
    Serial.println("⚠️  Ст05: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply5 = 0.0;
    tempReturn5 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St08 (GPIO16) ========================

void readTemperatureSt08()
{
  

  int deviceCount = sensors2.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors2.getTempCByIndex(i);
    DeviceAddress addr;
    sensors2.getAddress(addr, i);

    // Проверяем, это датчик Ст08 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st08SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст08 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st08ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст08 Подача: ERROR");
      }
      else
      {
        tempSupply8 = temp;
        Serial.printf("🔥 Ст08 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст08 Обратка: ERROR");
      }
      else
      {
        tempReturn8 = temp;
        Serial.printf("❄️  Ст08 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст08 Подача NOT FOUND!");
    tempSupply8 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст08 Обратка NOT FOUND!");
    tempReturn8 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply8 == tempReturn8)
  {
    Serial.println("⚠️  Ст08: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply8 = 0.0;
    tempReturn8 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St07 (GPIO16) ========================

void readTemperatureSt07()
{
  

  int deviceCount = sensors2.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors2.getTempCByIndex(i);
    DeviceAddress addr;
    sensors2.getAddress(addr, i);

    // Проверяем, это датчик Ст07 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st07SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст07 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st07ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст07 Подача: ERROR");
      }
      else
      {
        tempSupply7 = temp;
        Serial.printf("🔥 Ст07 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст07 Обратка: ERROR");
      }
      else
      {
        tempReturn7 = temp;
        Serial.printf("❄️  Ст07 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст07 Подача NOT FOUND!");
    tempSupply7 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст07 Обратка NOT FOUND!");
    tempReturn7 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply7 == tempReturn7)
  {
    Serial.println("⚠️  Ст07: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply7 = 0.0;
    tempReturn7 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St06 (GPIO16) ========================

void readTemperatureSt06()
{
  

  int deviceCount = sensors2.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors2.getTempCByIndex(i);
    DeviceAddress addr;
    sensors2.getAddress(addr, i);

    // Проверяем, это датчик Ст06 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st06SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст06 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st06ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст06 Подача: ERROR");
      }
      else
      {
        tempSupply6 = temp;
        Serial.printf("🔥 Ст06 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст06 Обратка: ERROR");
      }
      else
      {
        tempReturn6 = temp;
        Serial.printf("❄️  Ст06 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст06 Подача NOT FOUND!");
    tempSupply6 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст06 Обратка NOT FOUND!");
    tempReturn6 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply6 == tempReturn6)
  {
    Serial.println("⚠️  Ст06: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply6 = 0.0;
    tempReturn6 = 0.0;
  }
}

void sendToSprutHub()
{
  if (!wifiConnected)
    return;

  char supplyStr[10];
  char returnStr[10];
  char supplyStr2[10];
  char returnStr2[10];
  char supplyStr3[10];
  char returnStr3[10];
  char supplyStr4[10];
  char returnStr4[10];
  char supplyStr5[10];
  char returnStr5[10];
  char supplyStr8[10];
  char returnStr8[10];
  char supplyStr7[10];
  char returnStr7[10];
  char supplyStr6[10];
  char returnStr6[10];

  // St01
  if (tempSupply > 0.0 && tempReturn > 0.0)
  {
    sprintf(supplyStr, "%.2f", tempSupply);
    sprintf(returnStr, "%.2f", tempReturn);
    mqttClient.publish("SprutHub/St01-P/DS18B20/temperature", supplyStr, true);
    mqttClient.publish("SprutHub/St01-O/DS18B20/temperature", returnStr, true);
    Serial.printf("MQTT -> St01-P: %s  |  St01-O: %s\n", supplyStr, returnStr);
  }

  // St02
  if (tempSupply2 > 0.0 && tempReturn2 > 0.0)
  {
    sprintf(supplyStr2, "%.2f", tempSupply2);
    sprintf(returnStr2, "%.2f", tempReturn2);
    mqttClient.publish("SprutHub/St02-P/DS18B20/temperature", supplyStr2, true);
    mqttClient.publish("SprutHub/St02-O/DS18B20/temperature", returnStr2, true);
    Serial.printf("MQTT -> St02-P: %s  |  St02-O: %s\n", supplyStr2, returnStr2);
  }

  // St03
  if (tempSupply3 > 0.0 && tempReturn3 > 0.0)
  {
    sprintf(supplyStr3, "%.2f", tempSupply3);
    sprintf(returnStr3, "%.2f", tempReturn3);
    mqttClient.publish("SprutHub/St03-P/DS18B20/temperature", supplyStr3, true);
    mqttClient.publish("SprutHub/St03-O/DS18B20/temperature", returnStr3, true);
    Serial.printf("MQTT -> St03-P: %s  |  St03-O: %s\n", supplyStr3, returnStr3);
  }

  // St04
  if (tempSupply4 > 0.0 && tempReturn4 > 0.0)
  {
    sprintf(supplyStr4, "%.2f", tempSupply4);
    sprintf(returnStr4, "%.2f", tempReturn4);
    mqttClient.publish("SprutHub/St04-P/DS18B20/temperature", supplyStr4, true);
    mqttClient.publish("SprutHub/St04-O/DS18B20/temperature", returnStr4, true);
    Serial.printf("MQTT -> St04-P: %s  |  St04-O: %s\n", supplyStr4, returnStr4);
  }

  // St05
  if (tempSupply5 > 0.0 && tempReturn5 > 0.0)
  {
    sprintf(supplyStr5, "%.2f", tempSupply5);
    sprintf(returnStr5, "%.2f", tempReturn5);
    mqttClient.publish("SprutHub/St05-P/DS18B20/temperature", supplyStr5, true);
    mqttClient.publish("SprutHub/St05-O/DS18B20/temperature", returnStr5, true);
    Serial.printf("MQTT -> St05-P: %s  |  St05-O: %s\n", supplyStr5, returnStr5);
  }

  // St08
  if (tempSupply8 > 0.0 && tempReturn8 > 0.0)
  {
    sprintf(supplyStr8, "%.2f", tempSupply8);
    sprintf(returnStr8, "%.2f", tempReturn8);
    mqttClient.publish("SprutHub/St08-P/DS18B20/temperature", supplyStr8, true);
    mqttClient.publish("SprutHub/St08-O/DS18B20/temperature", returnStr8, true);
    Serial.printf("MQTT -> St08-P: %s  |  St08-O: %s\n", supplyStr8, returnStr8);
  }

  // St07
  if (tempSupply7 > 0.0 && tempReturn7 > 0.0)
  {
    sprintf(supplyStr7, "%.2f", tempSupply7);
    sprintf(returnStr7, "%.2f", tempReturn7);
    mqttClient.publish("SprutHub/St07-P/DS18B20/temperature", supplyStr7, true);
    mqttClient.publish("SprutHub/St07-O/DS18B20/temperature", returnStr7, true);
    Serial.printf("MQTT -> St07-P: %s  |  St07-O: %s\n", supplyStr7, returnStr7);
  }

  // St06
  if (tempSupply6 > 0.0 && tempReturn6 > 0.0)
  {
    sprintf(supplyStr6, "%.2f", tempSupply6);
    sprintf(returnStr6, "%.2f", tempReturn6);
    mqttClient.publish("SprutHub/St06-P/DS18B20/temperature", supplyStr6, true);
    mqttClient.publish("SprutHub/St06-O/DS18B20/temperature", returnStr6, true);
    Serial.printf("MQTT -> St06-P: %s  |  St06-O: %s\n", supplyStr6, returnStr6);
  }
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
