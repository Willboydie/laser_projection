import dxf_processing as dxf
import numpy as np
import save

all_layers, num_points = dxf.extract_dxf_paths("LowerBroich.dxf")
layer = "INTERNAL_WALLS"
polyline = all_layers[layer][4]

print(polyline.shape)
print(polyline[0], polyline[-3])

max_angle = np.pi/6
save.createBinFile(polyline, max_angle, max_range=35000.0)

import matplotlib.pyplot as plt
import numpy as np
fig, ax = plt.subplots(figsize=(10, 10))

entity = polyline
   
theta = np.array(entity)[:, 0]
phi = np.array(entity)[:, 1]

ax.plot(theta, phi, 'bo', ms=0.5)

plt.show()