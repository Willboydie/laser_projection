import pickle
import numpy as np
import visualise.vis as vis

start_index = 4
end_index = 36

mid1_index = 8
mid2_index = 17

with open('../data/dxf/temp.pkl', 'rb') as f:
    all_paths = pickle.load(f)

video_path_unedited = np.array(all_paths[0][17])
video_path_edited = np.array(list(video_path_unedited[start_index:mid1_index]) + list(video_path_unedited[mid1_index+1:mid2_index]) + list(video_path_unedited[mid2_index+1:end_index]) + [video_path_unedited[start_index]]) - np.array([2.64098e5, 6.95141e5, 0.0])

with open('../data/dxf/video_path.pkl', 'wb') as f:
    pickle.dump(video_path_edited, f)


with open('../data/dxf/video_path.pkl', 'rb') as f:
    video_path = pickle.load(f)

print(video_path.shape)
# vis.plot_path(video_path)

c=12
x = np.array(np.ones(len(video_path)) * c)
y = video_path[:, 0]
z = video_path[:, 1]
video_path = np.column_stack((x, y, z))


def cartesian_to_spherical(x, y, z):
    """Convert Cartesian coordinates to spherical coordinates (r, theta, phi)"""
    r = np.sqrt(x**2 + y**2 + z**2)
    phi = np.arcsin(z / r) if r > 0 else 0
    theta = np.arcsin(y / (r * np.cos(phi))) if r * np.cos(phi) > 0 else 0
    return r, theta, phi

def spherical_to_cartesian(r, theta, phi):
    """Convert spherical coordinates to Cartesian coordinates"""
    # Since cartesian_to_spherical now returns -phi, we need to negate it back
    phi_original = phi
    x = r * np.cos(theta) * np.cos(phi_original)
    y = r * np.sin(theta) * np.cos(phi_original)
    z = r * np.sin(phi_original)
    return x, y, z

def angular_distance_squared(theta1, phi1, theta2, phi2):
    """
    Calculate squared angular distance between two points in theta-phi space.
    This is a good approximation for small angles and avoids sqrt operations.
    """
    dtheta = theta2 - theta1
    dphi = phi2 - phi1
    return dtheta**2 + dphi**2

def angular_distance(theta1, phi1, theta2, phi2):
    """
    Calculate angular distance between two points in theta-phi space.
    """
    return np.sqrt(angular_distance_squared(theta1, phi1, theta2, phi2))

def cartesian_distance(p1, p2):
    """
    Calculate Cartesian distance between two points.
    """
    return np.sqrt((p2[0] - p1[0])**2 + (p2[1] - p1[1])**2 + (p2[2] - p1[2])**2)

