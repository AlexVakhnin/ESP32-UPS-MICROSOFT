import asyncio
import queue
import threading
import tkinter as tk
from tkinter import messagebox, ttk
from bleak import BleakClient, BleakScanner

# Common Nordic UART Service (NUS) UUIDs for Serial/Terminal emulation
# Update these if your target peripheral uses custom service/characteristic UUIDs
UART_TX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # Read/Notify from device
UART_RX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  # Write to device


class BleTerminalApp:

    def __init__(self, root):
        self.root = root
        self.root.title("BLE Terminal Pro")
        self.root.geometry("600x500")

        # Asyncio loop and thread variables
        self.loop = None
        self.client = None
        self.gui_queue = queue.Queue()

        # UI Layout setup
        self._build_ui()

        # Start background thread to handle asyncio event loop
        self.ble_thread = threading.Thread(
            target=self._start_async_loop, daemon=True
        )
        self.ble_thread.start()

        # Start periodic GUI queue processor
        self.root.after(100, self._process_queue)

    def _build_ui(self):
        """Builds the Tkinter interface layout."""
        # --- Top Scanning Frame ---
        scan_frame = ttk.LabelFrame(self.root, text=" 1. Connections ", padding=10)
        scan_frame.pack(fill="x", padx=10, pady=5)

        self.btn_scan = ttk.Button(
            scan_frame, text="Scan Devices", command=self.trigger_scan
        )
        self.btn_scan.pack(side="left", padx=5)

        self.device_combo = ttk.Combobox(
            scan_frame, state="readonly", width=40
        )
        self.device_combo.pack(side="left", padx=5, fill="x", expand=True)
        self.device_combo.set("Click Scan Devices to search...")

        self.btn_connect = ttk.Button(
            scan_frame, text="Connect", command=self.trigger_connect
        )
        self.btn_connect.pack(side="left", padx=5)

        # --- Middle Terminal Output ---
        output_frame = ttk.LabelFrame(self.root, text=" 2. Terminal Log ", padding=10)
        output_frame.pack(fill="both", expand=True, padx=10, pady=5)

        self.txt_log = tk.Text(output_frame, state="disabled", wrap="word")
        self.txt_log.pack(fill="both", expand=True, side="left")

        scrollbar = ttk.Scrollbar(
            output_frame, orient="vertical", command=self.txt_log.yview
        )
        scrollbar.pack(fill="y", side="right")
        self.txt_log["yscrollcommand"] = scrollbar.set

        # --- Bottom Input Command Frame ---
        input_frame = ttk.LabelFrame(self.root, text=" 3. Send Data ", padding=10)
        input_frame.pack(fill="x", padx=10, pady=10)

        self.ent_input = ttk.Entry(input_frame)
        self.ent_input.pack(side="left", fill="x", expand=True, padx=5)
        self.ent_input.bind("<Return>", lambda e: self.trigger_send())

        self.btn_send = ttk.Button(
            input_frame, text="Send", command=self.trigger_send, state="disabled"
        )
        self.btn_send.pack(side="right", padx=5)

    # -------------------------------------------------------------------------
    # Thread Safe Asyncio Management
    # -------------------------------------------------------------------------

    def _start_async_loop(self):
        """Runs an explicit asyncio loop inside the dedicated background thread."""
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def run_async(self, coro):
        """Helper to safely push coroutines onto our active asyncio loop thread."""
        if self.loop and self.loop.is_running():
            asyncio.run_coroutine_threadsafe(coro, self.loop)

    def _process_queue(self):
        """Checks the queue periodically for requests meant to update the main thread UI."""
        while not self.gui_queue.empty():
            task, data = self.gui_queue.get()

            if task == "LOG":
                self.txt_log.config(state="normal")
                self.txt_log.insert("end", f"{data}\n")
                self.txt_log.see("end")
                self.txt_log.config(state="disabled")

            elif task == "SCAN_DONE":
                self.device_combo["values"] = data
                if data:
                    self.device_combo.current(0)
                else:
                    self.device_combo.set("No devices found.")
                self.btn_scan.config(state="normal")

            elif task == "CONNECTED":
                self.btn_connect.config(text="Disconnect", state="normal")
                self.btn_send.config(state="normal")

            elif task == "DISCONNECTED":
                self.btn_connect.config(text="Connect", state="normal")
                self.btn_send.config(state="disabled")
                self.client = None

            elif task == "ERROR":
                messagebox.showerror("Error", data)
                self.btn_scan.config(state="normal")
                self.btn_connect.config(text="Connect", state="normal")

        # Loop this check every 100 milliseconds
        self.root.after(100, self._process_queue)

    # -------------------------------------------------------------------------
    # BLE Core Functions (Asyncio Methods)
    # -------------------------------------------------------------------------

    async def _async_scan(self):
        """Scans for nearby BLE devices and pushes updates back to GUI."""
        self.gui_queue.put(("LOG", "[Scanning for devices...]"))
        try:
            devices = await BleakScanner.discover(timeout=4.0)
            # Map out names paired with addresses for dropdown menu
            dev_strings = [
                f"{d.name or 'Unknown'} [{d.address}]" for d in devices
            ]
            self.gui_queue.put(("SCAN_DONE", dev_strings))
            self.gui_queue.put(("LOG", f"[Scan ended. Found {len(devices)} items]"))
        except Exception as e:
            self.gui_queue.put(("ERROR", str(e)))

    async def _async_connect(self, address):
        """Connects to selected BLE address and initializes data notification feeds."""
        self.gui_queue.put(("LOG", f"[Connecting to {address}...]"))
        try:
            self.client = BleakClient(address)
            await self.client.connect()

            self.gui_queue.put(("LOG", "[Connected successfully!]"))
            self.gui_queue.put(("CONNECTED", None))

            # Attempt listening to notification characteristics
            try:
                await self.client.start_notify(
                    UART_TX_UUID, self._on_notification_received
                )
                self.gui_queue.put(
                    ("LOG", f"[Subscribed to notifications on {UART_TX_UUID}]")
                )
            except Exception as e:
                self.gui_queue.put(
                    ("LOG", f"[Notice: Could not subscribe to RX: {e}]")
                )

        except Exception as e:
            self.gui_queue.put(("ERROR", f"Connection failed: {e}"))

    async def _async_disconnect(self):
        """Gracefully tears down the existing BLE peripheral client link."""
        if self.client and self.client.is_connected:
            try:
                await self.client.disconnect()
            except Exception:
                pass
        self.gui_queue.put(("DISCONNECTED", None))
        self.gui_queue.put(("LOG", "[Disconnected]"))

    async def _async_send(self, payload):
        """Converts text payload into bytes and writes to peripheral RX line."""
        if self.client and self.client.is_connected:
            try:
                # Append newline formatting often required by terminal firmwares
                data_bytes = (payload + "\r\n").encode("utf-8")
                await self.client.write_gatt_char(
                    UART_RX_UUID, data_bytes, response=False
                )
                self.gui_queue.put(("LOG", f"-> Sent: {payload}"))
            except Exception as e:
                self.gui_queue.put(("LOG", f"[Write Error: {e}]"))

    def _on_notification_received(self, sender: int, data: bytearray):
        """Callback invoked whenever the peripheral publishes wireless payload data."""
        try:
            decoded_text = data.decode("utf-8").strip()
            self.gui_queue.put(("LOG", f"<- Recv: {decoded_text}"))
        except UnicodeDecodeError:
            self.gui_queue.put(("LOG", f"<- Recv (Hex): {data.hex()}"))

    # -------------------------------------------------------------------------
    # Main-Thread GUI Action Triggers
    # -------------------------------------------------------------------------

    def trigger_scan(self):
        self.btn_scan.config(state="disabled")
        self.run_async(self._async_scan())

    def trigger_connect(self):
        if self.client and self.client.is_connected:
            self.run_async(self._async_disconnect())
        else:
            selection = self.device_combo.get()
            if "[" not in selection or "]" not in selection:
                messagebox.showwarning("Warning", "Please select a valid device first.")
                return
            # Extract MAC/UUID address inside brackets
            address = selection.split("[")[-1].split("]")[0]
            self.run_async(self._async_connect(address))

    def trigger_send(self):
        text_to_send = self.ent_input.get()
        if text_to_send:
            self.run_async(self._async_send(text_to_send))
            self.ent_input.delete(0, tk.END)


if __name__ == "__main__":
    app_root = tk.Tk()
    terminal_app = BleTerminalApp(app_root)
    app_root.mainloop()
