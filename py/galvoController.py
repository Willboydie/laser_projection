import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import serial
import time
import threading
import os

# Configuration
SERIAL_PORT = 'COM4'  # Change to your Teensy's port
BAUD_RATE = 1152000
RECORD_SIZE = 5 # uint16_t, uint16_t, uint8_t
UPLOAD_CHUNK_SIZE = 512

# Command Definitions (8-bit) - must match main.cpp
WRITE_CMD   = 0b00000001
PROJECT_CMD = 0b00000010
SAVE_CMD    = 0b00000100
PLAY_CMD    = 0b00001001
PAUSE_CMD   = 0b00001010
RESET_CMD   = 0b00001111

class GalvoController:
    def __init__(self, master):
        self.master = master
        master.title("Galvo Controller")
        master.geometry("500x600")
        
        self.ser = None
        self.connected = False
        self.current_file = None
        
        self.setup_ui()
        
    def setup_ui(self):
        # Main frame with padding
        main_frame = ttk.Frame(self.master, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.master.columnconfigure(0, weight=1)
        self.master.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        
        # Connection section
        ttk.Label(main_frame, text="Connection:", font=('Arial', 12, 'bold')).grid(row=0, column=0, columnspan=2, sticky=tk.W, pady=(0, 5))
        
        self.connect_btn = ttk.Button(main_frame, text="Connect to Teensy", command=self.connect_serial)
        self.connect_btn.grid(row=1, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))
        
        self.status_label = ttk.Label(main_frame, text="Disconnected", foreground="red")
        self.status_label.grid(row=2, column=0, columnspan=2, pady=(0, 10))
        
        # Control section
        ttk.Label(main_frame, text="Control:", font=('Arial', 12, 'bold')).grid(row=3, column=0, columnspan=2, sticky=tk.W, pady=(10, 5))
        
        # Mode buttons frame
        mode_frame = ttk.Frame(main_frame)
        mode_frame.grid(row=4, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))
        mode_frame.columnconfigure(0, weight=1)
        mode_frame.columnconfigure(1, weight=1)
        
        self.reset_btn = ttk.Button(mode_frame, text="RESET", command=self.reset, state='disabled')
        self.reset_btn.grid(row=0, column=0, padx=(0, 5), sticky=(tk.W, tk.E))
        
        self.write_btn = ttk.Button(mode_frame, text="WRITE Mode", command=self.write_mode, state='disabled')
        self.write_btn.grid(row=0, column=1, padx=(5, 0), sticky=(tk.W, tk.E))
        
        self.project_btn = ttk.Button(mode_frame, text="PROJECT Mode", command=self.project_mode, state='disabled')
        self.project_btn.grid(row=1, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))
        
        # File section
        ttk.Label(main_frame, text="File Operations:", font=('Arial', 12, 'bold')).grid(row=5, column=0, columnspan=2, sticky=tk.W, pady=(10, 5))
        
        file_frame = ttk.Frame(main_frame)
        file_frame.grid(row=6, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))
        file_frame.columnconfigure(1, weight=1)
        
        ttk.Label(file_frame, text="File:").grid(row=0, column=0, sticky=tk.W, padx=(0, 5))
        self.file_label = ttk.Label(file_frame, text="No file selected", foreground="gray")
        self.file_label.grid(row=0, column=1, sticky=(tk.W, tk.E))
        
        self.browse_btn = ttk.Button(file_frame, text="Browse", command=self.browse_file, state='disabled')
        self.browse_btn.grid(row=1, column=0, pady=5, sticky=tk.W)
        
        self.upload_btn = ttk.Button(file_frame, text="Upload File", command=self.upload_file, state='disabled')
        self.upload_btn.grid(row=1, column=1, pady=5, sticky=(tk.W, tk.E))
        
        # Playback section
        ttk.Label(main_frame, text="Playback:", font=('Arial', 12, 'bold')).grid(row=7, column=0, columnspan=2, sticky=tk.W, pady=(10, 5))
        
        playback_frame = ttk.Frame(main_frame)
        playback_frame.grid(row=8, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))
        playback_frame.columnconfigure(1, weight=1)
        
        ttk.Label(playback_frame, text="Speed:").grid(row=0, column=0, sticky=tk.W, padx=(0, 5))
        
        # Define speed options with proper bit positions
        self.speed_options = {
            "200KHz": 200,
            "175KHz": 175,
            "150KHz": 150,
            "125KHz": 125,
            "100KHz": 100,
            "75KHz": 75,
            "50KHz": 50,
            "25KHz": 25,
            "10KHz": 10,
            "1KHz": 1,
            "DEBUG (10Hz)": 0
        }
        
        self.speed_var = tk.StringVar(value="100KHz")
        self.speed_combo = ttk.Combobox(playback_frame, textvariable=self.speed_var, 
                                       values=list(self.speed_options.keys()), 
                                       state='readonly', width=20)
        self.speed_combo.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(5, 0))
        self.speed_combo.config(state='disabled')
        
        # Playback buttons
        btn_frame = ttk.Frame(playback_frame)
        btn_frame.grid(row=1, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))
        btn_frame.columnconfigure(0, weight=1)
        btn_frame.columnconfigure(1, weight=1)
        
        self.play_btn = ttk.Button(btn_frame, text="Play", command=self.play_command, state='disabled')
        self.play_btn.grid(row=0, column=0, padx=(0, 5), sticky=(tk.W, tk.E))
        
        self.pause_btn = ttk.Button(btn_frame, text="Pause/Resume", command=self.pause_command, state='disabled')
        self.pause_btn.grid(row=0, column=1, padx=(5, 0), sticky=(tk.W, tk.E))
        
        # Progress bar
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(main_frame, variable=self.progress_var, maximum=100)
        self.progress_bar.grid(row=9, column=0, columnspan=2, pady=10, sticky=(tk.W, tk.E))
        
        # Message display
        ttk.Label(main_frame, text="Messages:", font=('Arial', 12, 'bold')).grid(row=10, column=0, columnspan=2, sticky=tk.W, pady=(10, 5))
        
        # Create a frame for the text widget and scrollbar
        text_frame = ttk.Frame(main_frame)
        text_frame.grid(row=11, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        text_frame.columnconfigure(0, weight=1)
        text_frame.rowconfigure(0, weight=1)
        main_frame.rowconfigure(11, weight=1)
        
        self.message_text = tk.Text(text_frame, height=8, width=60, wrap=tk.WORD)
        scrollbar = ttk.Scrollbar(text_frame, orient=tk.VERTICAL, command=self.message_text.yview)
        self.message_text.configure(yscrollcommand=scrollbar.set)
        
        self.message_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))

    def log_message(self, message):
        """Thread-safe message logging"""
        self.master.after(0, lambda: self._log_message(message))
    
    def _log_message(self, message):
        """Internal message logging (called on main thread)"""
        timestamp = time.strftime("%H:%M:%S")
        self.message_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.message_text.see(tk.END)
        self.master.update_idletasks()

    def connect_serial(self):
        """Connect to Teensy via serial"""
        try:
            self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            time.sleep(2)  # Wait for Teensy to initialize
            
            self.connected = True
            self.status_label.config(text="Connected", foreground="green")
            self.connect_btn.config(text="Disconnect from Teensy", command=self.disconnect_serial, state='normal')
            
            # Enable control buttons
            self.reset_btn.config(state='normal')
            self.write_btn.config(state='normal')
            self.project_btn.config(state='normal')
            self.browse_btn.config(state='normal')
            
            self.log_message("Connected to Teensy successfully")
            
            # Start serial reading thread
            threading.Thread(target=self.read_serial, daemon=True).start()
            
        except Exception as e:
            self.log_message(f"Connection failed: {e}")
            messagebox.showerror("Connection Error", f"Failed to connect: {e}")

    def disconnect_serial(self):
        """Disconnect from Teensy"""
        try:
            self.connected = False
            
            if self.ser and self.ser.is_open:
                self.ser.close()
                self.log_message("Disconnected from Teensy")
            
            self.status_label.config(text="Disconnected", foreground="red")
            self.connect_btn.config(text="Connect to Teensy", command=self.connect_serial, state='normal')
            
            # Disable control buttons
            self.reset_btn.config(state='disabled')
            self.write_btn.config(state='disabled')
            self.project_btn.config(state='disabled')
            self.browse_btn.config(state='disabled')
            self.upload_btn.config(state='disabled')
            self.play_btn.config(state='disabled')
            self.pause_btn.config(state='disabled')
            self.speed_combo.config(state='disabled')
            
        except Exception as e:
            self.log_message(f"Disconnection error: {e}")
            messagebox.showerror("Disconnection Error", f"Failed to disconnect: {e}")

    def send_command(self, cmd, length, payload_bytes=None):
        """
        Send a 6-byte packet to Teensy
        cmd: 1 byte
        payload_bytes: 5 bytes (as bytes object or integer)
        """
        if not self.connected or not self.ser:
            messagebox.showwarning("Not Connected", "Please connect to Teensy first.")
            return False

        try:
            if length == 0 or payload_bytes is None:
                data = bytes([cmd]) + length.to_bytes(2, 'big')
            else:
                data = bytes([cmd]) + length.to_bytes(2, 'big') + payload_bytes
            
            self.ser.write(data)
            self.ser.flush()
            return True
        except Exception as e:
            self.log_message(f"Failed to send command: {e}")
            return False

    def write_mode(self):
        """Switch to WRITE mode"""
        if self.send_command(WRITE_CMD, 0, None):
            self.log_message("Switched to WRITE mode")
            self.upload_btn.config(state='normal')
            self.play_btn.config(state='disabled')
            self.pause_btn.config(state='disabled')
            self.speed_combo.config(state='disabled')

    def project_mode(self):
        """Switch to PROJECT mode"""
        if self.send_command(PROJECT_CMD, 0, None):
            self.log_message("Switched to PROJECT mode")
            self.upload_btn.config(state='disabled')
            self.play_btn.config(state='normal')
            self.pause_btn.config(state='normal')
            self.speed_combo.config(state='readonly')

    def reset(self):
        """Reset Teensy"""
        if self.send_command(RESET_CMD, 0, None):
            self.log_message("Reset command sent")
            time.sleep(2)  # Wait for reset to complete

    def browse_file(self):
        """Open file dialog to select .bin file"""
        data_dir = os.path.join("../data/projections")
        
        # Check if data directory exists
        if not os.path.exists(data_dir):
            self.log_message("Error: 'data' folder not found in current directory")
            return
        
        file_path = filedialog.askopenfilename(
            title="Select .bin file",
            initialdir=data_dir,
            filetypes=[("Binary files", "*.bin"), ("All files", "*.*")]
        )
        if file_path:
            self.current_file = file_path
            filename = os.path.basename(file_path)
            self.file_label.config(text=filename, foreground="black")
            self.log_message(f"Selected file: {filename}")

    def upload_file(self):
        """Upload selected file to Teensy"""
        if not self.current_file:
            messagebox.showwarning("No File", "Please select a file first.")
            return
            
        if not os.path.exists(self.current_file):
            messagebox.showerror("File Error", "Selected file does not exist.")
            return

        try:
            self.log_message("Starting file upload...")
            self.progress_var.set(0)
            
            file_size = os.path.getsize(self.current_file)
            bytes_sent = 0
            
            with open(self.current_file, 'rb') as f:
                 while True:
                    # Read UPLOAD_CHUNK_SIZE bytes at a time
                    data = f.read(UPLOAD_CHUNK_SIZE)
                    if not data:
                        break
                     
                    # Send SAVE_CMD + UPLOAD_CHUNK_SIZE bytes of data
                    length = len(data)
                    packet = bytes([SAVE_CMD]) + length.to_bytes(2, 'big') + data
                    try:
                        self.ser.write(packet)
                        self.ser.flush()  # Ensure data is sent immediately
                        
                        bytes_sent += length
                        progress = (bytes_sent / file_size) * 100
                        self.progress_var.set(progress)
                        self.master.update_idletasks()
                        
                        # Small delay to prevent overwhelming the serial buffer
                        time.sleep(0.00001)
                    except Exception as e:
                        self.log_message(f"Error sending packet: {e}")
                        raise
            
            self.progress_var.set(100)
            self.log_message(f"File upload completed: {bytes_sent} bytes sent")
            
        except Exception as e:
            self.log_message(f"Upload failed: {e}")
            messagebox.showerror("Upload Error", f"Failed to upload file: {e}")

    def play_command(self):
        """Start playback with selected speed"""
        selected_speed = self.speed_var.get()
        if selected_speed not in self.speed_options:
            messagebox.showwarning("Invalid Speed", "Please select a valid speed.")
            return
            
        freq_KHz = self.speed_options[selected_speed]
        # Pack freq_KHz as 16-bit: lower 8 bits in first byte, upper 8 bits in second byte
        freq_lower = freq_KHz & 0xFF  # Lower 8 bits
        freq_upper = (freq_KHz >> 8) & 0xFF  # Upper 8 bits
        payload_bytes = bytes([freq_lower, freq_upper])

        length = len(payload_bytes)
        if self.send_command(PLAY_CMD, length, payload_bytes):
            self.log_message(f"Started playback at {selected_speed}")

    def pause_command(self):
        """Pause/resume playback"""
        if self.send_command(PAUSE_CMD, 0, None):
            self.log_message("Pause/Resume command sent")

    def read_serial(self):
        """Read serial messages from Teensy"""
        while self.connected and self.ser and self.ser.is_open:
            try:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        self.log_message(f"Teensy: {line}")
                else:
                    time.sleep(0.01)  # Reduced polling frequency
            except Exception as e:
                self.log_message(f"Serial read error: {e}")
                break
        
        # Connection lost
        self.connected = False
        self.master.after(0, self.handle_disconnection)
    
    def handle_disconnection(self):
        """Handle disconnection from Teensy"""
        self.status_label.config(text="Disconnected", foreground="red")
        self.connect_btn.config(text="Connect to Teensy", command=self.connect_serial, state='normal')
        
        # Disable all buttons
        for btn in [self.reset_btn, self.write_btn, self.project_btn, 
                   self.browse_btn, self.upload_btn, self.play_btn, self.pause_btn]:
            btn.config(state='disabled')
        
        self.speed_combo.config(state='disabled')
        self.log_message("Disconnected from Teensy")

if __name__ == "__main__":
    root = tk.Tk()
    app = GalvoController(root)
    root.mainloop()
