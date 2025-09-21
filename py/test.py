import DXF.dxf_processing as dxf_processing
import geometry.shapes as shapes
import geometry.simplify as geom
import save
import numpy as np
import spherical.spherical as spherical
import pickle


if __name__ == "__main__":

    N = 65535
    rec = np.empty(N, dtype=[('A', '>u2'), ('B', '>u2'), ('laser', 'u1')])
    for i in range(N):
        rec[i]['A'] = i
        rec[i]['B'] = i
        rec[i]['laser'] = 1
    
    # Convert to bytes after filling the array
    rec_bytes = rec.tobytes()
    with open('../data/projections/path.bin', 'wb') as f:
        f.write(rec_bytes)
    print("Data saved to data/path.bin")
