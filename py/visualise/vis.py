import matplotlib.pyplot as plt
import struct
import numpy as np

def plot_path(path):

    fig, ax = plt.subplots()

    if isinstance(path, list):
        for subpath in path:
            ax.plot(subpath[:, 0], subpath[:, 1], 'ko', ms=1)
    else:
        ax.plot(path[:, 0], path[:, 1], 'kx', ms=2)
        ax.plot(path[:, 0], path[:, 1], 'r-')

    plt.show()


def plot_bin_file(filepath):
    """
    Plots theta vs phi from a binary file.
    
    Binary format per record (5 bytes total):
    - theta: 2 bytes (signed short integer)
    - phi: 2 bytes (signed short integer)
    - laser_on: 1 byte (boolean)
    
    Points are colored based on laser_on value:
    - Red when laser is on (laser_on = True)
    - Blue when laser is off (laser_on = False)
    """
    theta_on = []
    phi_on = []
    theta_off = []
    phi_off = []
    start = []
    
    with open(filepath, 'rb') as f:
        while True:
            # Read 5 bytes at a time
            chunk = f.read(5)
            if len(chunk) < 5:
                break
            
            # Unpack: 'H' = unsigned short (2 bytes), 'H' = unsigned short (2 bytes), 'B' = unsigned char (1 byte)
            theta, phi, laser_on = struct.unpack('>HHB', chunk)
            if not theta_on and not theta_off:
                start = [theta, phi]
            if laser_on:
                theta_on.append(theta)
                phi_on.append(phi)
            else:
                theta_off.append(theta)
                phi_off.append(phi)
    
    # Create the plot
    fig, ax = plt.subplots(figsize=(10, 8))

    ax.plot(start[0], start[1], 'go', ms=4, label='Start')
    
    # Plot points with laser off (blue) first so laser on points appear on top
    if theta_off:
        ax.scatter(theta_off, phi_off, c='blue', s=1, alpha=0.5, label='Laser Off')
    
    if theta_on:
        ax.scatter(theta_on, phi_on, c='red', s=1, alpha=0.5, label='Laser On')
    
    ax.set_xlabel('Theta')
    ax.set_ylabel('Phi')
    ax.set_title('Theta vs Phi Visualization')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.show()
    
    # Print summary statistics
    total_points = len(theta_on) + len(theta_off)
    print(f"Total points: {total_points}")
    print(f"Laser on points: {len(theta_on)} ({100*len(theta_on)/total_points:.1f}%)")
    print(f"Laser off points: {len(theta_off)} ({100*len(theta_off)/total_points:.1f}%)")


def plot_array_file(filepath):
    """
    Plots a numpy array from a saved .npy file.
    
    Expected array format:
    - Shape (N, 3): [x_angle, y_angle, laser_on] - Galvo angles (2D plot)
    - Shape (N, 4): [x, y, z, state] - Cartesian coordinates (3D plot)
    
    Points are colored based on laser_on/state value:
    - Green when laser_on/state = 1 (on)
    - Red when laser_on/state = 0 (off)
    """
    # Load the array
    data = np.load(filepath)
    
    if data.ndim != 2 or data.shape[1] not in [3, 4]:
        raise ValueError("Array must have shape (N, 3) or (N, 4)")
    
    # Determine if this is galvo angle data (N, 3) or cartesian data (N, 4)
    if data.shape[1] == 3:
        # Galvo angle data: [x_angle, y_angle, laser_on]
        plot_galvo_angles(data, filepath)
    else:
        # Cartesian data: [x, y, z, state]
        plot_cartesian_3d(data, filepath)


