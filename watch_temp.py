#!/usr/bin/env python3
"""
Мониторинг температуры стояков Ст01-Ст17, TU и Outdoors
Показывает температуру подачи, обратки и разницу в реальном времени
"""

import sys
sys.path.insert(0, '.')
from config import MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS
import paho.mqtt.client as mqtt
import signal
import time
from datetime import datetime

# Словарь для хранения температур: {номер_стояка: {"P": temp, "O": temp}}
stations = {}
data_count = 0
last_output_time = {}  # Для отслеживания последнего вывода

# Цвета для вывода
GREEN = '\033[92m'
YELLOW = '\033[93m'
RED = '\033[91m'
BLUE = '\033[94m'
RESET = '\033[0m'

def signal_handler(sig, frame):
    print("\n\n=== Статистика сессии ===")
    print(f"Всего сообщений: {data_count}")
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"✅ Подключено к MQTT брокеру ({MQTT_SERVER}:{MQTT_PORT})")
        print("📡 Подписка на все топики: SprutHub/#")
        print("🔄 Ожидание данных...\n")
        # Подписываемся на ВСЕ топики через wildcard
        client.subscribe("SprutHub/#")
    else:
        print(f"❌ Ошибка подключения: {rc}")
        sys.exit(1)

def on_message(client, userdata, msg, properties=None):
    global data_count
    
    try:
        value = float(msg.payload.decode())
    except ValueError:
        return
    
    # Парсим номер стояка из топика (SprutHub/St01-P/DS18B20/temperature)
    parts = msg.topic.split("/")
    if len(parts) < 3:
        return
    
    # parts[1] = "St01-P", parts[2] = "DS18B20"
    station_channel = parts[1]  # "St01-P"
    
    # Разделяем стояк и канал
    if "-P" in station_channel:
        station_id = station_channel.replace("-P", "")
        channel = "P"
        desc = "Подача"
    elif "-O" in station_channel:
        station_id = station_channel.replace("-O", "")
        channel = "O"
        desc = "Обратка"
    else:
        return
    
    # Сохраняем температуру
    if station_id not in stations:
        stations[station_id] = {}
    
    old_temp = stations[station_id].get(channel)
    stations[station_id][channel] = value
    data_count += 1
    
    # Показываем только НОВЫЕ данные (с изменением)
    if old_temp is None or abs(old_temp - value) > 0.01:
        now = datetime.now().strftime("%H:%M:%S")
        
        # Определяем цвет
        if station_id == "Outdoors":
            color = BLUE
            desc_full = "Улица"
        elif station_id == "TU":
            color = YELLOW
            desc_full = "ТеплоУзел"
        else:
            color = GREEN
            desc_full = station_id
        
        # Показываем изменение
        change = ""
        if old_temp is not None:
            diff = value - old_temp
            if diff > 0.1:
                change = f" {YELLOW}↑{diff:+.2f}{RESET}"
            elif diff < -0.1:
                change = f" {RED}↓{diff:+.2f}{RESET}"
        
        print(f"{color}[{data_count:04d}] {now}  {station_id}-{desc}: {value:6.2f}°C {change}{RESET}")
    else:
        # Даже без изменения показываем что сообщение пришло
        now = datetime.now().strftime("%H:%M:%S")
        print(f"  [{data_count:04d}] {now}  {station_id}-{desc}: {value:6.2f}°C (без изменений)")


# Создаём клиент (v2 API)
client = mqtt.Client(
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    client_id="smarthome-watcher"
)

# Настройки подключения
client.username_pw_set(MQTT_USER, MQTT_PASS)
client.on_connect = on_connect
client.on_message = on_message


# Подключаемся
print("🏠 SmartHome — Мониторинг температуры (реальное время)")
print(f"⏳ Подключение к {MQTT_SERVER}:{MQTT_PORT}...")
try:
    client.connect(MQTT_SERVER, MQTT_PORT, 60)
except Exception as e:
    print(f"❌ Ошибка подключения: {e}")
    sys.exit(1)

# Запускаем цикл в отдельном потоке
client.loop_start()
print("📡 Запущен мониторинг. Нажмите Ctrl+C для выхода.\n")

# Периодический вывод полной таблицы
try:
    while True:
        time.sleep(5)
        
        # Вывод полной таблицы каждые 30 секунд
        if datetime.now().second == 0 or datetime.now().second == 30:
            now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            print(f"\n{'='*60}")
            print(f"📊 {now} — Полная таблица:")
            print(f"{'='*60}")
            
            for sid, temps in sorted(stations.items()):
                if "P" in temps and "O" in temps:
                    p_temp = temps["P"]
                    o_temp = temps["O"]
                    diff = p_temp - o_temp
                    
                    # Цвет разницы
                    if abs(diff) < 1.0:
                        color = GREEN
                    elif diff > 10.0:
                        color = RED
                    else:
                        color = YELLOW
                        
                    print(f"{color}{sid}: {p_temp:6.2f}°C / {o_temp:6.2f}°C  Δ{color}{diff:+5.2f}°C{RESET}")
                elif "P" in temps:
                    print(f"{BLUE}{sid}: {temps['P']:6.2f}°C (только Подача){RESET}")
            
            print(f"{'='*60}\n")
            
except KeyboardInterrupt:
    client.loop_stop()
    signal_handler(None, None)
