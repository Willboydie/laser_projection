import dxf_processing as dxf
import json
import numpy as np

all_layers = dxf.extract_dxf_paths("LowerBroich.dxf")
layer = "INTERNAL_WALLS"
polyline = all_layers[layer][4]

print(np.array(polyline)[:, -1])


import matplotlib.pyplot as plt
import numpy as np
fig, ax = plt.subplots(figsize=(10, 10))

entity = polyline
   
x = np.array(entity)[:, 0]
y = np.array(entity)[:, 1]

ax.plot(x, y, 'bo', ms=0.5)

plt.show()