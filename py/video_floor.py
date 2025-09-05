import pickle
import numpy as np
import visualise.vis as vis

# start_index = 4
# end_index = 36

# mid1_index = 8
# mid2_index = 17

# with open('../data/dxf/temp.pkl', 'rb') as f:
#     all_paths = pickle.load(f)

# video_path_unedited = np.array(all_paths[0][17])
# video_path_edited = np.array(list(video_path_unedited[start_index:mid1_index]) + list(video_path_unedited[mid1_index+1:mid2_index]) + list(video_path_unedited[mid2_index+1:end_index]) + [video_path_unedited[start_index]]) - np.array([2.64098e5, 6.95141e5, 0.0])

# with open('../data/dxf/video_path.pkl', 'wb') as f:
#     pickle.dump(video_path_edited, f)


with open('../data/dxf/video_path.pkl', 'rb') as f:
    video_path = pickle.load(f)

print(video_path.shape)
vis.plot_path(video_path)