def plot_galvo_angles(data, filepath):
    """Plot galvo angle data as 2D plot"""
    x_angle = data[:, 0]
    y_angle = data[:, 1]
    laser_on = data[:, 2]
    
    # Create 2D plot
    fig, ax = plt.subplots(figsize=(12, 10))
    
    # Find segment boundaries
    segment_starts = np.where(np.diff(laser_on) != 0)[0] + 1
    segment_starts = np.concatenate(([0], segment_starts, [len(laser_on)]))
    
    # Plot each segment with appropriate color
    for i in range(len(segment_starts) - 1):
        start_idx = segment_starts[i]
        end_idx = segment_starts[i + 1]
        
        # Determine color based on state
        state = laser_on[start_idx]
        color = 'green' if state == 1 else 'red'
        linewidth = 2 if state == 1 else 1
        label = 'Laser On' if state == 1 else 'Laser Off'
        
        # Plot this segment
        ax.plot(x_angle[start_idx:end_idx], y_angle[start_idx:end_idx], 
               color=color, linewidth=linewidth, label=label if i < 2 else "")
    
    # Add start and end markers
    ax.scatter(x_angle[0], y_angle[0], color='blue', s=100, label='Start', marker='o', zorder=5)
    ax.scatter(x_angle[-1], y_angle[-1], color='purple', s=100, label='End', marker='s', zorder=5)
    
    ax.set_xlabel('X Angle (radians)', fontsize=12)
    ax.set_ylabel('Y Angle (radians)', fontsize=12)
    ax.set_title(f'Galvo Angle Visualization\n{filepath}', fontsize=14)
    
    # Remove duplicate labels in legend
    handles, labels = ax.get_legend_handles_labels()
    by_label = dict(zip(labels, handles))
    ax.legend(by_label.values(), by_label.keys())
    
    ax.grid(True, alpha=0.3)
    ax.set_aspect('equal', adjustable='box')
    
    plt.tight_layout()
    plt.show()
    
    # Print summary statistics
    print(f"Total points: {len(data)}")
    on_points = np.sum(laser_on == 1)
    off_points = np.sum(laser_on == 0)
    print(f"Laser on points: {on_points} ({100*on_points/len(data):.1f}%)")
    print(f"Laser off points: {off_points} ({100*off_points/len(data):.1f}%)")
    print(f"X angle range: [{x_angle.min():.4f}, {x_angle.max():.4f}] radians")
    print(f"Y angle range: [{y_angle.min():.4f}, {y_angle.max():.4f}] radians")
    print(f"X angle range: [{np.degrees(x_angle.min()):.2f}, {np.degrees(x_angle.max()):.2f}] degrees")
    print(f"Y angle range: [{np.degrees(y_angle.min()):.2f}, {np.degrees(y_angle.max()):.2f}] degrees")


def plot_cartesian_3d(data, filepath):
    """Plot cartesian data as 3D plot"""
    x = data[:, 0]
    y = data[:, 1]
    z = data[:, 2]
    states = data[:, 3]
    
    # Create the plot
    fig = plt.figure(figsize=(12, 10))
    ax = fig.add_subplot(111, projection='3d')
    
    # Find segment boundaries
    segment_starts = np.where(np.diff(states) != 0)[0] + 1
    segment_starts = np.concatenate(([0], segment_starts, [len(states)]))
    
    # Plot each segment with appropriate color
    for i in range(len(segment_starts) - 1):
        start_idx = segment_starts[i]
        end_idx = segment_starts[i + 1]
        
        # Determine color based on state
        state = states[start_idx]
        color = 'green' if state == 1 else 'red'
        linewidth = 2 if state == 1 else 1
        label = 'On' if state == 1 else 'Off'
        
        # Plot this segment
        ax.plot(x[start_idx:end_idx], y[start_idx:end_idx], z[start_idx:end_idx], 
               color=color, linewidth=linewidth, label=label if i < 2 else "")
    
    # Add start and end markers
    ax.scatter(x[0], y[0], z[0], color='blue', s=50, label='Start', marker='o')
    ax.scatter(x[-1], y[-1], z[-1], color='red', s=50, label='End', marker='s')
    
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title(f'3D Cartesian Visualization\n{filepath}')
    
    # Remove duplicate labels in legend
    handles, labels = ax.get_legend_handles_labels()
    by_label = dict(zip(labels, handles))
    ax.legend(by_label.values(), by_label.keys())
    
    ax.grid(True, alpha=0.3)
    
    # Set equal aspect ratio
    try:
        # Calculate the range for each axis
        x_range = x.max() - x.min()
        y_range = y.max() - y.min()
        z_range = z.max() - z.min()
        max_range = max(x_range, y_range, z_range)
        
        x_center = (x.max() + x.min()) / 2
        y_center = (y.max() + y.min()) / 2
        z_center = (z.max() + z.min()) / 2
        
        half_range = max_range / 2
        ax.set_xlim(x_center - half_range, x_center + half_range)
        ax.set_ylim(y_center - half_range, y_center + half_range)
        ax.set_zlim(z_center - half_range, z_center + half_range)
    except:
        ax.set_box_aspect([1, 1, 1])
    
    plt.tight_layout()
    plt.show()
    
    # Print summary statistics
    print(f"Total points: {len(data)}")
    on_points = np.sum(states == 1)
    off_points = np.sum(states == 0)
    print(f"On points: {on_points} ({100*on_points/len(data):.1f}%)")
    print(f"Off points: {off_points} ({100*off_points/len(data):.1f}%)")


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python vis.py <path_to_file>")
        print("\nExample files:")
        print("  Binary files:")
        print("    - data/projections/path.bin")
        print("    - data/projections/circle.bin")
        print("  Array files:")
        print("    - data/arrays/path.npy")
        sys.exit(1)
    
    filepath = sys.argv[1]
    
    # Determine file type and call appropriate function
    if filepath.endswith('.npy'):
        plot_array_file(filepath)
    else:
        plot_bin_file(filepath)