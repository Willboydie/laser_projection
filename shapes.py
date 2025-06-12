import numpy as np

def cartesianCircle(radius, inc, c):
    t = np.arange(0, 2 * np.pi, inc)
    z = radius * np.cos(t)
    y = radius * np.sin(t)
    x = np.ones(np.shape(z)[0]) * c
    return np.column_stack((x, y, z))

def cartesianRectangle(inc, c, r):
    # Top side (left to right)
    y_top = np.arange(-r/2, r/2, inc)
    z_top = np.full_like(y_top, r/2)
    x_top = np.full_like(y_top, c)
    top = np.column_stack((x_top, y_top, z_top))

    # Right side (top to bottom)
    z_right = np.arange(r/2, -r/2, -inc)
    y_right = np.full_like(z_right, r/2)
    x_right = np.full_like(z_right, c)
    right = np.column_stack((x_right, y_right, z_right))

    # Bottom side (right to left)
    y_bottom = np.arange(r/2, -r/2, -inc)
    z_bottom = np.full_like(y_bottom, -r/2)
    x_bottom = np.full_like(y_bottom, c)
    bottom = np.column_stack((x_bottom, y_bottom, z_bottom))

    # Left side (bottom to top)
    z_left = np.arange(-r/2, r/2, inc)
    y_left = np.full_like(z_left, -r/2)
    x_left = np.full_like(z_left, c)
    left = np.column_stack((x_left, y_left, z_left))

    # Concatenate in chronological order: top → right → bottom → left
    path = np.concatenate((top, right, bottom, left), axis=0)

    return path


