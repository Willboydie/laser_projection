import matplotlib.pyplot as plt

def plot_path(path):

    fig, ax = plt.subplots()

    if isinstance(path, list):
        for subpath in path:
            ax.plot(subpath[:, 0], subpath[:, 1], 'k-')
    else:
        ax.plot(path[:, 0], path[:, 1], 'kx', ms=2)
        ax.plot(path[:, 0], path[:, 1], 'r-')

    plt.show()