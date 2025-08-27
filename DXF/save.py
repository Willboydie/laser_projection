import numpy as np

def toBinCmd(spherical, max_angle):
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

    # Scale to 16-bit integers (0-65535), clamp, then cast
    scale = 65535.0 / (2.0 * float(max_angle))
    theta_scaled = ((theta + max_angle) * scale)
    phi_scaled = ((phi + max_angle) * scale)
    theta_scaled = np.clip(theta_scaled, 0.0, 65535.0).astype(np.uint16)
    phi_scaled = np.clip(phi_scaled, 0.0, 65535.0).astype(np.uint16)

    # Laser flag to uint8 0/1
    laser_u8 = (laser_on != 0).astype(np.uint8)

    # Pack as 5-byte records: [theta (le u2), phi (le u2), laser_on (u1)]
    n = theta_scaled.shape[0]
    rec = np.empty(n, dtype=[('theta', '<u2'), ('phi', '<u2'), ('laser', 'u1')])
    rec['theta'] = theta_scaled
    rec['phi'] = phi_scaled
    rec['laser'] = laser_u8
    return rec.tobytes()

def createBinFile(spherical_data, max_angle):
    cmd = toBinCmd(spherical_data, max_angle)
    with open('../data/path.bin', 'wb') as f:
        f.write(cmd)
    print("Data saved to data/path.bin")