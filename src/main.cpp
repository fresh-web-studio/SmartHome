/**
 * ESP32 - Датчик температуры на DS18B20 для стояков отопления
 * Отправляет данные температуры в SprutHub CE через MQTT
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <EthernetENC.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

// Configuration
#include "config.h"

// Датчики температуры
#define ONE_WIRE_BUS_1 4
#define ONE_WIRE_BUS_2 16  // D16 для Ст06-Ст08
#define ONE_WIRE_BUS_3 17  // D17 для Ст09-Ст13
#define ONE_WIRE_BUS_4 18  // D18 для Ст17

OneWire oneWire1(ONE_WIRE_BUS_1);
OneWire oneWire2(ONE_WIRE_BUS_2);
OneWire oneWire3(ONE_WIRE_BUS_3);
OneWire oneWire4(ONE_WIRE_BUS_4);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
DallasTemperature sensors3(&oneWire3);
DallasTemperature sensors4(&oneWire4);

// Интервал опроса
#define SENSING_INTERVAL 10000
#define SEND_INTERVAL 15000

// Индикация качества Wi-Fi через встроенный LED (GPIO 2)
#define LED_BUILTIN_PIN 2
#define WIFI_LED_INTERVAL 3000  // Обновление индикации Wi-Fi каждые 3 сек

// Ethernet W5500 (HSPI: GPIO14=SCLK, GPIO13=MOSI, GPIO12=MISO, GPIO5=CS)
#define ETHERNET_CS_PIN 5
// WiFi клиент для MQTT
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

// Адреса датчиков St09 (GPIO17)
DeviceAddress st09SupplyAddr = ST09_SUPPLY_ADDR;
DeviceAddress st09ReturnAddr = ST09_RETURN_ADDR;

// Адреса датчиков St10 (GPIO17)
DeviceAddress st10SupplyAddr = ST10_SUPPLY_ADDR;
DeviceAddress st10ReturnAddr = ST10_RETURN_ADDR;

// Адреса датчиков St11 (GPIO17)
DeviceAddress st11SupplyAddr = ST11_SUPPLY_ADDR;
DeviceAddress st11ReturnAddr = ST11_RETURN_ADDR;

// Адреса датчиков St12 (GPIO17)
DeviceAddress st12SupplyAddr = ST12_SUPPLY_ADDR;
DeviceAddress st12ReturnAddr = ST12_RETURN_ADDR;

// Адреса датчиков St13 (GPIO17)
DeviceAddress st13SupplyAddr = ST13_SUPPLY_ADDR;
DeviceAddress st13ReturnAddr = ST13_RETURN_ADDR;

// Адреса датчиков St17 (GPIO18)
DeviceAddress st17SupplyAddr = ST17_SUPPLY_ADDR;
DeviceAddress st17ReturnAddr = ST17_RETURN_ADDR;

// Адреса датчиков St16 (GPIO18)
DeviceAddress st16SupplyAddr = ST16_SUPPLY_ADDR;
DeviceAddress st16ReturnAddr = ST16_RETURN_ADDR;

// Адреса датчиков St15 (GPIO18)
DeviceAddress st15SupplyAddr = ST15_SUPPLY_ADDR;
DeviceAddress st15ReturnAddr = ST15_RETURN_ADDR;

// Адреса датчиков TU (ТеплоУзел, GPIO18)
DeviceAddress tuSupplyAddr = TU_SUPPLY_ADDR;
DeviceAddress tuReturnAddr = TU_RETURN_ADDR;

// Адреса датчиков St14 (GPIO18)
DeviceAddress st14SupplyAddr = ST14_SUPPLY_ADDR;
DeviceAddress st14ReturnAddr = ST14_RETURN_ADDR;

// Адреса датчиков Outdoors (GPIO4)
DeviceAddress outdoorsSupplyAddr = OUTDOORS_SUPPLY_ADDR;
DeviceAddress outdoorsReturnAddr = OUTDOORS_RETURN_ADDR;

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
  
  // --- Шина 3 (GPIO17) ---
  Serial.println("\nИщем все датчики DS18B20 на шине 3 (GPIO17)...\n");
  
  sensors3.begin();
  sensors3.setResolution(12);
  
  int deviceCount3 = sensors3.getDeviceCount();
  
  Serial.printf("Шина 3 (GPIO17): %d датчиков\n\n", deviceCount3);
  
  for (int i = 0; i < deviceCount3; i++)
  {
    DeviceAddress addr;
    sensors3.getAddress(addr, i);
    
    Serial.printf("Датчик %d: ", i);
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] < 16)
        Serial.print("0");
      Serial.print(addr[j], HEX);
      if (j < 7)
        Serial.print(":");
    }
    
    float temp = sensors3.getTempCByIndex(i);
    if (temp == DEVICE_DISCONNECTED_C)
    {
      Serial.println("  [НЕ ОТВЕЧАЕТ]");
    }
    else
    {
      Serial.printf("  Температура: %.2f°C", temp);
    }
  }
  
  // --- Шина 4 (GPIO18) ---
  Serial.println("\n========================================");
  Serial.println("\nИщем все датчики DS18B20 на шине 4 (GPIO18)...\n");
  
  sensors4.begin();
  sensors4.setResolution(12);
  
  int deviceCount4 = sensors4.getDeviceCount();
  
  Serial.printf("Шина 4 (GPIO5): %d датчиков\n\n", deviceCount4);
  
  for (int i = 0; i < deviceCount4; i++)
  {
    DeviceAddress addr;
    sensors4.getAddress(addr, i);
    
    Serial.printf("Датчик %d: ", i);
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] < 16)
        Serial.print("0");
      Serial.print(addr[j], HEX);
      if (j < 7)
        Serial.print(":");
    }
    
    float temp = sensors4.getTempCByIndex(i);
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
  Serial.println("\n📋 АДРЕСА ДЛЯ ШИНЫ 3 (GPIO17):");
  Serial.println("\n// Адреса датчиков St09:");
  Serial.println("#define ST09_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST09_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n// Адреса датчиков St10:");
  Serial.println("#define ST10_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST10_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n// Адреса датчиков St11:");
  Serial.println("#define ST11_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST11_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n// Адреса датчиков St12:");
  Serial.println("#define ST12_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST12_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n// Адреса датчиков St13:");
  Serial.println("#define ST13_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST13_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n========================================");
  Serial.println("\n📋 АДРЕСА ДЛЯ ШИНЫ 4 (GPIO18):");
  Serial.println("\n// Адреса датчиков St17:");
  Serial.println("#define ST17_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST17_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n// Адреса датчиков St16:");
  Serial.println("#define ST16_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST16_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n// Адреса датчиков St15:");
  Serial.println("#define ST15_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST15_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n// Адреса датчиков TU (ТеплоУзел):");
  Serial.println("#define TU_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define TU_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n// Адреса датчиков St14:");
  Serial.println("#define ST14_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define ST14_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("\n// Адреса датчиков Outdoors (Температура улицы, GPIO16):");
  Serial.println("#define OUTDOORS_SUPPLY_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
  Serial.println("#define OUTDOORS_RETURN_ADDR {0x28, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}");
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

  // --- Вторая ветка (GPIO16) — отключена ---
  /*
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
  */
  
  // Пробуем найти устройства
  sensors1.begin();
  
  Serial.println("\nПоиск датчиков...");
  int deviceCount1 = sensors1.getDeviceCount();

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

  // --- Вторая ветка (GPIO16) — отключена ---
  /*
  if (deviceCount2 > 0)
  {
    Serial.printf("\n✅ ШИНА 2: НАЙДЕНО %d ДАТЧИКОВ!\n\n", deviceCount2);
    sensors1.setResolution(10);
    
    for (int i = 0; i < deviceCount2; i++)
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
    Serial.println("\n❌ ШИНА 2: ДАТЧИКИ НЕ НАЙДЕНЫ!");
  }
  */
  
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
float tempSupply9 = 0.0;  // St09 Подача
float tempReturn9 = 0.0;  // St09 Обратка
float tempSupply10 = 0.0;  // St10 Подача
float tempReturn10 = 0.0;  // St10 Обратка
float tempSupply11 = 0.0;  // St11 Подача
float tempReturn11 = 0.0;  // St11 Обратка
float tempSupply12 = 0.0;  // St12 Подача
float tempReturn12 = 0.0;  // St12 Обратка
float tempSupply13 = 0.0;  // St13 Подача
float tempReturn13 = 0.0;  // St13 Обратка
float tempSupply17 = 0.0;  // St17 Подача
float tempReturn17 = 0.0;  // St17 Обратка
float tempSupply16 = 0.0;  // St16 Подача
float tempReturn16 = 0.0;  // St16 Обратка
float tempSupply15 = 0.0;  // St15 Подача
float tempReturn15 = 0.0;  // St15 Обратка
float tempSupplyTU = 0.0;  // TU Подача
float tempReturnTU = 0.0;  // TU Обратка
float tempSupply14 = 0.0;  // St14 Подача
float tempReturn14 = 0.0;  // St14 Обратка
float tempSupplyOutdoors = 0.0;  // Outdoors Подача
float tempReturnOutdoors = 0.0;  // Outdoors Обратка
bool wifiConnected = false;
bool ethConnected = false;
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
void readTemperatureSt09();
void readTemperatureSt10();
void readTemperatureSt11();
void readTemperatureSt12();
void readTemperatureSt13();
void readTemperatureSt17();
void readTemperatureSt16();
void readTemperatureSt15();
void readTemperatureTU();
void readTemperatureSt14();
void readTemperatureOutdoors();
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
  delay(1000);  // Ждём завершения сканирования

  // Инициализация встроенного LED для индикации
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, LOW);
  Serial.println("LED indicator initialized (GPIO 2)");

  // OTA временно отключена — используется Ethernet
  // if (ethConnected)
  // {
  //   initOTA();
  // }
  // else
  // {
  //   Serial.println("OTA skipped - Ethernet not connected");
  // }
}

