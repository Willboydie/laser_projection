import json
import os
import struct
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

import serial
from serial.tools import list_ports
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DEFAULT_PORT = "COM4"
BAUD_RATE = 115200
UPLOAD_CHUNK_SIZE = 512
STATUS_PAYLOAD_LENGTH = 14

# Interface command definitions from lib/interface/Interface.hpp
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
DEBUG_MODE_CMD = 0b00011010
TOGGLE_LOOPBACK_CMD = 0b00111001
TOGGLE_PROJECT_MODE_CMD = 0b00111010
SET_POINT_CMD = 0b00111011

# Interface output packet types
RECEIVE_STATUS_PACKET_CMD = 0b11111010
LOOPBACK_DATA_CMD = 0b11111011
DEBUG_CMD = 0b11111111
DEBUG_LOG_PATH = os.path.join(PROJECT_ROOT, "debug-b42dc2.log")
DEBUG_SESSION_ID = "b42dc2"


class GalvoController:
    def __init__(self, master):
        self.master = master
        self.master.title("Galvo Controller")
        self.master.geometry("760x880")

        self.ser = None
        self.connected = False
        self.current_file = None
        self.status_poll_job = None
        self.upload_in_progress = False
        self.loopback_enabled = False

        self.port_var = tk.StringVar(value=DEFAULT_PORT)
        self.speed_var = tk.StringVar(value="100 KHz")
        self.laser_duty_var = tk.DoubleVar(value=50.0)
        self.point_x_var = tk.IntVar(value=0)
        self.point_y_var = tk.IntVar(value=0)
        self.progress_var = tk.DoubleVar(value=0.0)

        self.status_vars = {
            "power_state": tk.StringVar(value="Unknown"),
            "mode": tk.StringVar(value="Unknown"),
            "enabled": tk.StringVar(value="Unknown"),
            "projecting": tk.StringVar(value="Unknown"),
            "laser_duty": tk.StringVar(value="Unknown"),
            "projector_delay": tk.StringVar(value="Unknown"),
            "points_frequency": tk.StringVar(value="Unknown"),
            "file_size": tk.StringVar(value="Unknown"),
            "debug_mode": tk.StringVar(value="Unknown"),
            "project_mode": tk.StringVar(value="Unknown"),
        }

        self.speed_options = {
            "200 KHz": 200,
            "175 KHz": 175,
            "150 KHz": 150,
            "125 KHz": 125,
            "100 KHz": 100,
            "75 KHz": 75,
            "50 KHz": 50,
            "25 KHz": 25,
            "10 KHz": 10,
            "1 KHz": 1,
        }

        self.action_buttons = []
        self.default_loopback_bg = None

        self.setup_ui()
        self.refresh_ports()
        self.update_connection_state(False)
        self.master.protocol("WM_DELETE_WINDOW", self.on_close)

    def debug_log(self, hypothesis_id, location, message, data):
        payload = {
            "sessionId": DEBUG_SESSION_ID,
            "runId": "pre-fix",
            "hypothesisId": hypothesis_id,
            "location": location,
            "message": message,
            "data": data,
            "timestamp": int(time.time() * 1000),
        }
        try:
            with open(DEBUG_LOG_PATH, "a", encoding="utf-8") as handle:
                handle.write(json.dumps(payload) + "\n")
        except OSError:
            pass

    def setup_ui(self):
        self.master.columnconfigure(0, weight=1)
        self.master.rowconfigure(0, weight=1)

        main_frame = ttk.Frame(self.master, padding=10)
        main_frame.grid(row=0, column=0, sticky="nsew")
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(1, weight=1)

        connection_frame = ttk.LabelFrame(main_frame, text="Connection", padding=10)
        connection_frame.grid(row=0, column=0, sticky="ew")
        connection_frame.columnconfigure(1, weight=1)

        ttk.Label(connection_frame, text="Port").grid(row=0, column=0, padx=(0, 6), sticky="w")
        self.port_combo = ttk.Combobox(connection_frame, textvariable=self.port_var, state="readonly")
        self.port_combo.grid(row=0, column=1, sticky="ew")

        self.refresh_btn = ttk.Button(connection_frame, text="Refresh Ports", command=self.refresh_ports)
        self.refresh_btn.grid(row=0, column=2, padx=(6, 6), sticky="ew")

        self.connect_btn = ttk.Button(connection_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=3, sticky="ew")

        self.connection_status = ttk.Label(connection_frame, text="Disconnected", foreground="red")
        self.connection_status.grid(row=1, column=0, columnspan=4, pady=(8, 0), sticky="w")

        notebook = ttk.Notebook(main_frame)
        notebook.grid(row=1, column=0, pady=(10, 0), sticky="nsew")

        self.system_tab = ttk.Frame(notebook, padding=10)
        self.projection_tab = ttk.Frame(notebook, padding=10)
        self.status_tab = ttk.Frame(notebook, padding=10)
        self.loopback_tab = ttk.Frame(notebook, padding=10)
        notebook.add(self.system_tab, text="System")
        notebook.add(self.projection_tab, text="Projection")
        notebook.add(self.status_tab, text="Status")
        notebook.add(self.loopback_tab, text="Loopback")

        self.build_system_tab()
        self.build_projection_tab()
        self.build_status_tab()
        self.build_loopback_tab()
        self.build_log_panel(main_frame)

    def build_system_tab(self):
        self.system_tab.columnconfigure(0, weight=1)
        self.system_tab.columnconfigure(1, weight=1)

        power_frame = ttk.LabelFrame(self.system_tab, text="Power / System", padding=10)
        power_frame.grid(row=0, column=0, columnspan=2, sticky="ew")
        for column in range(3):
            power_frame.columnconfigure(column, weight=1)

        self.enable_btn = self.add_action_button(power_frame, "Enable / Disable", self.toggle_enable_disable, 0, 0)
        self.restart_btn = self.add_action_button(power_frame, "Restart", self.restart_system, 0, 1)
        self.bootloader_btn = self.add_action_button(power_frame, "Bootloader", self.enter_bootloader, 0, 2)
        self.sleep_btn = self.add_action_button(power_frame, "Sleep", self.sleep_system, 1, 0)
        self.wake_btn = self.add_action_button(power_frame, "Wake", self.wake_system, 1, 1)

        debug_frame = ttk.LabelFrame(self.system_tab, text="Diagnostics", padding=10)
        debug_frame.grid(row=1, column=0, columnspan=2, pady=(10, 0), sticky="ew")
        for column in range(2):
            debug_frame.columnconfigure(column, weight=1)

        self.debug_btn = self.add_action_button(debug_frame, "Toggle Debug", self.toggle_debug_mode, 0, 0)
        self.status_btn = self.add_action_button(debug_frame, "Request Status", self.request_status, 0, 1)

    def build_projection_tab(self):
        self.projection_tab.columnconfigure(0, weight=1)
        self.projection_tab.columnconfigure(1, weight=1)

        mode_frame = ttk.LabelFrame(self.projection_tab, text="Modes", padding=10)
        mode_frame.grid(row=0, column=0, columnspan=2, sticky="ew")
        for column in range(3):
            mode_frame.columnconfigure(column, weight=1)

        self.write_btn = self.add_action_button(mode_frame, "Enter Write Mode", self.enter_write_mode, 0, 0)
        self.project_btn = self.add_action_button(mode_frame, "Enter Project Mode", self.enter_project_mode, 0, 1)
        self.toggle_project_btn = self.add_action_button(mode_frame, "Toggle Path / Point", self.toggle_project_mode, 0, 2)

        file_frame = ttk.LabelFrame(self.projection_tab, text="File Upload", padding=10)
        file_frame.grid(row=1, column=0, columnspan=2, pady=(10, 0), sticky="ew")
        file_frame.columnconfigure(1, weight=1)

        ttk.Label(file_frame, text="File").grid(row=0, column=0, padx=(0, 6), sticky="w")
        self.file_label = ttk.Label(file_frame, text="No file selected", foreground="gray")
        self.file_label.grid(row=0, column=1, sticky="ew")

        self.browse_btn = self.add_action_button(file_frame, "Browse", self.browse_file, 1, 0)
        self.upload_btn = self.add_action_button(file_frame, "Upload", self.upload_file, 1, 1)

        self.progress_bar = ttk.Progressbar(file_frame, variable=self.progress_var, maximum=100)
        self.progress_bar.grid(row=2, column=0, columnspan=2, pady=(10, 0), sticky="ew")

        playback_frame = ttk.LabelFrame(self.projection_tab, text="Playback", padding=10)
        playback_frame.grid(row=2, column=0, pady=(10, 0), padx=(0, 5), sticky="nsew")
        playback_frame.columnconfigure(1, weight=1)

        self.loopback_btn = tk.Button(playback_frame, text="Loopback", command=self.toggle_loopback)
        self.loopback_btn.grid(row=1, column=2, columnspan=1, padx=4, pady=4, sticky="ew")
        self.default_loopback_bg = self.loopback_btn.cget("background")
        self.action_buttons.append(self.loopback_btn)
        self.update_loopback_button()

        ttk.Label(playback_frame, text="Speed").grid(row=0, column=0, padx=(0, 6), sticky="w")
        self.speed_combo = ttk.Combobox(
            playback_frame,
            textvariable=self.speed_var,
            values=list(self.speed_options.keys()),
            state="readonly",
        )
        self.speed_combo.grid(row=0, column=1, sticky="ew")

        self.set_speed_btn = self.add_action_button(playback_frame, "Set Speed", self.set_playback_speed, 1, 0)
        self.play_pause_btn = self.add_action_button(playback_frame, "Play / Pause", self.play_pause_projection, 1, 1)

        laser_frame = ttk.LabelFrame(self.projection_tab, text="Laser", padding=10)
        laser_frame.grid(row=2, column=1, pady=(10, 0), padx=(5, 0), sticky="nsew")
        laser_frame.columnconfigure(1, weight=1)

        ttk.Label(laser_frame, text="Duty Cycle (%)").grid(row=0, column=0, padx=(0, 6), sticky="w")
        self.laser_scale = ttk.Scale(laser_frame, from_=0, to=100, variable=self.laser_duty_var, orient=tk.HORIZONTAL)
        self.laser_scale.grid(row=0, column=1, sticky="ew")

        self.laser_spinbox = tk.Spinbox(
            laser_frame,
            from_=0,
            to=100,
            textvariable=self.laser_duty_var,
            width=8,
        )
        self.laser_spinbox.grid(row=1, column=0, sticky="w")
        self.set_laser_btn = self.add_action_button(laser_frame, "Set Laser Duty", self.set_laser_duty_cycle, 1, 1)

        point_frame = ttk.LabelFrame(self.projection_tab, text="Point Mode", padding=10)
        point_frame.grid(row=3, column=0, columnspan=2, pady=(10, 0), sticky="ew")
        for column in (1, 3, 4):
            point_frame.columnconfigure(column, weight=1)

        ttk.Label(point_frame, text="X").grid(row=0, column=0, padx=(0, 6), sticky="w")
        self.point_x_spinbox = tk.Spinbox(point_frame, from_=0, to=65535, textvariable=self.point_x_var, width=10)
        self.point_x_spinbox.grid(row=0, column=1, sticky="w")

        ttk.Label(point_frame, text="Y").grid(row=0, column=2, padx=(12, 6), sticky="w")
        self.point_y_spinbox = tk.Spinbox(point_frame, from_=0, to=65535, textvariable=self.point_y_var, width=10)
        self.point_y_spinbox.grid(row=0, column=3, sticky="w")

        self.set_point_btn = self.add_action_button(point_frame, "Set Point", self.set_point, 0, 4)

    def build_status_tab(self):
        self.status_tab.columnconfigure(0, weight=1)

        status_frame = ttk.LabelFrame(self.status_tab, text="Device Status", padding=10)
        status_frame.grid(row=0, column=0, sticky="ew")
        status_frame.columnconfigure(1, weight=1)
        status_frame.columnconfigure(3, weight=1)

        fields = [
            ("Power State", "power_state"),
            ("Mode", "mode"),
            ("Enabled", "enabled"),
            ("Projecting", "projecting"),
            ("Laser Duty", "laser_duty"),
            ("Delay", "projector_delay"),
            ("Points / ms", "points_frequency"),
            ("File Size", "file_size"),
            ("Debug Mode", "debug_mode"),
            ("Project Mode", "project_mode"),
        ]

        for index, (label_text, key) in enumerate(fields):
            row = index // 2
            column_offset = (index % 2) * 2
            ttk.Label(status_frame, text=label_text).grid(row=row, column=column_offset, padx=(0, 6), pady=2, sticky="w")
            ttk.Label(status_frame, textvariable=self.status_vars[key]).grid(row=row, column=column_offset + 1, pady=2, sticky="w")

    def build_loopback_tab(self):
        self.loopback_tab.columnconfigure(0, minsize=300, weight=1)
        self.loopback_tab.rowconfigure(0, weight=1)

        fig = Figure(figsize=(6, 6), dpi=100, tight_layout=True)
        self.loopback_ax = fig.add_subplot(111)
        self.loopback_ax.set_title("Loopback Capture")
        self.loopback_ax.set_xlabel("X (DAC A)")
        self.loopback_ax.set_ylabel("Y (DAC B)")
        self.loopback_ax.set_xlim(0, 65535)
        self.loopback_ax.set_ylim(0, 65535)
        self.loopback_ax.set_aspect("equal")
        self.loopback_ax.grid(True, alpha=0.3)

        self.loopback_canvas = FigureCanvasTkAgg(fig, master=self.loopback_tab)
        self.loopback_canvas.draw()
        self.loopback_canvas.get_tk_widget().grid(row=0, column=0, sticky="nsew")

        toolbar_frame = ttk.Frame(self.loopback_tab)
        toolbar_frame.grid(row=1, column=0, sticky="ew")
        NavigationToolbar2Tk(self.loopback_canvas, toolbar_frame)

        self.loopback_status = ttk.Label(self.loopback_tab, text="No data received")
        self.loopback_status.grid(row=2, column=0, pady=(5, 0), sticky="w")

    def build_log_panel(self, parent):
        log_frame = ttk.LabelFrame(parent, text="Messages", padding=0)
        log_frame.grid(row=4, column=0, pady=(0, 0), sticky="nsew")
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)

        self.message_text = tk.Text(log_frame, wrap=tk.WORD, height=20)
        self.message_text.grid(row=0, column=0, sticky="nsew")

        scrollbar = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.message_text.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.message_text.configure(yscrollcommand=scrollbar.set)

    def add_action_button(self, parent, text, command, row, column):
        button = ttk.Button(parent, text=text, command=command)
        button.grid(row=row, column=column, padx=4, pady=4, sticky="ew")
        self.action_buttons.append(button)
        return button

    def refresh_ports(self):
        ports = sorted(port.device for port in list_ports.comports())
        self.port_combo["values"] = ports

        current_port = self.port_var.get().strip()
        if current_port:
            if current_port not in ports:
                ports = [current_port] + ports
                self.port_combo["values"] = ports
        elif ports:
            self.port_var.set(ports[0])

    def toggle_connection(self):
        if self.connected:
            self.disconnect_serial()
        else:
            self.connect_serial()

    def connect_serial(self):
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("No Port Selected", "Select a serial port first.")
            return

        try:
            self.ser = serial.Serial(port, BAUD_RATE, timeout=0.1, write_timeout=1)
            time.sleep(1.5)
        except Exception as exc:
            self.log_message(f"Connection failed: {exc}")
            messagebox.showerror("Connection Error", f"Failed to connect to {port}: {exc}")
            return

        self.connected = True
        self.update_connection_state(True)
        self.log_message(f"Connected to {port} @ {BAUD_RATE} baud")

        threading.Thread(target=self.read_serial, daemon=True).start()
        self.schedule_status_poll()
        self.request_status(log_request=False)

    def disconnect_serial(self):
        self.connected = False
        self.cancel_status_poll()

        if self.ser:
            try:
                if self.ser.is_open:
                    self.ser.close()
            except Exception as exc:
                self.log_message(f"Error while closing serial port: {exc}")
            finally:
                self.ser = None

        self.update_connection_state(False)
        self.log_message("Disconnected from device")

    def update_connection_state(self, connected):
        self.connection_status.config(
            text="Connected" if connected else "Disconnected",
            foreground="green" if connected else "red",
        )
        self.connect_btn.config(text="Disconnect" if connected else "Connect")
        self.port_combo.config(state="disabled" if connected else "readonly")
        self.refresh_btn.config(state="disabled" if connected else "normal")

        for button in self.action_buttons:
            button.config(state="normal" if connected else "disabled")

        self.speed_combo.config(state="readonly" if connected else "disabled")
        self.laser_scale.config(state="normal" if connected else "disabled")
        self.laser_spinbox.config(state="normal" if connected else "disabled")
        self.point_x_spinbox.config(state="normal" if connected else "disabled")
        self.point_y_spinbox.config(state="normal" if connected else "disabled")

        self.upload_btn.config(state="normal" if connected and self.current_file and not self.upload_in_progress else "disabled")
        if not connected:
            self.loopback_enabled = False
        self.update_loopback_button()

    def schedule_status_poll(self):
        self.cancel_status_poll()
        if self.connected:
            self.status_poll_job = self.master.after(500, self.poll_status)

    def cancel_status_poll(self):
        if self.status_poll_job is not None:
            self.master.after_cancel(self.status_poll_job)
            self.status_poll_job = None

    def poll_status(self):
        self.status_poll_job = None
        if self.connected:
            self.request_status(log_request=False)
            self.schedule_status_poll()

    def log_message(self, message):
        self.master.after(0, lambda: self._log_message(message))

    def _log_message(self, message):
        timestamp = time.strftime("%H:%M:%S")
        self.message_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.message_text.see(tk.END)

    def send_command(self, cmd, payload=b""):
        if not self.connected or not self.ser:
            messagebox.showwarning("Not Connected", "Connect to the device first.")
            return False

        try:
            packet = bytes([cmd]) + len(payload).to_bytes(2, "little") + payload
            if cmd in (ENABLE_DISABLE_CMD, PROJECT_MODE_CMD, TOGGLE_LOOPBACK_CMD):
                # #region agent log
                self.debug_log(
                    "H4",
                    "tools/galvoController.py:send_command",
                    "Sending control command",
                    {
                        "cmd": cmd,
                        "payload_hex": payload.hex(),
                        "mode": self.status_vars["mode"].get(),
                        "enabled": self.status_vars["enabled"].get(),
                        "project_mode": self.status_vars["project_mode"].get(),
                    },
                )
                # #endregion
            self.ser.write(packet)
            self.ser.flush()
            return True
        except Exception as exc:
            self.log_message(f"Failed to send command 0x{cmd:02X}: {exc}")
            return False

    def send_uint16_command(self, cmd, value):
        return self.send_command(cmd, int(value).to_bytes(2, "little", signed=False))

    def browse_file(self):
        data_dir = os.path.join(PROJECT_ROOT, "data", "projections")
        initial_dir = data_dir if os.path.isdir(data_dir) else PROJECT_ROOT

        file_path = filedialog.askopenfilename(
            title="Select projection file",
            initialdir=initial_dir,
            filetypes=[("Binary files", "*.bin"), ("All files", "*.*")],
        )
        if not file_path:
            return

        self.current_file = file_path
        self.file_label.config(text=os.path.basename(file_path), foreground="black")
        self.upload_btn.config(state="normal" if self.connected and not self.upload_in_progress else "disabled")
        self.log_message(f"Selected file: {file_path}")

    def upload_file(self):
        if self.upload_in_progress:
            return
        if not self.current_file:
            messagebox.showwarning("No File Selected", "Choose a file before uploading.")
            return
        if not os.path.isfile(self.current_file):
            messagebox.showerror("File Missing", "The selected file no longer exists.")
            return

        self.upload_in_progress = True
        self.upload_btn.config(state="disabled")
        self.progress_var.set(0.0)
        threading.Thread(target=self._upload_file_worker, daemon=True).start()

    def _upload_file_worker(self):
        bytes_sent = 0
        file_size = os.path.getsize(self.current_file)
        self.log_message("Starting upload...")

        try:
            with open(self.current_file, "rb") as handle:
                while self.connected:
                    chunk = handle.read(UPLOAD_CHUNK_SIZE)
                    if not chunk:
                        break

                    if not self.send_command(UPLOAD_CMD, chunk):
                        raise RuntimeError("Upload command failed")

                    bytes_sent += len(chunk)
                    progress = 100.0 if file_size == 0 else (bytes_sent / file_size) * 100.0
                    self.master.after(0, lambda value=progress: self.progress_var.set(value))
                    time.sleep(0.002)

            self.log_message(f"Upload complete: {bytes_sent} bytes sent")
            self.request_status(log_request=False)
        except Exception as exc:
            self.log_message(f"Upload failed: {exc}")
            self.master.after(
                0,
                lambda error_message=str(exc): messagebox.showerror(
                    "Upload Error",
                    f"Failed to upload file: {error_message}",
                ),
            )
        finally:
            self.upload_in_progress = False
            self.master.after(0, lambda: self.upload_btn.config(state="normal" if self.connected and self.current_file else "disabled"))

    def toggle_enable_disable(self):
        if self.send_command(ENABLE_DISABLE_CMD):
            self.log_message("Sent enable/disable toggle")
            self.request_status(log_request=False)

    def restart_system(self):
        if self.send_command(RESTART_CMD):
            self.log_message("Sent restart command")

    def sleep_system(self):
        if self.send_command(SLEEP_CMD):
            self.log_message("Sent sleep command")

    def wake_system(self):
        if self.send_command(WAKE_CMD):
            self.log_message("Sent wake command")

    def enter_bootloader(self):
        if self.send_command(BOOTLOADER_CMD):
            self.log_message("Sent bootloader command")

    def toggle_debug_mode(self):
        if self.send_command(DEBUG_MODE_CMD):
            self.log_message("Sent debug toggle")
            self.request_status(log_request=False)

    def request_status(self, log_request=True):
        if self.send_command(SEND_STATUS_CMD) and log_request:
            self.log_message("Requested status packet")

    def enter_write_mode(self):
        if self.send_command(WRITE_MODE_CMD):
            self.log_message("Sent write mode command")
            self.request_status(log_request=False)

    def enter_project_mode(self):
        if self.send_command(PROJECT_MODE_CMD):
            self.log_message("Sent project mode command")
            self.request_status(log_request=False)

    def toggle_project_mode(self):
        if self.send_command(TOGGLE_PROJECT_MODE_CMD):
            self.log_message("Sent project mode toggle")
            self.request_status(log_request=False)

    def toggle_loopback(self):
        if self.send_command(TOGGLE_LOOPBACK_CMD):
            self.loopback_enabled = not self.loopback_enabled
            self.update_loopback_button()
            state = "enabled" if self.loopback_enabled else "disabled"
            self.log_message(f"Loopback {state}")

    def update_loopback_button(self):
        if not hasattr(self, "loopback_btn"):
            return

        if self.loopback_enabled:
            self.loopback_btn.config(
                bg="#2196F3",
                activebackground="#1976D2",
                fg="white",
                activeforeground="white",
            )
        else:
            self.loopback_btn.config(
                bg=self.default_loopback_bg,
                activebackground=self.default_loopback_bg,
                fg="black",
                activeforeground="black",
            )

    def set_playback_speed(self):
        selected_speed = self.speed_var.get()
        if selected_speed not in self.speed_options:
            messagebox.showwarning("Invalid Speed", "Select a valid playback speed.")
            return

        freq_khz = self.speed_options[selected_speed]
        if self.send_uint16_command(SET_PLAYBACK_SPEED_CMD, freq_khz):
            self.log_message(f"Set playback speed to {selected_speed}")
            self.request_status(log_request=False)

    def play_pause_projection(self):
        if self.send_command(PLAY_PAUSE_CMD):
            self.log_message("Sent play/pause toggle")
            self.request_status(log_request=False)

    def set_laser_duty_cycle(self):
        duty_cycle = max(0, min(100, int(round(self.laser_duty_var.get()))))
        self.laser_duty_var.set(duty_cycle)

        if self.send_uint16_command(SET_LASER_DUTY_CYCLE_CMD, duty_cycle):
            self.log_message(f"Set laser duty cycle to {duty_cycle}%")
            self.request_status(log_request=False)

    def set_point(self):
        try:
            x = max(0, min(65535, int(self.point_x_var.get())))
            y = max(0, min(65535, int(self.point_y_var.get())))
        except tk.TclError:
            messagebox.showwarning("Invalid Point", "Point coordinates must be integers between 0 and 65535.")
            return

        self.point_x_var.set(x)
        self.point_y_var.set(y)

        payload = x.to_bytes(2, "little", signed=False) + y.to_bytes(2, "little", signed=False)
        if self.send_command(SET_POINT_CMD, payload):
            self.log_message(f"Set point to ({x}, {y})")

    def read_serial(self):
        while self.connected and self.ser and self.ser.is_open:
            try:
                first_byte = self.ser.read(1)
                if not first_byte:
                    continue

                packet_type = first_byte[0]

                if packet_type == RECEIVE_STATUS_PACKET_CMD:
                    self.handle_status_packet()
                elif packet_type == LOOPBACK_DATA_CMD:
                    self.handle_loopback_packet()
                elif packet_type == DEBUG_CMD:
                    self.handle_debug_packet()
                else:
                    self.handle_text_packet(first_byte)
            except Exception as exc:
                self.log_message(f"Serial read error: {exc}")
                break

        self.connected = False
        self.master.after(0, self.handle_disconnection)

    def read_exact(self, size):
        buffer = bytearray()
        while len(buffer) < size and self.connected and self.ser and self.ser.is_open:
            chunk = self.ser.read(size - len(buffer))
            if not chunk:
                continue
            buffer.extend(chunk)
        return bytes(buffer) if len(buffer) == size else None

    def handle_status_packet(self):
        header = self.read_exact(2)
        if not header:
            return

        payload_length = int.from_bytes(header, "little")
        payload = self.read_exact(payload_length)
        if not payload or len(payload) != payload_length:
            return

        if payload_length != STATUS_PAYLOAD_LENGTH:
            self.log_message(f"Unexpected status payload length: {payload_length}")
            return

        self.master.after(0, lambda data=payload: self.update_status_display(data))

    def handle_debug_packet(self):
        if not self.read_exact(2):
            return

        line = self.ser.readline()
        if not line:
            return

        message = line.decode("utf-8", errors="replace").strip()
        if message:
            if "LOOPBACK:" in message or "SD ERROR" in message or "File opened for read" in message:
                # #region agent log
                self.debug_log(
                    "H1",
                    "tools/galvoController.py:handle_debug_packet",
                    "Firmware debug line",
                    {"message": message},
                )
                # #endregion
            self.log_message(f"Teensy: {message}")

    def handle_text_packet(self, first_byte):
        remainder = self.ser.readline()
        raw = first_byte + remainder
        message = raw.decode("utf-8", errors="ignore").strip()
        if message:
            self.log_message(f"Teensy: {message}")

    def handle_loopback_packet(self):
        count_bytes = self.read_exact(4)
        if not count_bytes:
            self.log_message("Loopback: failed to read point count")
            return

        total_points = int.from_bytes(count_bytes, "little")
        self.log_message(f"Receiving loopback data: {total_points} points...")

        total_bytes = total_points * 4
        data = self.read_exact(total_bytes)
        if not data:
            self.log_message("Loopback: failed to read point data")
            return

        x_vals = []
        y_vals = []
        for i in range(total_points):
            offset = i * 4
            x, y = struct.unpack(">HH", data[offset : offset + 4])
            x_vals.append(x)
            y_vals.append(y)

        self.log_message(f"Loopback data received: {total_points} points")
        self.master.after(0, lambda: self.plot_loopback(x_vals, y_vals))

    def plot_loopback(self, x_vals, y_vals):
        self.loopback_ax.clear()
        self.loopback_ax.plot(x_vals, y_vals, "o", linewidth=0.5, color="#2196F3")
        self.loopback_ax.set_title("Loopback Capture")
        self.loopback_ax.set_xlabel("X (DAC A)")
        self.loopback_ax.set_ylabel("Y (DAC B)")
        self.loopback_ax.set_xlim(0, 65535)
        self.loopback_ax.set_ylim(0, 65535)
        self.loopback_ax.set_aspect("equal")
        self.loopback_ax.grid(True, alpha=0.3)
        self.loopback_canvas.draw()
        self.loopback_status.config(text=f"{len(x_vals)} points plotted")

    def update_status_display(self, payload):
        mode_lookup = {
            0: "Ready",
            1: "Project",
            2: "Write",
        }
        project_mode_lookup = {
            0: "Path",
            1: "Point",
        }

        points_frequency = int.from_bytes(payload[6:8], "little")
        file_size = int.from_bytes(payload[8:12], "little")

        values = {
            "power_state": "Awake" if payload[0] else "Sleeping",
            "mode": mode_lookup.get(payload[1], f"Unknown ({payload[1]})"),
            "enabled": "Yes" if payload[2] else "No",
            "projecting": "Yes" if payload[3] else "No",
            "laser_duty": f"{payload[4]}%",
            "projector_delay": f"{payload[5]} us",
            "points_frequency": str(points_frequency),
            "file_size": f"{file_size} bytes",
            "debug_mode": "On" if payload[12] else "Off",
            "project_mode": project_mode_lookup.get(payload[13], f"Unknown ({payload[13]})"),
        }

        for key, value in values.items():
            self.status_vars[key].set(value)

        # #region agent log
        self.debug_log(
            "H2",
            "tools/galvoController.py:update_status_display",
            "Status packet received",
            values,
        )
        # #endregion

    def handle_disconnection(self):
        was_connected = self.connection_status.cget("text") == "Connected"
        self.cancel_status_poll()
        self.update_connection_state(False)
        if was_connected:
            self.log_message("Serial connection closed")

    def on_close(self):
        if self.connected:
            self.disconnect_serial()
        self.master.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = GalvoController(root)
    root.mainloop()
