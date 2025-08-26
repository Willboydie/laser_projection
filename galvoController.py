import tkinter as tk
from tkinter import filedialog, messagebox
import serial
import time
import threading

# Configuration
SERIAL_PORT = 'COM4'  # Change to your Teensy's port
BAUD_RATE = 115200

# Command Definitions (4-bit)
WRITE_CMD   = 0b0001
PROJECT_CMD = 0b0010
SAVE_CMD    = 0b0100
PLAY_CMD    = 0b1001
PAUSE_CMD   = 0b1010
RESET_CMD   = 0b1111

class GalvoController:
    def __init__(self, master):
        self.master = master
        master.title("Galvo Controller")

        self.ser = None

        # Serial connection button
        self.connect_btn = tk.Button(master, text="Connect to Teensy", command=self.connect_serial)
        self.connect_btn.pack(pady=5)

        self.reset_btn = tk.Button(master, text="RESET", command=self.reset, state='disabled')
        self.reset_btn.pack(pady=5)

        # Mode buttons
        self.write_btn = tk.Button(master, text="WRITE Mode", command=self.write_mode, state='disabled')
        self.write_btn.pack(pady=5)

        self.project_btn = tk.Button(master, text="PROJECT Mode", command=self.project_mode, state='disabled')
        self.project_btn.pack(pady=5)

        self.save_btn = tk.Button(master, text="Upload .bin file", command=self.save_data, state='disabled')
        self.save_btn.pack(pady=5)

        # Play section with dropdown for delay bit
        play_frame = tk.Frame(master)
        play_frame.pack(pady=5)

        delay_label = tk.Label(play_frame, text="Points Per Second:")
        delay_label.pack(side='left', padx=5)

        # Define label-to-value mapping
        self.delay_options = {
            "Full Speed (250KHz)": 0,
            "70KHz": 1,
            "10KHz": 2,
            "1KHz": 3,
            "0.1KHz": 4
        }

        # Tkinter StringVar
        self.delay_bit_var = tk.StringVar(value="Full Speed (250KHz)")

        self.delay_dropdown = tk.OptionMenu(play_frame, self.delay_bit_var, *self.delay_options.keys())
        self.delay_dropdown.config(width=16, state='disabled')
        self.delay_dropdown.pack(side='left')

        self.play_btn = tk.Button(play_frame, text="Play", command=self.play_command, state='disabled')
        self.play_btn.pack(side='left', padx=5)

        self.pause_btn = tk.Button(master, text="Pause/Resume", command=self.pause_command, state='disabled')
        self.pause_btn.pack(pady=5)

        # Message display
        self.message_text = tk.Text(master, height=10, width=50)
        self.message_text.pack(pady=10)

    def connect_serial(self):
        try:
            self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            time.sleep(2)  # Wait for Teensy to initialize
            self.message_text.insert(tk.END, "Connected to Teensy.\n")
            self.connect_btn.config(state='disabled')
            self.enable_mode_buttons()
            threading.Thread(target=self.read_serial, daemon=True).start()
        except Exception as e:
            messagebox.showerror("Connection Error", f"Failed to connect: {e}")

    def enable_mode_buttons(self):
        self.write_btn.config(state='normal')
        self.project_btn.config(state='normal')
        self.reset_btn.config(state='normal')

    def enable_write_buttons(self):
        self.write_btn.config(state='normal')
        self.save_btn.config(state='normal')
        self.project_btn.config(state='normal')
        self.play_btn.config(state='disabled')
        self.pause_btn.config(state='disabled')
        self.delay_dropdown.config(state='disabled')

    def enable_project_buttons(self):
        self.write_btn.config(state='normal')
        self.save_btn.config(state='disabled')
        self.project_btn.config(state='normal')
        self.play_btn.config(state='normal')
        self.pause_btn.config(state='normal')
        self.delay_dropdown.config(state='normal')

    def send_command(self, cmd, payload=0):
        if not self.ser:
            messagebox.showwarning("Not Connected", "Please connect to Teensy first.")
            return

        data = bytes([
            ((cmd & 0x0F) << 4) | ((payload >> 32) & 0x0F),  # top nibble cmd, bottom nibble payload
            (payload >> 24) & 0xFF,
            (payload >> 16) & 0xFF,
            (payload >> 8)  & 0xFF,
            payload & 0xFF
        ])
        self.ser.write(data)

    def write_mode(self):
        self.send_command(WRITE_CMD)
        self.enable_write_buttons()
        time.sleep(0.2)

    def project_mode(self):
        self.send_command(PROJECT_CMD)
        self.enable_project_buttons()
        time.sleep(0.2)

    def reset(self):
        self.send_command(RESET_CMD)
        self.enable_mode_buttons()
        time.sleep(2)

    def save_data(self):
        file_path = "data/path.bin"
        if not file_path:
            return

        try:
            self.message_text.insert(tk.END, "Uploading file...\n")
            self.message_text.update_idletasks()

            with open(file_path, 'rb') as f:
                data = f.read()
                
                for i in range(0, len(data), 5):
                    valA = int.from_bytes(data[i:i+2], 'big')  # 16 bits
                    valB = int.from_bytes(data[i+2:i+4], 'big')  # 16 bits
                    laser_on = bool(data[i+4])  # single byte → bool
                    payload = (valA << 17) | (valB << 1) | laser_on
                    self.send_command(SAVE_CMD, payload)
            self.message_text.insert(tk.END, "File uploaded successfully.\n")
        except Exception as e:
            messagebox.showerror("File Error", f"Failed to read file: {e}")

    def play_command(self):
        selected_label = self.delay_bit_var.get()
        delay_bit = self.delay_options[selected_label]
        payload = 1 << (32 - delay_bit)
        self.send_command(PLAY_CMD, payload)
        time.sleep(0.2)

    def pause_command(self):
        self.send_command(PAUSE_CMD)
        time.sleep(0.2)

    def read_serial(self):
        while self.ser and self.ser.is_open:
            if self.ser.in_waiting:
                try:
                    line = self.ser.readline().decode().strip()
                    if line:
                        self.message_text.insert(tk.END, f"Teensy: {line}\n")
                        self.message_text.see(tk.END)
                except Exception:
                    pass
            time.sleep(0.05)

if __name__ == "__main__":
    root = tk.Tk()
    app = GalvoController(root)
    root.mainloop()
