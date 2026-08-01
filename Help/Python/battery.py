import asyncio
import subprocess
from bleak import BleakClient

# --- НАСТРОЙКИ ---
DEVICE_ADDRESS = "EC:DA:3B:BE:25:16"  # Замените на MAC вашего устройства
BATTERY_CHAR_UUID = "00002a19-0000-1000-8000-00805f9b34fb"  # Стандартный UUID характеристики заряда
# CRITICAL_LEVEL = 20  # Порог заряда (в %), при котором сработает скрипт
# SCRIPT_PATH = r"C:\path\to\your\script.bat"  # Полный путь к вашему Windows-скрипту (.bat, .exe или .ps1)
CHECK_INTERVAL = 60  # Интервал проверки устройства (в секундах)
# ------------------

async def check_battery():
    try:
        print(f"Подключение к {DEVICE_ADDRESS}...")
        async with BleakClient(DEVICE_ADDRESS) as client:
            if client.is_connected:
                print("Успешно подключено.")
                # Читаем байт данных из характеристики батареи
                battery_level_bytes = await client.read_gatt_char(BATTERY_CHAR_UUID)
                battery_level = battery_level_bytes[0]
                print(f"Текущий уровень заряда BLE устройства: {battery_level}%")
                
                # Проверяем условие
                #if battery_level <= CRITICAL_LEVEL:
                #    print(f"Внимание! Заряд ниже {CRITICAL_LEVEL}%. Запуск внешнего скрипта...")
                #    # Запуск Windows скрипта
                #    subprocess.run(SCRIPT_PATH, shell=True)
                    
    except Exception as e:
        print(f"Ошибка при работе с BLE устройством: {e}")

async def main():
    while True:
        await check_battery()
        print(f"Ожидание {CHECK_INTERVAL} секунд до следующей проверки...\n")
        await asyncio.sleep(CHECK_INTERVAL)

if __name__ == "__main__":
    asyncio.run(main())
