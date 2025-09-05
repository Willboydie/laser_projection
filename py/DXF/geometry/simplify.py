import numpy as np
from typing import List, Tuple, Optional
from collections import defaultdict, deque
from scipy.spatial import KDTree


#=============== TSP ===================
def _endpoint_2d(p):
	return (float(p[0]), float(p[1]))

def _path_endpoints_2d(path):
	"""Return ((sx, sy), (ex, ey)) for a non-empty path."""
	s = _endpoint_2d(path[0])
	e = _endpoint_2d(path[-1])
	return s, e

def _euclid2(a, b):
	ax, ay = a
	bx, by = b
	return ((ax - bx) ** 2 + (ay - by) ** 2) ** 0.5

def _materialize(polylines, order):
	"""Build the new ordered list given [(idx, reversed_flag), ...]."""
	out = []
	for idx, rev in order:
		path = polylines[idx]
		if not path:
			continue
		out.append(list(reversed(path)) if rev else path)
	return out

def _total_link_cost(order, endpoints):
	"""Compute total connection cost between consecutive polylines in order.
	endpoints[idx] = ((sx,sy),(ex,ey))
	"""
	if len(order) <= 1:
		return 0.0
	cost = 0.0
	# track the current exit endpoint given orientation
	prev_idx, prev_rev = order[0]
	_, prev_end = endpoints[prev_idx]
	prev_exit = endpoints[prev_idx][0] if prev_rev else endpoints[prev_idx][1]
	for k in range(1, len(order)):
		idx, rev = order[k]
		entry = endpoints[idx][1] if rev else endpoints[idx][0]
		cost += _euclid2(prev_exit, entry)
		prev_exit = endpoints[idx][0] if rev else endpoints[idx][1]
	return cost

def reorder_polylines(polylines: List[List[List[float]]], use_kdtree: bool = True, two_opt: bool = True):
	"""
	Reorder polylines using a nearest-neighbour heuristic, avoiding input mutation.
	- Tries both orientations for each candidate (normal/reversed) and chooses cheaper.
	- Optionally accelerates nearest lookup with KDTree (if SciPy available).
	- Optionally applies a simple 2-opt pass to reduce total travel.
	Returns: list of polylines in the chosen order (with orientation applied).
	"""
	n = len(polylines)
	if n <= 1:
		return polylines

	# precompute endpoints
	endpoints = {}
	for i, path in enumerate(polylines):
		if not path:
			continue
		endpoints[i] = _path_endpoints_2d(path)

	valid_indices = [i for i in range(n) if i in endpoints]
	if len(valid_indices) <= 1:
		return polylines

	# Build KDTree over BOTH endpoints for all paths for faster queries
	tree = None
	index_map = []  # maps tree row -> (poly_idx, side) where side 0=start,1=end
	if use_kdtree and KDTree is not None:
		pts = []
		for i in valid_indices:
			s, e = endpoints[i]
			pts.append(s); index_map.append((i, 0))
			pts.append(e); index_map.append((i, 1))
		pts = [(x, y) for (x, y) in pts]
		try:
			tree = KDTree(pts)
		except Exception:
			tree = None

	best_cost = float("inf")
	best_order: Optional[List[Tuple[int, bool]]] = None

	for start_idx in valid_indices:
		visited = {start_idx}
		# store (poly_idx, reversed_flag)
		order: List[Tuple[int, bool]] = [(start_idx, False)]
		# current exit endpoint (depends on current reversed flag)
		cur_exit = endpoints[start_idx][1]

		cost = 0.0
		for _ in range(len(valid_indices) - 1):
			pick = None
			min_d = float("inf")

			if tree is not None:
				# Query a growing number of neighbors until we find an unvisited poly index
				k = 4
				found = False
				while not found and k <= len(index_map):
					distances, nn_idx = tree.query([cur_exit], k=min(k, len(index_map)))
					# ensure iterable
					nn = nn_idx[0] if hasattr(nn_idx, "__len__") else [nn_idx]
					dists = distances[0] if hasattr(distances, "__len__") else [distances]
					for j, d in zip(nn, dists):
						poly_j, side = index_map[int(j)]
						if poly_j in visited:
							continue
						# try both orientations for candidate
						cand_entry = endpoints[poly_j][1] if side == 1 else endpoints[poly_j][0]
						d_keep = _euclid2(cur_exit, endpoints[poly_j][0])
						d_rev  = _euclid2(cur_exit, endpoints[poly_j][1])
						if d_keep < min_d:
							min_d, pick = d_keep, (poly_j, False)
						if d_rev < min_d:
							min_d, pick = d_rev, (poly_j, True)
						found = True
					if not found:
						k *= 2
				# if still not found (degenerate), fallback to linear scan
			if pick is None:
				for j in valid_indices:
					if j in visited:
						continue
					d_keep = _euclid2(cur_exit, endpoints[j][0])
					d_rev  = _euclid2(cur_exit, endpoints[j][1])
					if d_keep < min_d:
						min_d, pick = d_keep, (j, False)
					if d_rev < min_d:
						min_d, pick = d_rev, (j, True)

			# apply pick
			cost += min_d
			visited.add(pick[0])
			order.append(pick)
			# update current exit endpoint
			cur_exit = endpoints[pick[0]][0] if pick[1] else endpoints[pick[0]][1]

		if cost < best_cost:
			best_cost = cost
			best_order = order

	# Optional 2-opt on the sequence (simplified): try swapping pairs if it reduces link cost
	if two_opt and best_order and len(best_order) > 3:
		improved = True
		while improved:
			improved = False
			for i in range(1, len(best_order) - 2):
				for j in range(i + 1, len(best_order) - 1):
					cand = best_order[:i] + list(reversed(best_order[i:j])) + best_order[j:]
					if _total_link_cost(cand, endpoints) + 1e-12 < _total_link_cost(best_order, endpoints):
						best_order = cand
						improved = True
						break
				if improved:
					break

	return _materialize(polylines, best_order)



