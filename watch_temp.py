#!/usr/bin/env python3
"""
Мониторинг температуры стояков Ст01-Ст15
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
last_update_time = {}  # Для отслеживания последнего обновления
OUTPUT_INTERVAL = 5  # Выводить данные каждые 5 секунд

def signal_handler(sig, frame):
    print("\n\n=== Статистика сессии ===")
    print(f"Всего сообщений: {data_count}")
    print(f"Уникальных стояков: {list(stations.keys())}")
    for sid, temps in stations.items():
        p = temps.get("P", "N/A")
        o = temps.get("O", "N/A")
        print(f"  {sid}: P={p}, O={o}")
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("✅ Подключено к MQTT брокеру")
        print("📡 Подписка на топики: St01-St09")
        print("🔄 Ожидание данных...\n")
        # Подписываемся на топики
        client.subscribe("SprutHub/St01-P/DS18B20/temperature")
        client.subscribe("SprutHub/St01-O/DS18B20/temperature")
        client.subscribe("SprutHub/St02-P/DS18B20/temperature")
        client.subscribe("SprutHub/St02-O/DS18B20/temperature")
        client.subscribe("SprutHub/St03-P/DS18B20/temperature")
        client.subscribe("SprutHub/St03-O/DS18B20/temperature")
        client.subscribe("SprutHub/St04-P/DS18B20/temperature")
        client.subscribe("SprutHub/St04-O/DS18B20/temperature")
        client.subscribe("SprutHub/St05-P/DS18B20/temperature")
        client.subscribe("SprutHub/St05-O/DS18B20/temperature")
        client.subscribe("SprutHub/St08-P/DS18B20/temperature")
        client.subscribe("SprutHub/St08-O/DS18B20/temperature")
        client.subscribe("SprutHub/St07-P/DS18B20/temperature")
        client.subscribe("SprutHub/St07-O/DS18B20/temperature")
        client.subscribe("SprutHub/St06-P/DS18B20/temperature")
        client.subscribe("SprutHub/St06-O/DS18B20/temperature")
        client.subscribe("SprutHub/St09-P/DS18B20/temperature")
        client.subscribe("SprutHub/St09-O/DS18B20/temperature")
        client.subscribe("SprutHub/St10-P/DS18B20/temperature")
        client.subscribe("SprutHub/St10-O/DS18B20/temperature")
        client.subscribe("SprutHub/St11-P/DS18B20/temperature")
        client.subscribe("SprutHub/St11-O/DS18B20/temperature")
        client.subscribe("SprutHub/St12-P/DS18B20/temperature")
        client.subscribe("SprutHub/St12-O/DS18B20/temperature")
        client.subscribe("SprutHub/St13-P/DS18B20/temperature")
        client.subscribe("SprutHub/St13-O/DS18B20/temperature")
        client.subscribe("SprutHub/St17-P/DS18B20/temperature")
        client.subscribe("SprutHub/St17-O/DS18B20/temperature")
        client.subscribe("SprutHub/St16-P/DS18B20/temperature")
        client.subscribe("SprutHub/St16-O/DS18B20/temperature")
        client.subscribe("SprutHub/St15-P/DS18B20/temperature")
        client.subscribe("SprutHub/St15-O/DS18B20/temperature")
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
    elif "-O" in station_channel:
        station_id = station_channel.replace("-O", "")
        channel = "O"
    else:
        return
    
    # Сохраняем температуру
    if station_id not in stations:
        stations[station_id] = {}
    stations[station_id][channel] = value
    last_update_time[station_id] = time.time()
    
    data_count += 1



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
print("🏠 SmartHome — Мониторинг температуры")
print("⏳ Подключение к SprutHub CE...")
try:
    client.connect(MQTT_SERVER, MQTT_PORT, 60)
except Exception as e:
    print(f"❌ Ошибка подключения: {e}")
    sys.exit(1)

# Запускаем цикл в отдельном потоке
client.loop_start()
print("📡 Запущен мониторинг. Нажмите Ctrl+C для выхода.\n")

# Периодический вывод данных
try:
    while True:
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # Проверяем, есть ли данные для вывода
        has_data = False
        for sid, temps in sorted(stations.items()):
            if "P" in temps and "O" in temps:
                p_temp = temps["P"]
                o_temp = temps["O"]
                diff = p_temp - o_temp
                print(f"[{data_count:04d}] {now}  {sid}: {p_temp:6.2f}°C/{o_temp:6.2f}°C (Δ{diff:5.2f}°C)", flush=True)
                has_data = True
        
        time.sleep(OUTPUT_INTERVAL)
        
except KeyboardInterrupt:
    client.loop_stop()
    signal_handler(None, None)
