
import numpy as np

def toSphericalBasic(cart_point):
    x, y, z = cart_point
    r = np.sqrt(x**2 + y**2 + z**2)
    
    if r == 0:
        return [0, 0]  # Handle origin
    
    phi = np.arcsin(np.clip(z/r, -1, 1))
    
    rho = r * np.cos(phi)
    if abs(rho) < 1e-10:  # Near poles
        theta = 0
    else:
        theta = np.arcsin(np.clip(y/rho, -1, 1))
    
    return [theta, phi]

def project_onto_outward_curling_sphere(cart_path, alpha=1, beta=0.5):
    cart_path = np.array(cart_path)
    for i, point in enumerate(cart_path):
        x, y, z = point[:3]
        discriminant = x**2 + alpha*y**2 - beta*z**2
        if discriminant >= 0:
            cart_path[i, 0] = 2*x - np.sqrt(discriminant)
    return cart_path
