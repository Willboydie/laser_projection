import shapes
import numpy as np
import matplotlib.pyplot as plt
import save


max_angle = np.pi/6
angle_inc = 0.001
space_inc = 0.1
c = 473
radius = 100

alpha = 1
beta = 0.5

path = shapes.cartesianRectangle(space_inc, c, radius)
path = shapes.project_onto_outward_curling_sphere(path, alpha, beta) # we need to decide if this method works for lines which cross X space.
# path = shapes.hemiRectangle(space_inc, c, radius)
# path = shapes.cartesianCircle(radius, angle_inc, c)
# path = correct(path, alpha)

sph_path = []
for point in path:
    sph = shapes.toSpherical(point)
    sph_path.append(sph)

sph_path = np.array(sph_path)
save.createBinFile(sph_path, max_angle)

# # Plotting
# fig = plt.figure()
# ax = fig.add_subplot(111, projection='3d')

# # Plot the path
# ax.plot(path[:, 0], path[:, 1], path[:, 2], 'rx', ms=0.3)

# # Set labels and limits
# ax.set_xlabel('X')
# ax.set_ylabel('Y')
# ax.set_zlabel('Z')
# # ax.set_xlim(0, 2)
# # ax.set_ylim(-1.2, 1.2)
# # ax.set_zlim(-1.2, 1.2)
# ax.set_title(f'Square Path in Plane x = {c}')

# # Make axes have equal scale
# max_range = np.array([path[:, 0].max()-path[:, 0].min(), 
#                       path[:, 1].max()-path[:, 1].min(), 
#                       path[:, 2].max()-path[:, 2].min()]).max() / 2.0

# mid_x = (path[:, 0].max()+path[:, 0].min()) * 0.5
# mid_y = (path[:, 1].max()+path[:, 1].min()) * 0.5
# mid_z = (path[:, 2].max()+path[:, 2].min()) * 0.5

# ax.set_xlim(mid_x - max_range, mid_x + max_range)
# ax.set_ylim(mid_y - max_range, mid_y + max_range)
# ax.set_zlim(mid_z - max_range, mid_z + max_range)

# plt.show()