def resample_by_direction_change(polyline, base_step=0.05, min_step=0.00001, max_step=0.05, alpha=5.0):
	"""
	Adaptive resampling of a polyline based on local directional change.
	- base_step: nominal spacing on straight segments
	- min_step / max_step: clamps for spacing
	- alpha: scales sensitivity to direction change (larger => denser at turns)
	- Handles both 3D and 4D points, preserving the 4th dimension (laser_on flag)
	"""
	if len(polyline) < 3:
		return polyline

	# Handle 4D points by separating spatial coordinates from the 4th dimension
	is_4d = len(polyline[0]) == 4
	if is_4d:
		# Extract spatial coordinates for calculations
		pts_spatial = np.asarray([[p[0], p[1], p[2]] for p in polyline], dtype=float)
		# Extract 4th dimension (laser_on flags)
		laser_flags = np.asarray([p[3] for p in polyline], dtype=float)
	else:
		pts_spatial = np.asarray(polyline, dtype=float)
		laser_flags = None

	# Calculate segments and lengths using spatial coordinates only
	seg = pts_spatial[1:] - pts_spatial[:-1]
	seg_len = np.linalg.norm(seg, axis=1)
	# avoid zero-length issues
	seg_len = np.where(seg_len == 0.0, 1e-12, seg_len)
	tan = seg / seg_len[:, None]

	# angle between successive segment tangents (0 = straight, pi = U-turn)
	dot = np.sum(tan[1:] * tan[:-1], axis=1)
	angles = np.arccos(np.clip(dot, -1.0, 1.0))  # length = len(seg)-1

	# Convert vertex angles to per-segment weights (split between adjacent segments)
	seg_weight = np.zeros_like(seg_len)
	if len(angles) > 0:
		seg_weight[1:] += angles * 0.5
		seg_weight[:-1] += angles * 0.5

	# Adaptive step per segment
	step_per_seg = base_step / (1.0 + alpha * seg_weight)
	step_per_seg = np.clip(step_per_seg, min_step, max_step)

	# Start with first point
	if is_4d:
		out = [polyline[0]]  # Keep original 4D point
	else:
		out = [pts_spatial[0].tolist()]

	for i in range(len(seg_len)):
		L = seg_len[i]
		if L <= 1e-12:
			continue
		d = seg[i] / L
		step = float(step_per_seg[i])

		s = step
		while s < L - 1e-12:
			# Interpolate spatial coordinates
			interp_point = pts_spatial[i] + d * s
			
			if is_4d:
				# Interpolate laser flag between adjacent points
				# Use the laser flag from the starting point of this segment
				laser_flag = laser_flags[i]
				out.append([float(interp_point[0]), float(interp_point[1]), float(interp_point[2]), float(laser_flag)])
			else:
				out.append(interp_point.tolist())
			s += step

	# ensure we end exactly at the last vertex
	last_point = polyline[-1] if is_4d else pts_spatial[-1].tolist()
	if out[-1] != last_point:
		out.append(last_point)

	return out


