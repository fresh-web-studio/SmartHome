/**
 * Тест W5500 Ethernet модуля
 * Проверяет:
 * 1. SPI подключение (GPIO5=CS, GPIO12=MISO, GPIO13=MOSI, GPIO14=SCLK)
 * 2. Версия регистров W5500
 * 3. Возможность получения IP по DHCP
 */

#include <SPI.h>
#include <Ethernet3.h>

#define W5500_CS_PIN 5

// Версии регистров W5500
#define W5500_VERSION 0x04
#define W5100S_VERSION 0x04

void printMacAddress(byte* mac) {
  Serial.print("MAC: ");
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

void printIPAddress(IPAddress ip) {
  Serial.print("IP: ");
  Serial.println(ip);
}

// Проверка SPI связи с W5500
byte spiTransfer(byte data) {
  return SPI.transfer(data);
}

// Чтение регистра W5500
byte readW5500Register(byte reg) {
  digitalWrite(W5500_CS_PIN, LOW);
  SPI.transfer(0x00);  // Read command
  SPI.transfer(reg);
  byte data = SPI.transfer(0x00);
  digitalWrite(W5500_CS_PIN, HIGH);
  return data;
}

// Запись в регистр W5500
void writeW5500Register(byte reg, byte data) {
  digitalWrite(W5500_CS_PIN, LOW);
  SPI.transfer(0x04);  // Write command
  SPI.transfer(reg);
  SPI.transfer(data);
  digitalWrite(W5500_CS_PIN, HIGH);
}

// Проверка версии W5500
byte getW5500Version() {
  return readW5500Register(0x0039);  // VERSIONR register
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n========================================");
  Serial.println("  W5500 Ethernet Test");
  Serial.println("========================================");
  
  // Инициализация CS pin
  pinMode(W5500_CS_PIN, OUTPUT);
  digitalWrite(W5500_CS_PIN, HIGH);
  
  // Инициализация SPI
  Serial.println("\nИнициализация SPI...");
  Serial.println("  GPIO5  = CS");
  Serial.println("  GPIO12 = MISO");
  Serial.println("  GPIO13 = MOSI");
  Serial.println("  GPIO14 = SCLK");
  
  SPI.begin(14, 12, 13, 5);  // SCLK, MISO, MOSI, CS
  SPI.setClockDivider(SPI_CLOCK_DIV4);  // 21 MHz
  SPI.setDataMode(SPI_MODE0);
  
  delay(100);
  Serial.println("  SPI инициализирован");
  
  // Проверка SPI связи
  Serial.println("\nПроверка SPI связи с W5500...");
  
  // Читаем версию W5500
  byte version = getW5500Version();
  Serial.printf("  VERSIONR: 0x%02X\n", version);
  
  if (version == W5500_VERSION) {
    Serial.println("  ✅ W5500 найден! (версия 0x04)");
  } else if (version == 0xFF) {
    Serial.println("  ❌ W5500 НЕ найден! (версия 0xFF - нет связи)");
    Serial.println("  Проверьте подключение:");
    Serial.println("    - GPIO5 (CS)");
    Serial.println("    - GPIO12 (MISO)");
    Serial.println("    - GPIO13 (MOSI)");
    Serial.println("    - GPIO14 (SCLK)");
    Serial.println("    - 5V питание");
    Serial.println("    - GND");
  } else if (version == 0x00) {
    Serial.println("  ❌ W5500 НЕ найден! (версия 0x00 - короткое замыкание)");
  } else {
    Serial.printf("  ⚠️  W5500 имеет версию 0x%02X (ожидается 0x%02X)\n", version, W5500_VERSION);
  }
  
  // Проверка регистров
  Serial.println("\nПроверка регистров W5500...");
  
  byte phsr = readW5500Register(0x002C);  // PHCSR
  Serial.printf("  PHCSR: 0x%02X\n", phsr);
  
  byte mshr = readW5500Register(0x0035);  // MSHR
  Serial.printf("  MSHR: 0x%02X\n", mshr);
  
  byte tsr0 = readW5500Register(0x0025);  // TSR (Socket 0)
  Serial.printf("  TSR0: 0x%02X\n", tsr0);
  
  // Инициализация Ethernet
  Serial.println("\nИнициализация Ethernet...");
  
  byte mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x85};
  Serial.println("  MAC: ");
  printMacAddress(mac);
  
  Serial.println("  Ethernet.begin(mac)...");
  Ethernet.begin(mac);
  
  delay(2000);
  
  IPAddress ip = Ethernet.localIP();
  Serial.println("\nРезультаты:");
  printIPAddress(ip);
  
  if (ip[0] != 0 && ip[3] != 0) {
    Serial.println("  ✅ Ethernet работает!");
    Serial.printf("  IP: %s\n", ip);
    Serial.printf("  Gateway: %s\n", Ethernet.gatewayIP());
    Serial.printf("  Subnet: %s\n", Ethernet.subnetMask());
    
    byte macRead[6];
    Ethernet.macAddress(macRead);
    Serial.print("  MAC: ");
    printMacAddress(macRead);
  } else {
    Serial.println("  ❌ Ethernet НЕ работает!");
    Serial.println("  Проверьте:");
    Serial.println("    - Ethernet кабель подключён к роутеру");
    Serial.println("    - Роутер раздает IP по DHCP");
    Serial.println("    - W5500 подключён правильно");
  }
  
  // Проверка связи с роутером
  Serial.println("\nПроверка связи с роутером (192.168.1.1)...");
  if (Ethernet.hardwareStatus() == EthernetW5500) {
    Serial.println("  W5500 найден");
  } else {
    Serial.println("  W5500 НЕ найден");
  }
  
  if (Ethernet.linkStatus() == LinkON) {
    Serial.println("  ✅ Ethernet кабель подключён");
  } else if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("  ❌ Ethernet кабель НЕ подключён");
  } else {
    Serial.println("  ⚠️  Статус link неизвестен");
  }
  
  Serial.println("\n========================================");
  Serial.println("  Тест завершён");
  Serial.println("========================================");
}

void loop() {
  // Периодическая проверка
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    
    IPAddress ip = Ethernet.localIP();
    Serial.printf("\n[%lu] IP: %s\n", millis() / 1000, ip);
    
    if (Ethernet.linkStatus() == LinkON) {
      Serial.println("  Link: ON ✅");
    } else {
      Serial.println("  Link: OFF ❌");
    }
  }
  
  delay(100);
}
