import numpy as np

DAC_MAX_BIN = 65535
DAC_MAX_VOLTAGE = 10.0

def voltage_to_binary(voltage):
    return (((voltage + DAC_MAX_VOLTAGE) / (2.0 * DAC_MAX_VOLTAGE)) * DAC_MAX_BIN).astype(np.uint16)

def toBinCmd(spherical, max_angle, max_volts):
    # Expect spherical shape (N, 3): [theta, phi, laser_on]
    if spherical is None or np.size(spherical) == 0:
        return b""

    arr = np.asarray(spherical, dtype=np.float64)
    if arr.ndim != 2 or arr.shape[1] != 3:
        raise ValueError("spherical must have shape (N, 3): [theta, phi, laser_on]")

    theta = arr[:, 0]
    phi = arr[:, 1]
    laser_on = arr[:, 2]

    # # Replace NaNs/infs with 0 to avoid propagation
    # theta = np.nan_to_num(theta, nan=0.0, posinf=max_angle, neginf=-max_angle)
    # phi = np.nan_to_num(phi, nan=0.0, posinf=max_angle, neginf=-max_angle)

    # Scale from [-max_angle, max_angle] to [-max_volts, max_volts]
    scale = max_volts / (2.0 * float(max_angle))  # Correct scale calculation
    theta_volts = theta * scale
    phi_volts = phi * scale

    bin_X = voltage_to_binary(theta_volts)
    bin_Y = voltage_to_binary(phi_volts)


    # Laser flag to uint8 0/1
    laser_u8 = (laser_on != 0).astype(np.uint8)

    # Pack as 5-byte records: [theta (be u2), phi (be u2), laser_on (u1)]
    n = bin_X.shape[0]
    rec = np.empty(n, dtype=[('theta', '>u2'), ('phi', '>u2'), ('laser', 'u1')])
    rec['theta'] = bin_X
    rec['phi'] = bin_Y
    rec['laser'] = laser_u8
    return rec.tobytes()

def createBinFile(spherical_data, max_angle=0.47, max_volts=7.5):
    bin_data = toBinCmd(spherical_data, max_angle, max_volts)
    return bin_data