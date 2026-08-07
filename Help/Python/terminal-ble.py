import asyncio
import sys
from bleak import BleakScanner, BleakClient

# Standard Nordic UART Service (NUS) UUIDs
UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
UART_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" # Peripheral TX (PC RX - Notify)
UART_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" # Peripheral RX (PC TX - Write)

def notification_handler(sender, data):
    """Callback function for handling data received from the BLE peripheral."""
    try:
        print(f"\n[Received]: {data.decode('utf-8').strip()}")
    except UnicodeDecodeError:
        print(f"\n[Received Hex]: {data.hex()}")
    print("[Send] > ", end="", flush=True)

async def scan_for_devices():
    """Scans for nearby BLE devices and prompts the user to select one."""
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover()
    
    if not devices:
        print("No BLE devices found.")
        return None

    print("\nAvailable Devices:")
    for idx, device in enumerate(devices):
        name = device.name if device.name else "Unknown Device"
        print(f"[{idx}] {name} ({device.address})")

    try:
        selection = int(input("\nSelect a device number to connect: "))
        if 0 <= selection < len(devices):
            return devices[selection].address
    except ValueError:
        pass
    
    print("Invalid selection.")
    return None

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
    # Step 1: Scan and Pick Device
    device_address = await scan_for_devices()
    if not device_address:
        return

    # Step 2: Connect and Run Terminal
    print(f"Connecting to {device_address}...")
    async with BleakClient(device_address) as client:
        if client.is_connected:
            # Subscribe to notifications from the peripheral
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
        sys.exit(0)
