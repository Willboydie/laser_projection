import numpy as np
import save

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


if __name__ == "__main__":
    max_angle = np.pi/9  # Example max angle
    inc = 0.001     # Increment in radians

    xyz = cartesianCircle(max_angle, inc)
    spherical = toSpherical(xyz)

    save.createBinFile(spherical, max_angle)
