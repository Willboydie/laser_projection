import shapes
import numpy as np
import save


def toSpherical(cart_point):

    x, y, z = cart_point
    # r = np.sqrt(x**2 + y**2 + z**2)
    r = 160 #179

    phi = np.arcsin(z/r)
    theta = np.arctan2(y, x)

    return [theta, phi]


space_inc = 0.01
c = 164
width = 110
max_angle = np.pi/6;

cart_path = shapes.cartesianRectangle(space_inc, c, width)
sph_path = []
for point in cart_path:
    sph = toSpherical(point)
    sph_path.append(sph)

sph_path = np.array(sph_path)

save.createBinFile(sph_path, max_angle);

re_proj = []
for sph in sph_path:
    theta, phi = sph
    x = 

import matplotlib.pyplot as plt

fig, ax = plt.subplots()
ax.plot(sph_path[:, 0], sph_path[:, 1])
plt.show()