def close_single_polyline(polyline):
	"""
	Close a polyline by connecting its last point to its first point.
	- Uses module-level spatial step 'increment' if available (fallback 0.01).
	- Treats nearly closed polylines as closed within a small tolerance.
	- Produces 4D points [x, y, z, laser_on] where laser_on=1 for original points,
	  laser_on=0 for closure points.
	"""
	if not polyline or len(polyline) < 2:
		# Return a single 4D point if polyline is too short
		if not polyline:
			return [[0.0, 0.0, 0.0, 1.0]]
		# Convert the single point to 4D format
		point = polyline[0]
		if len(point) == 3:
			return [[point[0], point[1], point[2], 1.0]]
		elif len(point) == 4:
			return [point]
		else:  # 2D point
			return [[point[0], point[1], 0.0, 1.0]]

	# Convert input to 4D format if needed
	if len(polyline[0]) == 3:
		polyline_4d = [[point[0], point[1], point[2], 1.0] for point in polyline]
	elif len(polyline[0]) == 4:
		polyline_4d = polyline.copy()
	else:
		# Handle 2D points
		polyline_4d = [[point[0], point[1], 0.0, 1.0] for point in polyline]

	start = polyline_4d[0][:3]  # Extract x,y,z coordinates
	end = polyline_4d[-1][:3]

	x0, y0 = float(end[0]), float(end[1])
	x1, y1 = float(start[0]), float(start[1])
	z0 = float(end[2]) if len(end) > 2 else 0.0
	z1 = float(start[2]) if len(start) > 2 else 0.0

	dx = x1 - x0
	dy = y1 - y0
	length = (dx * dx + dy * dy) ** 0.5

	# If already (nearly) closed, snap last point to the first and return
	tol = 1e-12
	if length <= tol:
		# ensure dimensional consistency; do not mutate input
		start_xyz = [x1, y1, z1, 1]
		# Always return 4D points, replacing the last point with the start point
		return polyline_4d[:-1] + [start_xyz]

	# Determine spatial step
	try:
		step = float(increment)
	except Exception:
		step = 0.01
	if step <= 0.0:
		step = 0.01

	# Number of interior samples between end (t=0) and start (t=1)
	# We include the final start point explicitly to ensure closure
	import math
	n_interior = max(int(math.ceil(length / step)) - 1, 0)

	closed_points = []
	if n_interior > 0:
		ts = np.linspace(0.0, 1.0, num=n_interior + 2, endpoint=True)[1:-1]  # exclude 0 and 1
		xs = x0 + ts * dx
		ys = y0 + ts * dy
		zs = z0 + ts * (z1 - z0)
		closed_points.extend([[float(xs[i]), float(ys[i]), float(zs[i]), 0] for i in range(len(ts))])

	# Append the exact starting vertex to explicitly close the loop
	closed_points.append([x1, y1, z1, 1.0])

	return polyline_4d + closed_points



