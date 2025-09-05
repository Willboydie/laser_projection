
import numpy as np

def toSpherical(cart_point):
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

def toWallCoords(cart_path, d_to_wall):
    cart_path = np.asarray(cart_path, dtype=np.float64)
    
    def normalize_robust(vals):
        v_min, v_max = np.min(vals), np.max(vals)
        if v_max == v_min:
            return np.zeros_like(vals)
        return 2 * (vals - (v_max + v_min)/2) / (v_max - v_min)
    
    normal_xs = normalize_robust(cart_path[:, 0])
    normal_ys = normalize_robust(cart_path[:, 1]) 
    normal_zs = normalize_robust(cart_path[:, 2])
    
    wall_coords = np.column_stack([
        normal_zs + d_to_wall,  # wall_x
        normal_xs,              # wall_y  
        normal_ys,              # wall_z
        cart_path[:, 3]         # laser_ons
    ])
    
    return wall_coords

def convert_to_wall_angles(cart_path, d_to_wall, alpha=1, beta=0.5):
    wall_coords = toWallCoords(cart_path, d_to_wall)
    corrected_path = project_onto_outward_curling_sphere(wall_coords)
    
    sph_path = []
    for point in wall_coords:
        x, y, z, laser_on = point
        theta, phi = toSpherical([x, y, z])
        sph_path.append([theta, -phi, laser_on])  ### I dont know why this is -phi but it works
    return np.array(sph_path)


def 