def partition_line_spherical_gradient(cart_p1, cart_p2, minimum_increment, alpha):
    """
    Partition a line segment with gradient-based spherical increments.
    Increment is minimum at vertices and increases linearly with Cartesian distance from nearest vertex.
    
    Args:
        cart_p1: First endpoint (vertex)
        cart_p2: Second endpoint (vertex)
        minimum_increment: Minimum increment at vertices (globally defined)
        alpha: Gradient parameter controlling how increment increases with distance
    
    Returns:
        List of spherical points representing the partitioned line [theta, phi, r]
    """
    x1, y1, z1 = cart_p1
    x2, y2, z2 = cart_p2
    
    # Convert endpoints to spherical coordinates
    r1, theta1, phi1 = cartesian_to_spherical(x1, y1, z1)
    r2, theta2, phi2 = cartesian_to_spherical(x2, y2, z2)
    
    # Calculate total angular distance of the segment
    total_angular_distance = angular_distance(theta1, phi1, theta2, phi2)
    
    # Calculate total Cartesian distance for reference
    total_cartesian_distance = cartesian_distance(cart_p1, cart_p2)
    
    # Estimate number of points needed (rough calculation)
    # Use a conservative estimate based on minimum increment
    estimated_points = int(np.ceil(total_angular_distance / minimum_increment)) + 1
    
    # Generate points with gradient-based increments
    points = []
    points.append([theta1, phi1, r1])  # Add first vertex in spherical coordinates
    
    # Start from the first vertex and step along the line
    current_theta = theta1
    current_phi = phi1
    current_cartesian = np.array([x1, y1, z1])
    
    step_count = 0
    max_steps = estimated_points * 2  # Safety limit
    
    while step_count < max_steps:
        # Calculate current Cartesian position based on current spherical position
        # First convert current spherical position to Cartesian
        x_sph = np.cos(current_phi) * np.cos(current_theta)
        y_sph = np.cos(current_phi) * np.sin(current_theta)
        z_sph = np.sin(current_phi)
        
        # Solve for r to stay on the line
        dx = x2 - x1
        dy = y2 - y1
        dz = z2 - z1
        
        if abs(dx) > 1e-10 and abs(dy) > 1e-10:
            r_current = (x1/dx - y1/dy) / (x_sph/dx - y_sph/dy)
        elif abs(dy) > 1e-10 and abs(dz) > 1e-10:
            r_current = (y1/dy - z1/dz) / (y_sph/dy - z_sph/dz)
        elif abs(dx) > 1e-10 and abs(dz) > 1e-10:
            r_current = (x1/dx - z1/dz) / (x_sph/dx - z_sph/dz)
        else:
            # Fallback: linear interpolation
            r_current = r1 + (step_count / (estimated_points - 1)) * (r2 - r1)
        
        # Get current Cartesian position
        x_current, y_current, z_current = spherical_to_cartesian(r_current, current_theta, current_phi)
        current_cartesian = np.array([x_current, y_current, z_current])
        
        # Calculate Cartesian distances to both vertices
        dist_to_start = cartesian_distance(current_cartesian, cart_p1)
        dist_to_end = cartesian_distance(current_cartesian, cart_p2)
        
        # Calculate increment based on distance to nearest vertex
        # This creates a V-shape: minimum at vertices, maximum in middle
        distance_to_nearest = min(dist_to_start, dist_to_end)
        current_increment = minimum_increment + alpha * (distance_to_nearest ** 2)  # Quadratic increase
        
        # Check if we're close enough to the end
        remaining_angular_distance = angular_distance(current_theta, current_phi, theta2, phi2)
        
        if remaining_angular_distance <= current_increment:
            # We're close enough to the end, add the final vertex and break
            points.append([theta2, phi2, r2])  # Add final vertex in spherical coordinates
            break
        
        # Calculate step direction in spherical space
        dtheta_total = theta2 - theta1
        dphi_total = phi2 - phi1
        
        # Normalize direction and scale by increment
        direction_magnitude = np.sqrt(dtheta_total**2 + dphi_total**2)
        if direction_magnitude > 1e-10:
            step_dtheta = (dtheta_total / direction_magnitude) * current_increment
            step_dphi = (dphi_total / direction_magnitude) * current_increment
        else:
            step_dtheta = 0
            step_dphi = 0
        
        # Update position in spherical space
        current_theta += step_dtheta
        current_phi += step_dphi
        
        # Add the current point in spherical coordinates
        points.append([current_theta, current_phi, r_current])
        
        step_count += 1
    
    return np.array(points)


if __name__ == "__main__":
    
    # Test gradient-based partitioning
    minimum_increment = 4e-5  # Minimum increment at vertices
    alpha = 1e-3  # Gradient parameter
    partitioned_path = []
    
    print(f"\nGradient-based partitioning:")
    print(f"Minimum increment: {minimum_increment} rad")
    print(f"Alpha (gradient): {alpha}")
    
    total_points = 0
    for i in range(len(video_path)-1):
        partitioned_points = partition_line_spherical_gradient(
            video_path[i], video_path[i+1], minimum_increment, alpha
        )
        print(f"Line segment {i}: {len(partitioned_points)} points")
        partitioned_path.append(partitioned_points)
        total_points += len(partitioned_points)
    print(f"Total points: {total_points}")

    vis.plot_path(partitioned_path)

    with open('../data/dxf/video_path_sph.pkl', 'wb') as f:
        pickle.dump(partitioned_path, f)

    with open('../data/dxf/video_path_sph.pkl', 'rb') as f:
        video_path_sph = pickle.load(f)

    video_path_sph_processed = []
    for i in range(len(video_path_sph)):
        for j in range(len(video_path_sph[i])):
            video_path_sph_processed.append([video_path_sph[i][j][0], video_path_sph[i][j][1], 1])

    video_path_sph_processed = np.array(video_path_sph_processed)
    print(video_path_sph_processed.shape)

    import save
    save.createBinFile(video_path_sph_processed, 0.47, 0.75*2**16)
