
import numpy as np
import math

h = 0.015

def toSphericalBasic(cart_point):
    x, y, z, laser_on = cart_point
    r = np.sqrt(x**2 + y**2 + z**2)
    
    if r == 0:
        return [0, 0, laser_on]  # Handle origin
    
    phi = np.arcsin(z/r)
    theta = np.arctan2(y, x)
    
    return [theta, phi, laser_on]

def toSpherical(cart_point):
    return np.array(toSphericalBasic(cart_point))

pi_2 = math.pi / 2

def arccotangent(x):
    return np.where(x == 0, np.pi / 2, np.arctan2(1, x))

def ymirror_formula(x, z, h):
    return (arccotangent((h - z)/x) - np.pi/2) / 1

def xmirror_formula(x, y, h, b):
    return (arccotangent((y * np.sin(pi_2 + b)) / (x + h * np.sin(pi_2 + b))) - np.pi/2) / (-1)


def toGalvoSpace(x, y, z, laser_on):
    b = ymirror_formula(x, z, h)
    a = xmirror_formula(x, y, h, b)

    return np.array([a, b, laser_on]).T  ## transpose to get (N, 3)