void loop()
{
  unsigned long currentTime = millis();

  // ==================== ПЕРЕПОДКЛЮЧЕНИЕ К WI-FI ====================
  if (!WiFi.isConnected())
  {
    Serial.println("\n⚠️  Wi-Fi отключён! Переподключение...");
    wifiConnected = false;

    WiFi.disconnect();
    delay(1000);
    WiFi.begin(WIFI_SSID_1, WIFI_PASSWORD_1);

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
    }
    else
    {
      Serial.println("\n  ❌ Не удалось переподключиться!");
    }
  }

  // Чтение температуры — без блокирующих delay!
  if (currentTime - previousSensingTime >= SENSING_INTERVAL)
  {
    previousSensingTime = currentTime;
    sensors1.requestTemperatures();
    // Ждём завершения чтения шины 1 (750мс для 12-бит)
    unsigned long waitStart = millis();
    while (millis() - waitStart < 750)
    {
      mqttClient.loop();  // Поддерживаем MQTT во время ожидания
    }
    
    sensors2.requestTemperatures();
    // Ждём завершения чтения шины 2
    waitStart = millis();
    while (millis() - waitStart < 750)
    {
      mqttClient.loop();  // Поддерживаем MQTT во время ожидания
    }
    
    readTemperature();
    readTemperatureSt02();
    readTemperatureSt03();
    readTemperatureSt04();
    readTemperatureSt05();
    readTemperatureSt08();
    readTemperatureSt07();
    readTemperatureSt06();
    readTemperatureOutdoors();
    
    // Чтение шины 3 (GPIO17) для St09, St10, St11, St12 и St13
    sensors3.requestTemperatures();
    waitStart = millis();
    while (millis() - waitStart < 750)
    {
      mqttClient.loop();  // Поддерживаем MQTT во время ожидания
    }
    readTemperatureSt09();
    readTemperatureSt10();
    readTemperatureSt11();
    readTemperatureSt12();
    readTemperatureSt13();
    
    // Чтение шины 4 (GPIO18) для St17, St16, St15, TU и St14
    sensors4.requestTemperatures();
    waitStart = millis();
    while (millis() - waitStart < 750)
    {
      mqttClient.loop();  // Поддерживаем MQTT во время ожидания
    }
    readTemperatureSt17();
    readTemperatureSt16();
    readTemperatureSt15();
    readTemperatureTU();
    readTemperatureSt14();
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
    Serial.println("⚠️  MQTT disconnected — reconnecting...");
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

  // Индикация работы: мигание каждые 2 секунды
  static unsigned long ledBlinkLast = 0;
  if (millis() - ledBlinkLast > 2000)
  {
    ledBlinkLast = millis();
    digitalWrite(LED_BUILTIN_PIN, !digitalRead(LED_BUILTIN_PIN));
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

// ======================== ЧТЕНИЕ ДАТЧИКОВ St09 (GPIO17) ========================

void readTemperatureSt09()
{
  int deviceCount = sensors3.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors3.getTempCByIndex(i);
    DeviceAddress addr;
    sensors3.getAddress(addr, i);

    // Проверяем, это датчик Ст09 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st09SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст09 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st09ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст09 Подача: ERROR");
      }
      else
      {
        tempSupply9 = temp;
        Serial.printf("🔥 Ст09 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст09 Обратка: ERROR");
      }
      else
      {
        tempReturn9 = temp;
        Serial.printf("❄️  Ст09 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст09 Подача NOT FOUND!");
    tempSupply9 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст09 Обратка NOT FOUND!");
    tempReturn9 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply9 == tempReturn9)
  {
    Serial.println("⚠️  Ст09: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply9 = 0.0;
    tempReturn9 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St10 (GPIO17) ========================

void readTemperatureSt10()
{
  int deviceCount = sensors3.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors3.getTempCByIndex(i);
    DeviceAddress addr;
    sensors3.getAddress(addr, i);

    // Проверяем, это датчик Ст10 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st10SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст10 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st10ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст10 Подача: ERROR");
      }
      else
      {
        tempSupply10 = temp;
        Serial.printf("🔥 Ст10 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст10 Обратка: ERROR");
      }
      else
      {
        tempReturn10 = temp;
        Serial.printf("❄️  Ст10 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст10 Подача NOT FOUND!");
    tempSupply10 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст10 Обратка NOT FOUND!");
    tempReturn10 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply10 == tempReturn10)
  {
    Serial.println("⚠️  Ст10: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply10 = 0.0;
    tempReturn10 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St11 (GPIO17) ========================

void readTemperatureSt11()
{
  int deviceCount = sensors3.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors3.getTempCByIndex(i);
    DeviceAddress addr;
    sensors3.getAddress(addr, i);

    // Проверяем, это датчик Ст11 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st11SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст11 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st11ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст11 Подача: ERROR");
      }
      else
      {
        tempSupply11 = temp;
        Serial.printf("🔥 Ст11 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст11 Обратка: ERROR");
      }
      else
      {
        tempReturn11 = temp;
        Serial.printf("❄️  Ст11 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст11 Подача NOT FOUND!");
    tempSupply11 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст11 Обратка NOT FOUND!");
    tempReturn11 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply11 == tempReturn11)
  {
    Serial.println("⚠️  Ст11: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply11 = 0.0;
    tempReturn11 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St12 (GPIO17) ========================

void readTemperatureSt12()
{
  int deviceCount = sensors3.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors3.getTempCByIndex(i);
    DeviceAddress addr;
    sensors3.getAddress(addr, i);

    // Проверяем, это датчик Ст12 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st12SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст12 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st12ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст12 Подача: ERROR");
      }
      else
      {
        tempSupply12 = temp;
        Serial.printf("🔥 Ст12 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст12 Обратка: ERROR");
      }
      else
      {
        tempReturn12 = temp;
        Serial.printf("❄️  Ст12 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст12 Подача NOT FOUND!");
    tempSupply12 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст12 Обратка NOT FOUND!");
    tempReturn12 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply12 == tempReturn12)
  {
    Serial.println("⚠️  Ст12: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply12 = 0.0;
    tempReturn12 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St13 (GPIO17) ========================

void readTemperatureSt13()
{
  int deviceCount = sensors3.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors3.getTempCByIndex(i);
    DeviceAddress addr;
    sensors3.getAddress(addr, i);

    // Проверяем, это датчик Ст13 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st13SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст13 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st13ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст13 Подача: ERROR");
      }
      else
      {
        tempSupply13 = temp;
        Serial.printf("🔥 Ст13 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст13 Обратка: ERROR");
      }
      else
      {
        tempReturn13 = temp;
        Serial.printf("❄️  Ст13 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст13 Подача NOT FOUND!");
    tempSupply13 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст13 Обратка NOT FOUND!");
    tempReturn13 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply13 == tempReturn13)
  {
    Serial.println("⚠️  Ст13: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply13 = 0.0;
    tempReturn13 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St17 (GPIO5) ========================

void readTemperatureSt17()
{
  int deviceCount = sensors4.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors4.getTempCByIndex(i);
    DeviceAddress addr;
    sensors4.getAddress(addr, i);

    // Проверяем, это датчик Ст17 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st17SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст17 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st17ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст17 Подача: ERROR");
      }
      else
      {
        tempSupply17 = temp;
        Serial.printf("🔥 Ст17 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст17 Обратка: ERROR");
      }
      else
      {
        tempReturn17 = temp;
        Serial.printf("❄️  Ст17 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст17 Подача NOT FOUND!");
    tempSupply17 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст17 Обратка NOT FOUND!");
    tempReturn17 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply17 == tempReturn17)
  {
    Serial.println("⚠️  Ст17: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply17 = 0.0;
    tempReturn17 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St16 (GPIO18) ========================

void readTemperatureSt16()
{
  int deviceCount = sensors4.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors4.getTempCByIndex(i);
    DeviceAddress addr;
    sensors4.getAddress(addr, i);

    // Проверяем, это датчик Ст16 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st16SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст16 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st16ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст16 Подача: ERROR");
      }
      else
      {
        tempSupply16 = temp;
        Serial.printf("🔥 Ст16 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст16 Обратка: ERROR");
      }
      else
      {
        tempReturn16 = temp;
        Serial.printf("❄️  Ст16 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст16 Подача NOT FOUND!");
    tempSupply16 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст16 Обратка NOT FOUND!");
    tempReturn16 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply16 == tempReturn16)
  {
    Serial.println("⚠️  Ст16: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply16 = 0.0;
    tempReturn16 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St15 (GPIO18) ========================

void readTemperatureSt15()
{
  int deviceCount = sensors4.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors4.getTempCByIndex(i);
    DeviceAddress addr;
    sensors4.getAddress(addr, i);

    // Проверяем, это датчик Ст15 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st15SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст15 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st15ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст15 Подача: ERROR");
      }
      else
      {
        tempSupply15 = temp;
        Serial.printf("🔥 Ст15 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст15 Обратка: ERROR");
      }
      else
      {
        tempReturn15 = temp;
        Serial.printf("❄️  Ст15 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст15 Подача NOT FOUND!");
    tempSupply15 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст15 Обратка NOT FOUND!");
    tempReturn15 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply15 == tempReturn15)
  {
    Serial.println("⚠️  Ст15: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply15 = 0.0;
    tempReturn15 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ TU (ТеплоУзел, GPIO18) ========================

void readTemperatureTU()
{
  int deviceCount = sensors4.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors4.getTempCByIndex(i);
    DeviceAddress addr;
    sensors4.getAddress(addr, i);

    // Проверяем, это датчик TU Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != tuSupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик TU Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != tuReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 TU Подача: ERROR");
      }
      else
      {
        tempSupplyTU = temp;
        Serial.printf("🔥 TU Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  TU Обратка: ERROR");
      }
      else
      {
        tempReturnTU = temp;
        Serial.printf("❄️  TU Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  TU Подача NOT FOUND!");
    tempSupplyTU = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  TU Обратка NOT FOUND!");
    tempReturnTU = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupplyTU == tempReturnTU)
  {
    Serial.println("⚠️  TU: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupplyTU = 0.0;
    tempReturnTU = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ St14 (GPIO18) ========================

void readTemperatureSt14()
{
  int deviceCount = sensors4.getDeviceCount();

  if (deviceCount == 0)
    return;

  bool supplyFound = false;
  bool returnFound = false;

  for (int i = 0; i < deviceCount; i++)
  {
    float temp = sensors4.getTempCByIndex(i);
    DeviceAddress addr;
    sensors4.getAddress(addr, i);

    // Проверяем, это датчик Ст14 Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st14SupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Ст14 Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != st14ReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🔥 Ст14 Подача: ERROR");
      }
      else
      {
        tempSupply14 = temp;
        Serial.printf("🔥 Ст14 Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Ст14 Обратка: ERROR");
      }
      else
      {
        tempReturn14 = temp;
        Serial.printf("❄️  Ст14 Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Ст14 Подача NOT FOUND!");
    tempSupply14 = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Ст14 Обратка NOT FOUND!");
    tempReturn14 = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupply14 == tempReturn14)
  {
    Serial.println("⚠️  Ст14: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupply14 = 0.0;
    tempReturn14 = 0.0;
  }
}

// ======================== ЧТЕНИЕ ДАТЧИКОВ Outdoors (GPIO4) ========================

void readTemperatureOutdoors()
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

    // Проверяем, это датчик Outdoors Подача?
    bool isSupply = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != outdoorsSupplyAddr[j])
      {
        isSupply = false;
        break;
      }
    }

    // Проверяем, это датчик Outdoors Обратка?
    bool isReturn = true;
    for (int j = 0; j < 8; j++)
    {
      if (addr[j] != outdoorsReturnAddr[j])
      {
        isReturn = false;
        break;
      }
    }

    if (isSupply)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("🌡️  Outdoors Подача: ERROR");
      }
      else
      {
        tempSupplyOutdoors = temp;
        Serial.printf("🌡️  Outdoors Подача: %.2f°C\n", temp);
        supplyFound = true;
      }
    }
    else if (isReturn)
    {
      if (temp == DEVICE_DISCONNECTED_C)
      {
        Serial.println("❄️  Outdoors Обратка: ERROR");
      }
      else
      {
        tempReturnOutdoors = temp;
        Serial.printf("❄️  Outdoors Обратка: %.2f°C\n", temp);
        returnFound = true;
      }
    }
  }

  if (!supplyFound)
  {
    Serial.println("⚠️  Outdoors Подача NOT FOUND!");
    tempSupplyOutdoors = 0.0;
  }
  if (!returnFound)
  {
    Serial.println("⚠️  Outdoors Обратка NOT FOUND!");
    tempReturnOutdoors = 0.0;
  }
  
  // Проверка: если температуры одинаковые — ошибка чтения
  if (supplyFound && returnFound && tempSupplyOutdoors == tempReturnOutdoors)
  {
    Serial.println("⚠️  Outdoors: Одинаковые температуры — ошибка чтения 1-Wire!");
    tempSupplyOutdoors = 0.0;
    tempReturnOutdoors = 0.0;
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
  char supplyStr9[10];
  char returnStr9[10];
  char supplyStr10[10];
  char returnStr10[10];
  char supplyStr11[10];
  char returnStr11[10];
  char supplyStr12[10];
  char returnStr12[10];
  char supplyStr13[10];
  char returnStr13[10];
  char supplyStr17[10];
  char returnStr17[10];
  char supplyStr16[10];
  char returnStr16[10];
  char supplyStr15[10];
  char returnStr15[10];
  char supplyStrTU[10];
  char returnStrTU[10];
  char supplyStr14[10];
  char returnStr14[10];
  char supplyStrOutdoors[10];
  char returnStrOutdoors[10];

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

  // St09 (Ветка 3, GPIO17)
  if (tempSupply9 > 0.0 && tempReturn9 > 0.0)
  {
    sprintf(supplyStr9, "%.2f", tempSupply9);
    sprintf(returnStr9, "%.2f", tempReturn9);
    mqttClient.publish("SprutHub/St09-P/DS18B20/temperature", supplyStr9, true);
    mqttClient.publish("SprutHub/St09-O/DS18B20/temperature", returnStr9, true);
    Serial.printf("MQTT -> St09-P: %s  |  St09-O: %s\n", supplyStr9, returnStr9);
  }

  // St10 (Ветка 3, GPIO17)
  if (tempSupply10 > 0.0 && tempReturn10 > 0.0)
  {
    sprintf(supplyStr10, "%.2f", tempSupply10);
    sprintf(returnStr10, "%.2f", tempReturn10);
    mqttClient.publish("SprutHub/St10-P/DS18B20/temperature", supplyStr10, true);
    mqttClient.publish("SprutHub/St10-O/DS18B20/temperature", returnStr10, true);
    Serial.printf("MQTT -> St10-P: %s  |  St10-O: %s\n", supplyStr10, returnStr10);
  }

  // St11 (Ветка 3, GPIO17)
  if (tempSupply11 > 0.0 && tempReturn11 > 0.0)
  {
    sprintf(supplyStr11, "%.2f", tempSupply11);
    sprintf(returnStr11, "%.2f", tempReturn11);
    mqttClient.publish("SprutHub/St11-P/DS18B20/temperature", supplyStr11, true);
    mqttClient.publish("SprutHub/St11-O/DS18B20/temperature", returnStr11, true);
    Serial.printf("MQTT -> St11-P: %s  |  St11-O: %s\n", supplyStr11, returnStr11);
  }

  // St12 (Ветка 3, GPIO17)
  if (tempSupply12 > 0.0 && tempReturn12 > 0.0)
  {
    sprintf(supplyStr12, "%.2f", tempSupply12);
    sprintf(returnStr12, "%.2f", tempReturn12);
    mqttClient.publish("SprutHub/St12-P/DS18B20/temperature", supplyStr12, true);
    mqttClient.publish("SprutHub/St12-O/DS18B20/temperature", returnStr12, true);
    Serial.printf("MQTT -> St12-P: %s  |  St12-O: %s\n", supplyStr12, returnStr12);
  }

  // St13 (Ветка 3, GPIO17)
  if (tempSupply13 > 0.0 && tempReturn13 > 0.0)
  {
    sprintf(supplyStr13, "%.2f", tempSupply13);
    sprintf(returnStr13, "%.2f", tempReturn13);
    mqttClient.publish("SprutHub/St13-P/DS18B20/temperature", supplyStr13, true);
    mqttClient.publish("SprutHub/St13-O/DS18B20/temperature", returnStr13, true);
    Serial.printf("MQTT -> St13-P: %s  |  St13-O: %s\n", supplyStr13, returnStr13);
  }

  // St17 (Ветка 4, GPIO18)
  if (tempSupply17 > 0.0 && tempReturn17 > 0.0)
  {
    sprintf(supplyStr17, "%.2f", tempSupply17);
    sprintf(returnStr17, "%.2f", tempReturn17);
    mqttClient.publish("SprutHub/St17-P/DS18B20/temperature", supplyStr17, true);
    mqttClient.publish("SprutHub/St17-O/DS18B20/temperature", returnStr17, true);
    Serial.printf("MQTT -> St17-P: %s  |  St17-O: %s\n", supplyStr17, returnStr17);
  }

  // St16 (Ветка 4, GPIO18)
  if (tempSupply16 > 0.0 && tempReturn16 > 0.0)
  {
    sprintf(supplyStr16, "%.2f", tempSupply16);
    sprintf(returnStr16, "%.2f", tempReturn16);
    mqttClient.publish("SprutHub/St16-P/DS18B20/temperature", supplyStr16, true);
    mqttClient.publish("SprutHub/St16-O/DS18B20/temperature", returnStr16, true);
    Serial.printf("MQTT -> St16-P: %s  |  St16-O: %s\n", supplyStr16, returnStr16);
  }

  // St15 (Ветка 4, GPIO18)
  if (tempSupply15 > 0.0 && tempReturn15 > 0.0)
  {
    sprintf(supplyStr15, "%.2f", tempSupply15);
    sprintf(returnStr15, "%.2f", tempReturn15);
    mqttClient.publish("SprutHub/St15-P/DS18B20/temperature", supplyStr15, true);
    mqttClient.publish("SprutHub/St15-O/DS18B20/temperature", returnStr15, true);
    Serial.printf("MQTT -> St15-P: %s  |  St15-O: %s\n", supplyStr15, returnStr15);
  }

  // TU (ТеплоУзел, Ветка 4, GPIO18)
  if (tempSupplyTU > 0.0 && tempReturnTU > 0.0)
  {
    sprintf(supplyStrTU, "%.2f", tempSupplyTU);
    sprintf(returnStrTU, "%.2f", tempReturnTU);
    mqttClient.publish("SprutHub/TU-P/DS18B20/temperature", supplyStrTU, true);
    mqttClient.publish("SprutHub/TU-O/DS18B20/temperature", returnStrTU, true);
    Serial.printf("MQTT -> TU-P: %s  |  TU-O: %s\n", supplyStrTU, returnStrTU);
  }

  // St14 (Ветка 4, GPIO18)
  if (tempSupply14 > 0.0 && tempReturn14 > 0.0)
  {
    sprintf(supplyStr14, "%.2f", tempSupply14);
    sprintf(returnStr14, "%.2f", tempReturn14);
    mqttClient.publish("SprutHub/St14-P/DS18B20/temperature", supplyStr14, true);
    mqttClient.publish("SprutHub/St14-O/DS18B20/temperature", returnStr14, true);
    Serial.printf("MQTT -> St14-P: %s  |  St14-O: %s\n", supplyStr14, returnStr14);
  }

  // Outdoors (Ветка 1, GPIO4) — только Подача (нет Обратки)
  if (tempSupplyOutdoors > 0.0)
  {
    sprintf(supplyStrOutdoors, "%.2f", tempSupplyOutdoors);
    mqttClient.publish("SprutHub/Outdoors-P/DS18B20/temperature", supplyStrOutdoors, true);
    Serial.printf("MQTT -> Outdoors-P: %s\n", supplyStrOutdoors);
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
