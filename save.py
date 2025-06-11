import numpy as np

def toBinCmd(spherical, max_angle):
    theta, phi = spherical[:, 0], spherical[:, 1]
    cmd = np.zeros((theta.size, 2), dtype='>u2')
    cmd[:, 0] = (((theta + max_angle) / (2 * max_angle)) * 0xFFFF).astype('>u2')
    cmd[:, 1] = (((phi + max_angle) / (2 * max_angle)) * 0xFFFF).astype('>u2')
    return cmd

def createBinFile(spherical_data, max_angle):
    
    cmd = toBinCmd(spherical_data, max_angle)
    
    # Save to binary file
    with open('data/path.bin', 'wb') as f:
        # Write the data
        cmd.tofile(f)
        
    print("Data saved to data/path.bin")