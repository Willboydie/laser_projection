
import shapes
import numpy as np

inc = 0.1
c = 200
r = 50
N = 100

path = shapes.cartesianRectangle(inc, c, r)
# path = shapes.cartesianCircle(r, inc, c)
# xs = np.linspace(100, 80, N)
# ys = np.linspace(-80, 80, N) 
# zs = np.linspace(-30, 50, N)

# path = np.column_stack([xs, ys, zs])

for i,point in enumerate(path):
    x, y, z = point
    r = np.sqrt(x**2 + y**2 + z**2)
    path[i, 0] = (path[i, 0] / r)**-1
    path[i, 1] = path[i, 1] / r
    path[i, 2] = path[i, 2] / r

# print("corners: \n")
# print(f"{(path[0, 0], path[0, 1], path[0, 2])}\n")
# print(f"{(path[500, 0], path[500, 1], path[500, 2])}\n")
# print(f"{(path[1000, 0], path[1000, 1], path[1000, 2])}\n")
# print(f"{(path[1500, 0], path[1500, 1], path[1500, 2])}\n")

import matplotlib.pyplot as plt

# Plotting
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

# Plot the path
ax.plot(path[:, 0], path[:, 1], path[:, 2], color='blue')
ax.plot(path[0, 0], path[0, 1], path[0, 2], 'rx')
ax.plot(path[500, 0], path[500, 1], path[500, 2], 'rx')
ax.plot(path[1000, 0], path[1000, 1], path[1000, 2], 'rx')
ax.plot(path[1500, 0], path[1500, 1], path[1500, 2], 'rx')

# Set labels and limits
ax.set_xlabel('X/r')
ax.set_ylabel('Y/r')
ax.set_zlabel('Z/r')
ax.set_title('Ratio between (x, y, z) and r in cartesian space on the plane x = c')

# Make axes have equal scale
max_range = np.array([path[:, 0].max()-path[:, 0].min(), 
                      path[:, 1].max()-path[:, 1].min(), 
                      path[:, 2].max()-path[:, 2].min()]).max() / 2.0

mid_x = (path[:, 0].max()+path[:, 0].min()) * 0.5
mid_y = (path[:, 1].max()+path[:, 1].min()) * 0.5
mid_z = (path[:, 2].max()+path[:, 2].min()) * 0.5

ax.set_xlim(mid_x - max_range, mid_x + max_range)
ax.set_ylim(mid_y - max_range, mid_y + max_range)
ax.set_zlim(mid_z - max_range, mid_z + max_range)

plt.show()