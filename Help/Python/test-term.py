
import asyncio

from bleak import BleakClient, BleakError

async def main():
    ble_address = "EC:DA:3B:BE:25:16"
    TX_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
    RX_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
    data_to_send = bytearray(b"ati")
    try:
        async with BleakClient(ble_address) as client:
            print(f"Connected: {client.is_connected}")
            await client.write_gatt_char(RX_UUID, data_to_send, response=True)
            print(f"Data '{data_to_send.decode()}' written to {RX_UUID}.")

            rdata = await client.read_gatt_char(TX_UUID)
            print(f"Data term: {rdata.decode()}")
    except BleakError as e:
        print(f"Bleak Error: {e}")
    except asyncio.TimeoutError:
        print("Timeout Error...")

asyncio.run(main())

###
