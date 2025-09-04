import numpy as np
import struct

def convert_to_bin(voltage):
    """
    Convert voltage from -7.5V to +7.5V range to 16-bit DAC value (0 to 65535)
    -7.5V → 0
    0V → 32768 (middle)
    +7.5V → 65535
    """
    # Map voltage range [-7.5, 7.5] to [0, 65535]
    # Formula: (voltage + 7.5) / 15.0 * 65535
    dac_value = int((voltage + 7.5) / 15.0 * 65535)
    # Clamp to valid 16-bit range
    dac_value = max(0, min(65535, dac_value))
    return dac_value

def upload_v_test(minV: float, maxV: float, ax: int, bin_inc: float):
    # Convert voltage range to binary values
    min_dac = convert_to_bin(minV)
    max_dac = convert_to_bin(maxV)
    
    
    print(f"Debug: minV={minV}V → DAC={min_dac}")
    print(f"Debug: maxV={maxV}V → DAC={max_dac}")
    
    V_path_bin = np.arange(min_dac, max_dac, bin_inc, dtype=np.uint16)
    
    if ax==0:
        path = np.column_stack((V_path_bin, np.full_like(V_path_bin, 32768), np.ones_like(V_path_bin)))
    elif ax==1:
        path = np.column_stack((np.full_like(V_path_bin, 32768), V_path_bin, np.ones_like(V_path_bin)))

    # Add the way back - reverse the path to return to starting point
    if len(path) > 0:
        # Create return path (excluding the last point to avoid duplication)
        return_path = np.flip(path[:-1], axis=0)
        # Combine forward and return paths
        complete_path = np.vstack((path, return_path))
        return complete_path
    
    return path


if __name__ == "__main__":
    minV = -7.5
    maxV = 0.0
    inc = 10
    ax = 0
    path = upload_v_test(minV, maxV, ax, inc)
    
    with open('data/voltage_test.bin', 'wb') as f:
        for point in path:
            # Pack each point as 5 bytes: first 16 bits, second 16 bits, last 8 bits
            # Using '>' for big-endian, 'H' for unsigned short (16-bit), 'B' for unsigned char (8-bit)
            packed_data = struct.pack('>HHB', point[0], point[1], point[2])
            f.write(packed_data)
    
    print("Data saved to data/voltage_test.bin")
    print(f"Each point is now saved as 5 bytes (first 16 bits, second 16 bits, last 8 bits)")
    print(f"Total points: {len(path)}")
    print(f"Total file size: {len(path) * 5} bytes")
    
    # Print some conversion examples for verification
    print(f"\nVoltage to DAC conversion examples:")
    print(f"0.0V → {convert_to_bin(0.0)} (should be 32768)")
    print(f"2.0V → {convert_to_bin(2.0)} (should be 43690)")
    print(f"-7.5V → {convert_to_bin(-7.5)} (should be 0)")
    print(f"+7.5V → {convert_to_bin(7.5)} (should be 65535)")