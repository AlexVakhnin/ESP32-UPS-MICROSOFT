import asyncio
import threading
import time
from PIL import Image, ImageDraw
from bleak import BleakClient
import pystray

# НАСТРОЙКИ
DEVICE_ADDRESS = "EC:DA:3B:BE:25:16"  # MAC вашего устройства (для macOS укажите UUID)
BATTERY_CHAR_UUID = "00002a19-0000-1000-8000-00805f9b34fb"  # Стандартный UUID BLE Battery Level
UPDATE_INTERVAL = 60  # Интервал опроса в секундах

# Глобальные переменные состояния
current_battery = "Узнаем..."
#current_battery = 100
is_running = True
icon = None


def create_battery_icon(percentage):
    """Генерирует динамическую иконку батареи в зависимости от заряда"""
    image = Image.new("RGB", (64, 64), color="black")
    draw = ImageDraw.Draw(image)

    # Если заряд еще не определен, рисуем знак вопроса
    if isinstance(percentage, str):
        draw.text((20, 15), "?", fill="white", font_size=36)
        return image

    # Рисуем контур батарейки
    draw.rectangle([10, 15, 50, 45], outline="white", width=4)
    draw.rectangle([50, 25, 54, 35], fill="white")  # Носик батарейки

    # Заполняем батарейку цветом в зависимости от остатка
    fill_width = int(6 * (percentage / 100))  # делим на секции
    color = "green" if percentage > 20 else "red"

    for i in range(fill_width):
        start_x = 14 + (i * 5)
        draw.rectangle([start_x, 19, start_x + 3, 41], fill=color)

    return image


async def fetch_battery():
    """Асинхронный запрос заряда батареи через Bleak"""
    global current_battery
    try:
        async with BleakClient(DEVICE_ADDRESS) as client:
            if client.is_connected:
                # Читаем байты (обычно возвращается 1 байт со значением от 0 до 100)
                battery_bytes = await client.read_gatt_char(BATTERY_CHAR_UUID)
                level = int(battery_bytes[0])
                current_battery = level
                return level
    except Exception as e:
        print(f"Ошибка подключения к BLE: {e}")
        current_battery = "Ошибка"
    return None


def ble_loop_worker():
    """Фоновый цикл обновления данных о батарее"""
    global icon
    # Создаем новый цикл событий для отдельного потока
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    while is_running:
        level = loop.run_until_complete(fetch_battery())

        if icon:
            # Обновляем текст при наведении
            icon.title = f"Батарея BLE: {current_battery}%"
            # Обновляем иконку на панели задач
            icon.icon = create_battery_icon(current_battery)
            icon.update_menu()

        # Спим заданное время с шагом в 1 сек, чтобы быстро среагировать на выход
        for _ in range(UPDATE_INTERVAL):
            if not is_running:
                break
            time.sleep(1)


def on_exit(icon_item, item):
    """Корректное завершение работы приложения при клике на Выход"""
    global is_running
    is_running = False
    icon.stop()


def start_tray():
    """Запуск иконки в системном трее"""
    global icon
    menu = pystray.Menu(
        pystray.MenuItem(
            lambda text: f"Заряд: {current_battery}%", action=None, enabled=False
        ),
        pystray.MenuItem("Выход", on_exit),
    )

    icon = pystray.Icon(
        "ble_battery_monitor",
        create_battery_icon(current_battery),
        title="Мониторинг BLE батареи...",
        menu=menu,
    )

    # Запускаем BLE поток перед стартом трея
    threading.Thread(target=ble_loop_worker, daemon=True).start()

    # Запуск трея (блокирует основной поток)
    icon.run()


if __name__ == "__main__":
    start_tray()
