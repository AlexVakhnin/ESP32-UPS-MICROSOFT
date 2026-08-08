import asyncio
import sys
#import pyautogui
from bleak import BleakScanner, BleakClient

# Standard Nordic UART Service (NUS) UUIDs
UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
UART_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" # Peripheral TX (PC RX - Notify)
UART_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" # Peripheral RX (PC TX - Write)

#global _flag
#_flag = 1

def handle_disconnect(client: BleakClient):
    print(f"[-] Соединение с устройством {client.address} потеряно!")


def notification_handler(sender, data):
    """Callback function for handling data received from the BLE peripheral."""
    try:
        print(f"\n[Received]: {data.decode('utf-8').strip()}")
    except UnicodeDecodeError:
        print(f"\n[Received Hex]: {data.hex()}")
    print("[Send] > ", end="", flush=True)

# terminal cycle organization-----------------------------------------------------------
async def terminal_input_loop(client):
    """Asynchronous loop to take terminal input and send it over BLE."""
    loop = asyncio.get_running_loop()
    print("\n--- Connected to BLE Terminal! Type 'exit' or 'quit' to disconnect. ---")
    
    while True:
        # Run input() in a separate thread so it does not block incoming notifications
        user_input = await loop.run_in_executor(None, lambda: input("[Send] > "))

        if user_input.lower() in ['exit', 'quit']:
            break
            
        if user_input:
            # Convert text string to bytes and send to the peripheral's RX characteristic
            data_to_send = (user_input + "\n").encode('utf-8')
            await client.write_gatt_char(UART_RX_CHAR_UUID, data_to_send, response=False)

async def main():
    device_address = "EC:DA:3B:BE:25:16"
    if not device_address:
        return

    # Connect and Run Terminal-------------------------------------------------
    print(f"Connecting to {device_address}...")
    async with BleakClient(device_address, disconnected_callback=handle_disconnect ) as client:

        if client.is_connected:
            # Subscribe to notifications from the peripheral(+callback: notification_handler)
            await client.start_notify(UART_TX_CHAR_UUID, notification_handler)
            
            # Start the terminal interactive shell
            await terminal_input_loop(client)
            
            # Clean up before exit
            await client.stop_notify(UART_TX_CHAR_UUID)
            print("Disconnected safely.")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nProgram terminated by user.")
    finally:
        sys.exit(0)
