import numpy as np

def cartesianCircle(max_angle, inc):
    t = np.arange(0, 2 * np.pi, inc)
    radius = abs(np.tan(max_angle))
    z = radius * np.cos(t)
    y = radius * np.sin(t)
    x = np.ones(np.shape(z)[0])
    return np.column_stack((x, y, z))

def toSpherical(xyz): # only works for circles
    x, y, z = xyz[:, 0], xyz[:, 1], xyz[:, 2]
    r = np.sqrt(x**2 + y**2 + z**2)
    theta = np.arctan2(y, x)  # azimuthal angle
    phi = np.arcsin(z / r)    # polar angles
    return np.column_stack((theta, phi))

def toBinCmd(spherical, max_angle):
    theta, phi = spherical[:, 0], spherical[:, 1]
    cmd = np.zeros((theta.size, 2), dtype='>u2')
    cmd[:, 0] = (((theta + max_angle) / (2 * max_angle)) * 0xFFFF).astype('>u2')
    cmd[:, 1] = (((phi + max_angle) / (2 * max_angle)) * 0xFFFF).astype('>u2')
    return cmd

def createSphericalBinFile(xyz, max_angle):
    
    spherical = toSpherical(xyz)
    cmd = toBinCmd(spherical, max_angle)
    
    # Save to binary file
    with open('data/circle.bin', 'wb') as f:
        # Write the data
        cmd.tofile(f)

if __name__ == "__main__":
    max_angle = np.pi/9  # Example max angle
    inc = 0.001     # Increment in radians

    xyz = cartesianCircle(max_angle, inc)
    createSphericalBinFile(xyz, max_angle)
    print(f"Circle data saved to data/circle.bin")
