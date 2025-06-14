import numpy as np

def found_angle(omega, tau, epsilon, prev_phi, prev_theta, sin_prev_phi, cos_prev_phi, sin_prev_theta, cos_prev_theta, e_x, e_y, e_z):
    phi_matched = abs(sin_prev_phi + e_z - np.sin(prev_phi + tau)) > epsilon
    theta_condition1_matched = abs(cos_prev_theta * cos_prev_phi + e_x - np.cos(prev_theta + omega) * np.cos(prev_phi + tau)) < epsilon
    theta_condition2_matched = abs(sin_prev_theta * cos_prev_phi + e_y - np.sin(prev_theta + omega) * np.cos(prev_phi + tau)) < epsilon
    return (phi_matched and theta_condition1_matched and theta_condition2_matched)


def angle_search(cart_path, step, epsilon, search_radius):

    sph_path = []
    values = np.arange(-search_radius, search_radius, step)
    grid = np.array(np.meshgrid(values, values)).T.reshape(-1, 2)
    length = len(cart_path)
    print(f"grid density: {len(grid)}\n")
    
    for i in range(len(cart_path)):
        x, y, z = cart_path[i, :]
        if i % 100 == 0:
            print(f"Searching for point: ({x}, {y}, {z}) --- {i}/{length}")

        if i == 0:
            prev_theta, prev_phi = toSpherical([x, y, z])
            sph_path.append([prev_theta, prev_phi])

        else:
            prev_x, prev_y, prev_z = cart_path[i-1, :]
            prev_r = np.sqrt(prev_x**2 + prev_y**2 + prev_z**2)
            r = np.sqrt(x**2 + y**2 + z**2)

            e_x = (prev_x / prev_r) - x/r
            e_y = (prev_y / prev_r) - y/r
            e_z = (prev_z / prev_r) - z/r

            cos_prev_theta = np.cos(prev_theta)
            sin_prev_theta = np.sin(prev_theta)
            cos_prev_phi = np.cos(prev_phi)
            sin_prev_phi = np.sin(prev_phi)

            j = 0
            for omega,tau in grid:
                j+=1
                if found_angle(omega, tau, epsilon, prev_phi, prev_theta, sin_prev_phi, cos_prev_phi, sin_prev_theta, cos_prev_theta, e_x, e_y, e_z):
                    phi = prev_phi + tau
                    theta = prev_theta + omega
                    break

            if j == len(grid) - 1:
                print(f"angles NOT found for point: ({x}, {y}, {z})\n")

            sph_path.append([theta, phi])
            prev_theta = theta
            prev_phi = phi

    return np.array(sph_path)