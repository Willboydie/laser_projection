"""
Tkinter UI for the laser projector - mirrors the serial command protocol
defined in lib/interface/Interface.hpp for Teensy communication.
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
import serial
import serial.tools.list_ports
import struct
import threading
import queue
import time

# Command constants (must match Interface.hpp)
SLEEP_CMD = 0b11100000
WAKE_CMD = 0b11100001
BOOTLOADER_CMD = 0b11100010
ENABLE_DISABLE_CMD = 0b10001111
RESTART_CMD = 0b10001110
WRITE_MODE_CMD = 0b00000010
PROJECT_MODE_CMD = 0b00000011
UPLOAD_CMD = 0b00000001
PLAY_PAUSE_CMD = 0b00001001
SET_PLAYBACK_SPEED_CMD = 0b00001010
SET_LASER_DUTY_CYCLE_CMD = 0b00011011
SEND_STATUS_CMD = 0b00001011
RECEIVE_STATUS_PACKET_CMD = 0b00011010

BAUDRATE = 115200
STATUS_POLL_INTERVAL_MS = 300
RECORD_SIZE = 5  # 2 X + 2 Y + 1 laser state
BUFFER_SIZE = 500


class ProjectorUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Laser Projector Control")
        self.root.minsize(420, 520)
        self.root.configure(bg="#f5f5f5")

        self.serial_port = None
        self.status_queue = queue.Queue()
        self.polling = False
        self.after_id = None
        self._serial_buffer = bytearray()

        self._build_ui()

    def _build_ui(self):
        """Build the complete UI layout."""
        # Style
        style = ttk.Style()
        style.theme_use("clam")
        style.configure(
            "Dark.TFrame",
            background="#f5f5f5",
        )
        style.configure(
            "Dark.TLabel",
            background="#f5f5f5",
            foreground="#333",
            font=("Consolas", 10),
        )
        style.configure(
            "Dark.TButton",
            background="#e8e8e8",
            foreground="#222",
            font=("Consolas", 9),
        )
        style.map("Dark.TButton", background=[("active", "#d0d0d0")])

        main = ttk.Frame(self.root, padding=12, style="Dark.TFrame")
        main.pack(fill=tk.BOTH, expand=True)

        # ---------- Serial connection ----------
        conn_frame = ttk.LabelFrame(main, text="Connection", style="Dark.TFrame")
        conn_frame.pack(fill=tk.X, pady=(0, 8))

        conn_inner = ttk.Frame(conn_frame)
        conn_inner.pack(fill=tk.X, padx=8, pady=6)

        ttk.Label(conn_inner, text="Port:", style="Dark.TLabel").pack(side=tk.LEFT, padx=(0, 4))
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(
            conn_inner,
            textvariable=self.port_var,
            width=18,
            state="readonly",
        )
        self.port_combo.pack(side=tk.LEFT, padx=(0, 8))
        ttk.Button(
            conn_inner,
            text="Refresh",
            command=self._refresh_ports,
            style="Dark.TButton",
        ).pack(side=tk.LEFT, padx=(0, 8))
        self.connect_btn = ttk.Button(
            conn_inner,
            text="Connect",
            command=self._toggle_connect,
            style="Dark.TButton",
        )
        self.connect_btn.pack(side=tk.LEFT)
        self._refresh_ports()

        # ---------- System controls ----------
        sys_frame = ttk.LabelFrame(main, text="System", style="Dark.TFrame")
        sys_frame.pack(fill=tk.X, pady=(0, 8))

        sys_grid = ttk.Frame(sys_frame)
        sys_grid.pack(fill=tk.X, padx=8, pady=6)

        ttk.Button(sys_grid, text="Sleep", command=lambda: self._send(SLEEP_CMD, _optimistic_status="Power: Sleeping"), style="Dark.TButton").grid(
            row=0, column=0, padx=2, pady=2
        )
        ttk.Button(sys_grid, text="Wake", command=lambda: self._send(WAKE_CMD, _optimistic_status="Power: Awake"), style="Dark.TButton").grid(
            row=0, column=1, padx=2, pady=2
        )
        ttk.Button(sys_grid, text="Restart", command=lambda: self._send(RESTART_CMD), style="Dark.TButton").grid(
            row=0, column=2, padx=2, pady=2
        )
        ttk.Button(sys_grid, text="Bootloader", command=lambda: self._send(BOOTLOADER_CMD), style="Dark.TButton").grid(
            row=0, column=3, padx=2, pady=2
        )
        ttk.Button(
            sys_grid,
            text="Enable/Disable",
            command=lambda: self._send(ENABLE_DISABLE_CMD),
            style="Dark.TButton",
        ).grid(row=0, column=4, padx=2, pady=2)

        # ---------- Mode ----------
        mode_frame = ttk.LabelFrame(main, text="Mode", style="Dark.TFrame")
        mode_frame.pack(fill=tk.X, pady=(0, 8))

        mode_inner = ttk.Frame(mode_frame)
        mode_inner.pack(fill=tk.X, padx=8, pady=6)
        ttk.Button(
            mode_inner,
            text="Write Mode",
            command=lambda: self._send(WRITE_MODE_CMD),
            style="Dark.TButton",
        ).pack(side=tk.LEFT, padx=(0, 8))
        ttk.Button(
            mode_inner,
            text="Project Mode",
            command=lambda: self._send(PROJECT_MODE_CMD),
            style="Dark.TButton",
        ).pack(side=tk.LEFT)

        # ---------- Playback ----------
        play_frame = ttk.LabelFrame(main, text="Playback", style="Dark.TFrame")
        play_frame.pack(fill=tk.X, pady=(0, 8))

        play_inner = ttk.Frame(play_frame)
        play_inner.pack(fill=tk.X, padx=8, pady=6)

        ttk.Label(play_inner, text="Speed (KHz):", style="Dark.TLabel").pack(side=tk.LEFT, padx=(0, 4))
        self.speed_var = tk.StringVar(value="200")
        speed_spin = ttk.Spinbox(
            play_inner,
            from_=1,
            to=200,
            textvariable=self.speed_var,
            width=6,
        )
        speed_spin.pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(
            play_inner,
            text="Set",
            command=self._set_playback_speed,
            style="Dark.TButton",
        ).pack(side=tk.LEFT, padx=(0, 16))
        ttk.Button(
            play_inner,
            text="Play/Pause",
            command=lambda: self._send(PLAY_PAUSE_CMD),
            style="Dark.TButton",
        ).pack(side=tk.LEFT)

        # ---------- Laser duty cycle ----------
        laser_frame = ttk.LabelFrame(main, text="Laser", style="Dark.TFrame")
        laser_frame.pack(fill=tk.X, pady=(0, 8))

        laser_inner = ttk.Frame(laser_frame)
        laser_inner.pack(fill=tk.X, padx=8, pady=6)
        ttk.Label(laser_inner, text="Duty cycle (%):", style="Dark.TLabel").pack(side=tk.LEFT, padx=(0, 4))
        self.duty_var = tk.StringVar(value="50")
        duty_spin = ttk.Spinbox(
            laser_inner,
            from_=0,
            to=100,
            textvariable=self.duty_var,
            width=6,
        )
        duty_spin.pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(
            laser_inner,
            text="Set",
            command=self._set_laser_duty_cycle,
            style="Dark.TButton",
        ).pack(side=tk.LEFT)

        # ---------- Upload (file) ----------
        upload_frame = ttk.LabelFrame(main, text="Upload frames", style="Dark.TFrame")
        upload_frame.pack(fill=tk.X, pady=(0, 8))

        upload_inner = ttk.Frame(upload_frame)
        upload_inner.pack(fill=tk.X, padx=8, pady=6)
        ttk.Button(
            upload_inner,
            text="Select file...",
            command=self._upload_file,
            style="Dark.TButton",
        ).pack(side=tk.LEFT, padx=(0, 8))
        self.upload_status = ttk.Label(upload_inner, text="(Select a .bin file)", style="Dark.TLabel")
        self.upload_status.pack(side=tk.LEFT)

        # ---------- Live status ----------
        live_frame = ttk.LabelFrame(main, text="Live status", style="Dark.TFrame")
        live_frame.pack(fill=tk.X, pady=(0, 8))
        self.live_status = tk.Label(
            live_frame,
            text="— Disconnected —",
            font=("Consolas", 10),
            bg="#fafafa",
            fg="#2d7a2d",
            anchor=tk.W,
            padx=8,
            pady=6,
        )
        self.live_status.pack(fill=tk.X)

        # ---------- Log ----------
        log_frame = ttk.LabelFrame(main, text="Log", style="Dark.TFrame")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 8))

        self.status_text = scrolledtext.ScrolledText(
            log_frame,
            height=6,
            font=("Consolas", 9),
            bg="#fafafa",
            fg="#2d5a2d",
            insertbackground="#2d5a2d",
            wrap=tk.WORD,
        )
        self.status_text.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)
        self.status_text.insert(tk.END, "Disconnected. Connect to a projector to see activity.\n")
        self.status_text.config(state=tk.DISABLED)

        # Bind close
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    def _refresh_ports(self):
        comports = list(serial.tools.list_ports.comports())
        display = [f"{p.device} ({p.description})" for p in comports]
        self.port_combo["values"] = display
        devices = [p.device for p in comports]
        if devices and not self.port_var.get():
            self.port_var.set(display[0] if display else devices[0])

    def _send(self, cmd: int, payload: bytes = b"", *, _quiet: bool = True, _optimistic_status: str | None = None):
        """Send a command: [1 byte cmd][2 bytes length BE][payload]."""
        if not self.serial_port or not self.serial_port.is_open:
            self._log("Not connected.", "warn")
            return
        try:
            length = len(payload)
            packet = bytes([cmd]) + struct.pack(">H", length) + payload
            self.serial_port.write(packet)
            if _optimistic_status:
                self._update_live_status(_optimistic_status)
            if not _quiet:
                self._log(f"Sent cmd 0x{cmd:02X}, len={length}")
        except serial.SerialException as e:
            self._log(f"Serial error: {e}", "err")

    def _send_with_payload(self, cmd: int, payload: bytes):
        self._send(cmd, payload)

    def _set_playback_speed(self):
        try:
            freq = int(self.speed_var.get())
            if freq < 0:
                freq = 0
            elif freq >= 200:
                freq = 200  # Full speed
            payload = struct.pack("<H", freq)
            self._send(SET_PLAYBACK_SPEED_CMD, payload)
        except ValueError:
            self._log("Invalid speed value.", "err")

    def _set_laser_duty_cycle(self):
        try:
            pct = float(self.duty_var.get())
            if pct < 0:
                pct = 0
            elif pct > 100:
                pct = 100
            # Payload: 0–100 maps to 0–1 on device (Interface divides by 100)
            duty_val = int(round(pct))
            payload = struct.pack("<H", duty_val)
            self._send(SET_LASER_DUTY_CYCLE_CMD, payload)
        except ValueError:
            self._log("Invalid duty cycle value.", "err")

    def _upload_file(self):
        path = filedialog.askopenfilename(
            title="Select projection data",
            filetypes=[("Binary data", "*.bin"), ("All files", "*.*")],
        )
        if not path:
            return
        if not self.serial_port or not self.serial_port.is_open:
            self._log("Not connected. Cannot upload.", "err")
            return
        try:
            with open(path, "rb") as f:
                data = f.read()
            if len(data) % RECORD_SIZE != 0:
                self._log(f"File size must be multiple of {RECORD_SIZE} bytes.", "err")
                return
            # Send in chunks; use multiples of RECORD_SIZE (5 bytes)
            chunk = BUFFER_SIZE
            sent = 0
            for i in range(0, len(data), chunk):
                chunk_data = data[i : i + chunk]
                self._send(UPLOAD_CMD, chunk_data)
                sent += len(chunk_data)
                time.sleep(0.01)  # Allow device to process
            self.upload_status.config(text=f"Uploaded {sent} bytes")
            self._log(f"Uploaded {sent} bytes from {path}")
        except Exception as e:
            self._log(f"Upload failed: {e}", "err")

    def _toggle_connect(self):
        if self.serial_port and self.serial_port.is_open:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("No port", "Select a serial port.")
            return
        # Extract bare port name if user selected "COM4 (USB Serial)" format
        if " " in port:
            port = port.split()[0]
        try:
            self.serial_port = serial.Serial(port=port, baudrate=BAUDRATE, timeout=0.05, write_timeout=2.0)
            self.connect_btn.config(text="Disconnect")
            self.port_combo.config(state=tk.DISABLED)
            self._update_live_status("Connecting…")
            self._log(f"Opening {port} @ {BAUDRATE}…")
            self.root.update()
            # Teensy is in while(!Serial) until we open; then runs init and sends ASCII. Wait and drain.
            time.sleep(1.0)
            drained = bytearray()
            for _ in range(20):  # Drain for up to ~1s
                n = self.serial_port.in_waiting
                if n:
                    drained.extend(self.serial_port.read(n))
                time.sleep(0.05)
            if drained:
                self._log(f"Device responded ({len(drained)} bytes). Connected to {port}.")
            else:
                self._log(f"No data from device—verify correct port. Connected to {port} @ {BAUDRATE}")
            self._update_live_status("Waiting for status…")
            self._start_status_polling()
        except serial.SerialException as e:
            messagebox.showerror("Connection failed", str(e))
            self._log(f"Connection failed: {e}", "err")

    def _disconnect(self):
        self._stop_status_polling()
        self._serial_buffer.clear()
        if self.serial_port:
            try:
                self.serial_port.close()
            except Exception:
                pass
            self.serial_port = None
        self.connect_btn.config(text="Connect")
        self.port_combo.config(state="readonly")
        self._update_live_status("— Disconnected —")
        self._log("Disconnected.")

    def _start_status_polling(self):
        self.polling = True
        self._poll_status()

    def _stop_status_polling(self):
        self.polling = False
        if self.after_id:
            self.root.after_cancel(self.after_id)
            self.after_id = None

    def _poll_status(self):
        if not self.polling: # or not self.serial_port or not self.serial_port.is_open:
            return
        self._send(SEND_STATUS_CMD, _quiet=True)
        # Give device time to respond, then read and schedule next poll
        self.after_id = self.root.after(80, self._read_status_then_poll)

    def _process_serial_buffer(self):
        """Extract binary status packets and ASCII debug lines from _serial_buffer."""
        STATUS_PACKET_SIZE = 9
        buf = self._serial_buffer
        i = 0
        while i < len(buf):
            if buf[i] == RECEIVE_STATUS_PACKET_CMD and i + STATUS_PACKET_SIZE <= len(buf):
                data = bytes(buf[i : i + STATUS_PACKET_SIZE])
                i += STATUS_PACKET_SIZE
                power = "Awake" if data[1] else "Sleeping"
                mode_str = {0: "Ready", 1: "Project", 2: "Write"}.get(data[2], "?")
                enabled = "Yes" if data[3] else "No"
                projecting = "Yes" if data[4] else "No"
                duty = data[5]
                delay_us = data[6]
                freq_khz = data[7] | (data[8] << 8) if len(data) >= 9 else data[7]
                status = (
                    f"Power: {power} | Mode: {mode_str} | Enabled: {enabled} | "
                    f"Projecting: {projecting} | Duty: {duty}% | Delay: {delay_us}µs | Freq: ~{freq_khz} KHz"
                )
                if self.root.winfo_exists() and self.live_status.winfo_exists():
                    self.root.after(0, lambda s=status: self._update_live_status(s))
            else:
                j = i
                while j < len(buf) and buf[j] not in (0x0A, 0x0D):
                    if buf[j] == RECEIVE_STATUS_PACKET_CMD:
                        break
                    j += 1
                if j < len(buf) and buf[j] in (0x0A, 0x0D):
                    line = bytes(buf[i:j]).decode("utf-8", errors="replace").strip()
                    if line:
                        self._log(f"< {line}")
                    while j < len(buf) and buf[j] in (0x0A, 0x0D):
                        j += 1
                    i = j
                else:
                    break
        self._serial_buffer = buf[i:]

    def _read_status_then_poll(self):
        self.after_id = None
        if not self.polling:#
            return
        try:
            n = self.serial_port.in_waiting
            if n:
                self._serial_buffer.extend(self.serial_port.read(n))
            self._process_serial_buffer()
        except serial.SerialException:
            pass
        if self.polling:
            self.after_id = self.root.after(STATUS_POLL_INTERVAL_MS, self._poll_status)

    def _update_live_status(self, text: str):
        if self.live_status.winfo_exists():
            self.live_status.config(text=text)

    def _log(self, msg: str, tag: str = "info"):
        def _do():
            self.status_text.config(state=tk.NORMAL)
            if tag == "err":
                self.status_text.insert(tk.END, f"[ERR] {msg}\n", "err")
            elif tag == "warn":
                self.status_text.insert(tk.END, f"[!] {msg}\n", "warn")
            else:
                self.status_text.insert(tk.END, f"{msg}\n")
            self.status_text.see(tk.END)
            self.status_text.config(state=tk.DISABLED)

        if self.root.winfo_exists():
            self.root.after(0, _do)

    def _on_close(self):
        self._disconnect()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = ProjectorUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