def join_polylines(vectors, tolerance):
    """
    Join fragmented polylines whose endpoints are within a given tolerance.
    Scalable approach:
    - Quantize points to tolerance grid and snap to canonical average per cell
    - Build endpoint index for O(1) joins
    - Grow polylines from both ends, merging coincident points
    - Remove collinear points from joined polylines to keep only actual vertices
    - Avoid mutating inputs

    vectors: list[list[list[float]]]  # list of polylines, each is list of 3D points
    tolerance: float
    """

    if len(vectors) <= 1:
        return vectors

    tol = float(tolerance)

    def _key(p):
        x, y = p[0], p[1]
        z = p[2] if len(p) > 2 else 0.0
        return (round(x / tol), round(y / tol), round(z / tol))

    # 1) Gather all points into quantized bins and compute canonical averages
    bins = defaultdict(list)
    for vec in vectors:
        if len(vec) == 0:
            continue
        for p in vec:
            bins[_key(p)].append(p)

    canonical = {}
    for k, pts in bins.items():
        arr = np.asarray(pts, dtype=float)
        mean = arr.mean(axis=0)
        if mean.shape[0] == 2:
            mean = np.array([mean[0], mean[1], 0.0], dtype=float)
        canonical[k] = mean.tolist()

    # 2) Snap all points to their canonical representative and drop duplicates
    snapped_vectors = []
    for vec in vectors:
        if len(vec) == 0:
            snapped_vectors.append([])
            continue
        snapped = []
        last_key = None
        for p in vec:
            k = _key(p)
            cp = canonical.get(k)
            if cp is None:
                cp = [p[0], p[1], p[2] if len(p) > 2 else 0.0]
            if k != last_key:
                snapped.append(cp)
                last_key = k
        snapped_vectors.append(snapped)

    # 3) Build endpoint index: key -> set of (vector_index, endpoint_side)
    # endpoint_side: 0 for start, 1 for end
    endpoint_index = defaultdict(set)
    for idx, vec in enumerate(snapped_vectors):
        if len(vec) == 0:
            continue
        start_key = _key(vec[0])
        end_key = _key(vec[-1])
        endpoint_index[start_key].add((idx, 0))
        endpoint_index[end_key].add((idx, 1))

    unvisited = set(i for i, v in enumerate(snapped_vectors) if v)
    polylines = []

    def _remove_endpoint(idx, side):
        vec = snapped_vectors[idx]
        if len(vec) == 0:
            return
        key = _key(vec[0] if side == 0 else vec[-1])
        s = endpoint_index.get(key)
        if s and (idx, side) in s:
            s.discard((idx, side))
            if not s:
                endpoint_index.pop(key, None)

    while unvisited:
        cur_idx = unvisited.pop()
        cur = deque(snapped_vectors[cur_idx])
        _remove_endpoint(cur_idx, 0)
        _remove_endpoint(cur_idx, 1)

        extended = True
        while extended:
            extended = False

            # Extend at head
            head_key = _key(cur[0])
            candidates = list(endpoint_index.get(head_key, ()))
            attached = False
            for idx, side in candidates:
                if idx not in unvisited:
                    continue
                vec = snapped_vectors[idx]
                if side == 0:  # their start matches our head -> reverse
                    seq = list(reversed(vec))
                else:          # their end matches our head -> keep
                    seq = vec
                seq_to_add = seq[:-1]
                _remove_endpoint(idx, 0)
                _remove_endpoint(idx, 1)
                unvisited.discard(idx)
                for p in reversed(seq_to_add):
                    cur.appendleft(p)
                extended = True
                attached = True
                break
            if attached:
                continue

            # Extend at tail
            tail_key = _key(cur[-1])
            candidates = list(endpoint_index.get(tail_key, ()))
            for idx, side in candidates:
                if idx not in unvisited:
                    continue
                vec = snapped_vectors[idx]
                if side == 1:  # their end matches our tail -> reverse
                    seq = list(reversed(vec))
                else:          # their start matches our tail -> keep
                    seq = vec
                seq_to_add = seq[1:]
                _remove_endpoint(idx, 0)
                _remove_endpoint(idx, 1)
                unvisited.discard(idx)
                for p in seq_to_add:
                    cur.append(p)
                extended = True
                break

        if len(cur) >= 2 and _key(cur[0]) == _key(cur[-1]):
            cur.pop()

        polylines.append(np.array(cur))

    # 4) Remove collinear points from the joined polylines
    def _are_collinear(p1, p2, p3, tolerance=1e-6):
        """Check if three points are collinear using cross product."""
        if len(p1) < 2 or len(p2) < 2 or len(p3) < 2:
            return False
        
        # Convert to 3D points
        pt1 = np.array([p1[0], p1[1], p1[2] if len(p1) > 2 else 0.0])
        pt2 = np.array([p2[0], p2[1], p2[2] if len(p2) > 2 else 0.0])
        pt3 = np.array([p3[0], p3[1], p3[2] if len(p3) > 2 else 0.0])
        
        # Vectors from p1 to p2 and p2 to p3
        v1 = pt2 - pt1
        v2 = pt3 - pt2
        
        # Cross product magnitude
        cross = np.linalg.norm(np.cross(v1, v2))
        return cross < tolerance

    cleaned_polylines = []
    for polyline in polylines:
        if len(polyline) < 3:
            cleaned_polylines.append(polyline)
            continue
        
        # Convert to list for easier manipulation
        points = polyline.tolist()
        cleaned = [points[0]]  # Always keep first point
        
        for i in range(1, len(points) - 1):
            # Check if current point is collinear with previous and next
            if not _are_collinear(points[i-1], points[i], points[i+1], tol):
                cleaned.append(points[i])
        
        cleaned.append(points[-1])  # Always keep last point
        cleaned_polylines.append(np.array(cleaned))

    return cleaned_polylines


