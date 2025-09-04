import numpy as np
import ezdxf
from collections import defaultdict, deque
from typing import List, Tuple, Optional
try:
	from scipy.spatial import cKDTree as KDTree  # faster if available
except Exception:  # noqa: S110 broad-except acceptable for optional dep
	KDTree = None
import pickle

# "LINE"          # Straight line segment
# "CIRCLE"        # Circle defined by center and radius
# "ARC"           # Arc defined by center, radius, start angle, and end angle
# "POLYLINE"      # 2D or 3D polyline (sequence of connected lines/arcs)
# "ELLIPSE"       # Ellipse defined by center, major axis, and minor axis
# "LWPOLYLINE"    # Polyline defined by vertices and bulge values


def save_dxf(vectors, selected_layers):
    layer_path = [vectors[layer] for layer in selected_layers]

    with open(f"../../data/dxf/temp.pkl", "wb") as f:
        pickle.dump(layer_path, f)
    
    print(f"Saved selected_layers to data/dxf/temp.pkl")


def read_dxf_vectors(file_path, eps, msp_=True, psp_=False, blocks_=False):
    """Reads DXF file and extracts 3D vector lines, ignoring doors/windows."""

    doc = ezdxf.readfile(file_path)

    msp = doc.modelspace()
    psp = doc.paperspace()
    blocks = doc.blocks

    # Use the local function directly, not the imported one
    extracted_layers = extract_layers(file_path, msp_, psp_, blocks_)
    layers = extracted_layers

    all_vectors = {}
    num_points = 0
    for layer in layers['msp']:
        vectors = []
        for entity in msp:
            entity_dxf = entity.dxf
            entity_type = entity.dxftype()
            if entity_dxf.layer == layer:
                path = process_dxf_entity(entity, entity_dxf, entity_type, eps)
                if path is not None:
                    vectors.append(path)
        all_vectors[layer] = vectors
        num_points += len(vectors)

    return all_vectors, num_points

def extract_layers(file_path, msp=True, psp=False, blocks=False):

    layers = {
        'msp': {},
        'psp': {},
        'blocks': {}
    }
    
    doc = ezdxf.readfile(file_path)
    
    if msp:
        msp_section = doc.modelspace()
        for entity in msp_section:
            sorting(entity, layers['msp'])

    if psp:
        psp_section = doc.paperspace()
        for entity in psp_section:
            sorting(entity, layers['psp'])

    if blocks:
        blocks_section = doc.blocks
        for block in blocks_section:
            for entity in block:
                sorting(entity, layers['blocks'])

    return layers

def sorting(entity, layers):
    
    layer = entity.dxf.layer
    type = entity.dxftype()

    if layer in layers:
        layers[layer] |= {type}
    else:
        layers[layer] = {type}

    return None

def process_dxf_entity(entity, entity_dxf, entity_type, eps):

    if entity_type == "ARC":
        return process_arc(entity_dxf, eps)
    elif entity_type == "CIRCLE":
        return process_circle(entity_dxf, eps)
    elif entity_type == "LINE":
        return process_line(entity_dxf)
    elif entity_type == "LWPOLYLINE":
        return process_lwpolyline(entity, entity_dxf, eps)
    else:
        return None

def process_arc(entity_dxf, eps):
    centre = entity_dxf.center
    radius = entity_dxf.radius
    start_angle = entity_dxf.start_angle
    end_angle = entity_dxf.end_angle
    elevation = entity_dxf.elevation
    
    path = trace_minimal_arc(centre, radius, start_angle, end_angle, elevation, eps)
    return path

def process_circle(entity_dxf, eps):
    centre = entity_dxf.center
    radius = entity_dxf.radius
    start_angle = 0
    end_angle = 2*np.pi
    elevation = entity_dxf.elevation

    path = trace_minimal_arc(centre, radius, start_angle, end_angle, elevation, eps)
    return path

def process_line(entity_dxf):
    start = entity_dxf.start
    end = entity_dxf.end

    path = np.array([start, end])
    return path

def process_lwpolyline(entity, entity_dxf, eps):
    lwpolyline = list(entity.__iter__())
    closed = entity.is_closed
    elevation = entity_dxf.elevation

    if closed:
        lwpolyline = lwpolyline + [lwpolyline[0]]

    all_points = []

    for i, segment in enumerate(lwpolyline[:-1]):
        x, y = segment[:2]
        start = [x, y, 0.0]
        next_x = lwpolyline[i+1][0]
        next_y = lwpolyline[i+1][1]
        end = [next_x, next_y, 0.0]
        bulge = segment[4]

        if bulge == 0.0:
            # Create a simple line segment for non-bulge segments
            sub_segment = np.array([start, end])
        else:
            centre, radius, start_angle, end_angle = get_arc_parameters_from_bulge(start, end, bulge)
            sub_segment = trace_minimal_arc(centre, radius, start_angle, end_angle, elevation, eps)

        all_points.extend(sub_segment)
    
    path = np.array(all_points) if all_points else np.empty((0, 3))
    
    return path

def get_arc_parameters_from_bulge(start, end, bulge):
    '''
    Get the arc parameters from the bulge, start and end points.
    '''

    start = np.array(start)
    end = np.array(end)

    bulge_sign = bulge/abs(bulge)

    chord_len = ((start[0]-end[0])**2 + (start[1] - end[1])**2)**(1/2)
    m = chord_len*0.5

    theta = 2*np.pi - 4*np.arctan(m/abs(bulge))
    if theta > np.pi:
        theta = 2*np.pi - theta
    radius = (bulge**2 + m**2) / (2*abs(bulge))

    a = end[0] - start[0]
    b = end[1] - start[1] # start - end = [a, b]
    midpoint = np.array([(start[0] + end[0])/2, (start[1] + end[1])/2, 0])

    k = (radius - abs(bulge)) / np.sqrt(b**2 + a**2)
    n = np.array([bulge_sign*b*k, -bulge_sign*a*k, 0])
    centre = midpoint + n

    centre_start_direction = (start - centre) / np.linalg.norm(np.array(start - centre))

    cos_start_angle = np.dot(centre_start_direction, np.array([0, 1, 0]))
    start_angle = np.arccos(cos_start_angle)
    if start[0] < centre[0]:
        start_angle = 2*np.pi - start_angle

    end_angle = start_angle - bulge_sign*theta
    
    return centre, radius, start_angle, end_angle

def trace_minimal_arc(centre, radius, start_angle, end_angle, elevation, eps):
    '''
    Express an arc with the minimum number of points such that we have |approx - true| < eps

    Output: 
        points: np.array of shape (n, 3)
    '''
    inc = find_minimum_angle_increment(radius, eps)
    
    angles = np.arange(start_angle, end_angle, inc)
    points = np.zeros((len(angles), 3))
    for i, angle in enumerate(angles):
        points[i] = centre + radius * np.array([np.cos(angle), np.sin(angle), 0])
    
    return points

def find_minimum_angle_increment(radius, eps):
    return np.sqrt(8*eps/radius)
