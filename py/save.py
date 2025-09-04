import numpy as np

def toBinCmd(spherical, max_angle, max_range=65535.0):
    # Expect spherical shape (N, 3): [theta, phi, laser_on]
    if spherical is None or np.size(spherical) == 0:
        return b""

    arr = np.asarray(spherical, dtype=np.float64)
    if arr.ndim != 2 or arr.shape[1] != 3:
        raise ValueError("spherical must have shape (N, 3): [theta, phi, laser_on]")

    theta = arr[:, 0]
    phi = arr[:, 1]
    laser_on = arr[:, 2]

    # Replace NaNs/infs with 0 to avoid propagation
    theta = np.nan_to_num(theta, nan=0.0, posinf=0.0, neginf=0.0)
    phi = np.nan_to_num(phi, nan=0.0, posinf=0.0, neginf=0.0)

    # Center on 2^15 (32768) with range defined by max_range
    # Scale from [-max_angle, max_angle] to [32768 - max_range/2, 32768 + max_range/2]
    center = 2**15  # 2^15
    scale = 2**16 / (2.0 * float(max_angle))
    theta_scaled = center + (theta * scale)
    phi_scaled = center + (phi * scale)
    
    # Clamp to valid uint16 range and cast
    theta_scaled = np.clip(theta_scaled, 0.25*2**16, 0.75*2**16).astype(np.uint16)
    phi_scaled = np.clip(phi_scaled, 0.25*2**16, 0.75*2**16).astype(np.uint16)

    # Laser flag to uint8 0/1
    laser_u8 = (laser_on != 0).astype(np.uint8)

    # Pack as 5-byte records: [theta (be u2), phi (be u2), laser_on (u1)]
    n = theta_scaled.shape[0]
    rec = np.empty(n, dtype=[('theta', '>u2'), ('phi', '>u2'), ('laser', 'u1')])
    rec['theta'] = theta_scaled
    rec['phi'] = phi_scaled
    rec['laser'] = laser_u8
    return rec.tobytes(), theta_scaled, phi_scaled, laser_u8

def createBinFile(spherical_data, max_angle, max_range=65535.0):
    cmd, theta_scaled, phi_scaled, laser_u8 = toBinCmd(spherical_data, max_angle, max_range=max_range)
    with open('data/path.bin', 'wb') as f:
        f.write(cmd)
    print("Data saved to data/path.bin")
    return cmd, theta_scaled, phi_scaled, laser_u8