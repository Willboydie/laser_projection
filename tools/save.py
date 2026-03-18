import numpy as np

DAC_MAX_BIN = 65535
DAC_MAX_VOLTAGE = 10.0

def voltage_to_Ybinary(voltage):
    return ((voltage + 9.990) / 0.0003048).astype(np.uint16)

def voltage_to_Xbinary(voltage):
    return ((voltage + 9.934) / 0.0003055).astype(np.uint16)

def galvo_angles_to_voltage(galvo_angles, max_angle, max_volts):
    # Expect galvo_angles shape (N, 3): [x_angle, y_angle, laser_on]
    if galvo_angles is None or np.size(galvo_angles) == 0:
        raise ValueError("galvo_angles is None or empty")

    arr = np.asarray(galvo_angles, dtype=np.float64)
    if arr.ndim != 2 or arr.shape[1] != 3:
        raise ValueError("galvo_angles must have shape (N, 3): [x_angle, y_angle, laser_on]")

    x_angle = arr[:, 0]
    y_angle = arr[:, 1]
    laser_on = arr[:, 2]


    # Scale from [-max_angle, max_angle] to [-max_volts, max_volts]
    scale = max_volts / (2.0 * float(max_angle))  # Correct scale calculation
    x_angle_volts = x_angle * scale
    y_angle_volts = y_angle * scale

    return np.array([x_angle_volts, y_angle_volts, laser_on]).T


def toBinCmd(galvo_angles, max_angle, max_volts):
    
    x_angle_volts, y_angle_volts, laser_on = galvo_angles_to_voltage(galvo_angles, max_angle, max_volts).T

    bin_X = voltage_to_Xbinary(x_angle_volts)
    bin_Y = voltage_to_Ybinary(y_angle_volts)

    # Laser flag to uint8 0/1
    laser_u8 = (laser_on != 0).astype(np.uint8)

    # Pack as 5-byte records: [x_angle (be u2), y_angle (be u2), laser_on (u1)]
    n = bin_X.shape[0]
    rec = np.empty(n, dtype=[('x_angle', '>u2'), ('y_angle', '>u2'), ('laser', 'u1')])
    rec['x_angle'] = bin_X
    rec['y_angle'] = bin_Y
    rec['laser'] = laser_u8
    return rec.tobytes()


def createBinFile(galvo_angles_data, max_angle=0.47, max_volts=12):
    bin_data = toBinCmd(galvo_angles_data, max_angle, max_volts)
    return bin_data

def saveArray(array, filename):
    """
    Save a numpy array to data/arrays directory
    
    Args:
        array: numpy array to save
        filename: name of the file (without path, .npy extension will be added if not present)
    
    Returns:
        str: full path to the saved file
    """
    import os
    
    # Add .npy extension if not present
    if not filename.endswith('.npy'):
        filename += '.npy'
    
    # Construct full path to data/arrays
    base_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    arrays_dir = os.path.join(base_dir, 'data', 'arrays')
    
    # Create directory if it doesn't exist
    os.makedirs(arrays_dir, exist_ok=True)
    
    # Full file path
    file_path = os.path.join(arrays_dir, filename)
    
    # Save the array
    np.save(file_path, array)
    
    return file_path