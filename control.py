import serial
import time

# Configuration
SERIAL_PORT = 'COM4'  # Change to your Teensy's port
BAUD_RATE = 115200

# Command Definitions (4-bit)
WRITE_CMD   = 0b0001
PROJECT_CMD = 0b0010
SAVE_CMD    = 0b0100
PLAY_CMD    = 0b1001
PAUSE_CMD   = 0b1010

def send_command(ser, cmd, payload=0):
    """Send a 36-bit command (4-bit cmd + 32-bit payload)."""
    data = bytes([
        cmd & 0x0F,                          # Command (4 bits)
        (payload >> 24) & 0xFF,              # Payload (MSB first)
        (payload >> 16) & 0xFF,
        (payload >> 8) & 0xFF,
        payload & 0xFF
    ])
    ser.write(data)
    # print(f"Sent: CMD=0b{cmd:04b}, Payload=0x{payload:08X}")


def main():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)  # Wait for Teensy to initialize

    try:
        while True:
            print("\n--- MENU ---")
            print("1. Switch to WRITE mode")
            print("2. Switch to PROJECT mode")
            print("3. Save data (from data.bin)")
            print("4. Play (with delay)")
            print("5. Pause/Resume")
            print("6. Exit")
            choice = input("Select an option: ")
            print("\n")

            if choice == '1':
                send_command(ser, WRITE_CMD)
                time.sleep(0.5) # wait for teensy response
            elif choice == '2':
                send_command(ser, PROJECT_CMD)
                time.sleep(0.5) # wait for teensy response
            elif choice == '3':
                print("Uploading bin file...")
                with open('data/circle.bin', 'rb') as f:
                    data = f.read()
                    for i in range(0, len(data), 4):
                        valA = int.from_bytes(data[i:i+2], 'big')
                        valB = int.from_bytes(data[i+2:i+4], 'big')
                        payload = (valA << 16) | valB
                        send_command(ser, SAVE_CMD, payload)
                    
            elif choice == '4':
                delay_bit = int(input("Enter delay bit (0-31, LSB=bit31): "))
                payload = 1 << (31 - delay_bit)  # Set the specified bit
                send_command(ser, PLAY_CMD, payload)
                time.sleep(0.5) # wait for teensy response
            elif choice == '5':
                send_command(ser, PAUSE_CMD)
                time.sleep(0.5) # wait for teensy response
            elif choice == '6':
                break

            # Read Teensy responses
            while ser.in_waiting:
                line = ser.readline().decode().strip()
                print(f"Teensy: {line}")

    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        ser.close()

if __name__ == "__main__":
    main()