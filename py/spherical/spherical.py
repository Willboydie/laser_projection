
import numpy as np

def toSphericalBasic(cart_point):
    x, y, z, laser_on = cart_point
    r = np.sqrt(x**2 + y**2 + z**2)
    
    if r == 0:
        return [0, 0, laser_on]  # Handle origin
    
    phi = np.arcsin(z/r)
    theta = np.arctan2(y, x)
    
    return [theta, phi, laser_on]

def toSpherical(cart_point):
    return toSphericalBasic(cart_point)

def project_onto_outward_curling_sphere(cart_path, alpha=1, beta=0.5):
    cart_path = np.array(cart_path)
    for i, point in enumerate(cart_path):
        x, y, z = point[:3]
        discriminant = x**2 + alpha*y**2 - beta*z**2
        if discriminant >= 0:
            cart_path[i, 0] = 2*x - np.sqrt(discriminant)
    return cart_path
