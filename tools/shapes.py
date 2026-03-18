import numpy as np

def cartesianCircle(radius, inc, c):
    t = np.arange(0, 2 * np.pi, inc)
    z = radius * np.cos(t)
    y = radius * np.sin(t)
    x = np.ones(np.shape(z)[0]) * c
    return np.column_stack((x, y, z))


def cartesianRectangle(inc, c, r):
    # Top side (left to right)
    y_top = np.arange(-r, r, inc)
    z_top = np.full_like(y_top, r)
    x_top = np.full_like(y_top, c)
    top = np.column_stack((x_top, y_top, z_top))

    # Right side (top to bottom)
    z_right = np.arange(r, -r, -inc)
    y_right = np.full_like(z_right, r)
    x_right = np.full_like(z_right, c)
    right = np.column_stack((x_right, y_right, z_right))

    # Bottom side (right to left)
    y_bottom = np.arange(r, -r, -inc)
    z_bottom = np.full_like(y_bottom, -r)
    x_bottom = np.full_like(y_bottom, c)
    bottom = np.column_stack((x_bottom, y_bottom, z_bottom))

    # Left side (bottom to top)
    z_left = np.arange(-r, r, inc)
    y_left = np.full_like(z_left, -r)
    x_left = np.full_like(z_left, c)
    left = np.column_stack((x_left, y_left, z_left))

    # Concatenate in chronological order: top → right → bottom → left
    path = np.concatenate((top, right, bottom, left), axis=0)

    return path


def hemiRectangle(inc, c, r):

    # Bottom side (right to left)
    y_bottom = np.arange(r, -r, -inc)
    z_bottom = np.full_like(y_bottom, -r)
    x_bottom = np.full_like(y_bottom, c)
    bottom = np.column_stack((x_bottom, y_bottom, z_bottom))

    # Left side (bottom to top)
    z_square_left = np.arange(-r, 0, inc)
    y_square_left = np.full_like(z_square_left, -r)
    x_square_left = np.full_like(z_square_left, c)
    left_square = np.column_stack((x_square_left, y_square_left, z_square_left))

    z_square_right = np.arange(0, -r, -inc)
    y_square_right = np.full_like(z_square_right, r)
    x_square_right = np.full_like(z_square_right, c)
    right_square = np.column_stack((x_square_right, y_square_right, z_square_right))

    t = np.linspace(-np.pi/2, np.pi/2, int(np.pi*r / inc))
    z = r * np.cos(t)
    y = r * np.sin(t)
    x = np.ones(np.shape(z)[0]) * c
    top_hemi = np.column_stack((x, y, z))

    # Concatenate in chronological order: top → right → bottom → left
    path = np.concatenate((top_hemi, right_square, bottom, left_square), axis=0)

    return path