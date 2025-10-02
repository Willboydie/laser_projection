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


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python vis.py <path_to_bin_file>")
        print("\nExample bin files:")
        print("  - data/projections/path.bin")
        print("  - data/projections/circle.bin")
        sys.exit(1)
    
    plot_bin_file(sys.argv[1])