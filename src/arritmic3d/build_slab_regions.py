import json
import os
import numpy as np



def _get_tissue_write_indices(grid, flat_inds_all, n_points):
    """
    Returns indices inside tissue (restitution_model != 0) or None if no tissue.
    """
    if "restitution_model" not in grid.point_data:
        raise ValueError("Field 'restitution_model' not found in grid.point_data; required to determine tissue mask.")
    rm = grid.point_data["restitution_model"]
    if rm.ndim != 1 or rm.size != n_points:
        raise ValueError("Field 'restitution_model' has unexpected shape.")
    tissue_mask = rm[flat_inds_all] != 0
    if not np.any(tissue_mask):
        return None
    return flat_inds_all[tissue_mask]


def _write_field_values(grid, field, val, write_inds, n_points):
    """
    Create/validate the target field array in grid.point_data and write `val` at indices `write_inds`.
    - field: field name (string)
    - val: scalar or vector-like
    - write_inds: 1D array of flat point indices to write into
    - n_points: total number of points in grid
    """
    is_vector = isinstance(val, (list, tuple, np.ndarray)) and not isinstance(val, (str, bytes))

    if field in grid.point_data:
        current = grid.point_data[field].copy()
        # existing scalar
        if current.ndim == 1:
            if is_vector:
                raise ValueError(f"Field '{field}' exists as scalar but region provides vector value.")
            target = current
        else:
            # existing vector
            if not is_vector:
                raise ValueError(f"Field '{field}' exists as vector but region provides scalar value.")
            comps = current.shape[1]
            if len(val) != comps:
                raise ValueError(f"Field '{field}' expects {comps} components, region provides {len(val)}.")
            target = current
    else:
        # create field based on val type
        if is_vector:
            comps = len(val)
            target = np.zeros((n_points, comps), dtype=float)
        else:
            target = np.zeros(n_points, dtype=int) if isinstance(val, int) else np.zeros(n_points, dtype=float)

    # perform assignment
    if is_vector:
        arr_val = np.array(val, dtype=float)
        target[write_inds] = arr_val
    else:
        target[write_inds] = val

    grid.point_data[field] = target


def set_region(grid, center, radius, data, shape="circle"):
    """Write the values in `data` into grid.point_data for points inside the 2D mask.

    - center: (cx, cy) in world distance units
    - radius: scalar radius in world units (applies to the mask generation)
    - data: dict { field_name: value } where value is scalar or list-like (vector)
    - shape: "circle", "square" or "diamond" (determines mask metric)
    """
    cx, cy = center
    x_coords = np.asarray(grid.x)
    y_coords = np.asarray(grid.y)

    # Create 2D coordinate arrays (i -> x, j -> y)
    X, Y = np.meshgrid(x_coords, y_coords, indexing='ij')

    r = float(radius)

    if shape == "circle":
        dist2d = np.sqrt((X - cx) ** 2 + (Y - cy) ** 2)
    elif shape == "square":
        # Chebyshev distance: max(|dx|, |dy|) <= r  => square of half-side r
        dist2d = np.maximum(np.abs(X - cx), np.abs(Y - cy))
    elif shape == "diamond":
        # Manhattan distance: |dx| + |dy| <= r  => diamond shape
        dist2d = np.abs(X - cx) + np.abs(Y - cy)
    else:
        raise ValueError("Unsupported shape: choose 'circle', 'square' or 'diamond'")

    mask2d = dist2d <= r
    i_inds, j_inds = np.nonzero(mask2d)

    nx = x_coords.size
    ny = y_coords.size
    nz = grid.dimensions[2]

    # compute flat indices for all z layers
    flat_inds_all = []
    for k in range(nz):
        # compute flat indices for this z layer
        # ravel_multi_index expects tuple of arrays in order (i,j,k) and returns flat indices
        ks = np.full_like(i_inds, k)
        flat_inds = np.ravel_multi_index((i_inds, j_inds, ks), (nx, ny, nz), order="F")
        flat_inds_all.append(flat_inds)
    flat_inds_all = np.concatenate(flat_inds_all)

    n_points = grid.number_of_points
    write_inds = _get_tissue_write_indices(grid, flat_inds_all, n_points)
    if write_inds is None:
        return

    # For each field in data, prepare target array (create if necessary) and assign value(s)
    for field, val in data.items():
        _write_field_values(grid, field, val, write_inds, n_points)



def load_regions_from_file(path):
    """Load regions list from a JSON file. Raise ValueError on invalid format."""
    if not os.path.isfile(path):
        raise ValueError(f"Regions file not found: {path}")
    try:
        with open(path, "r") as fh:
            data = json.load(fh)
    except Exception as e:
        raise ValueError(f"Error reading regions file '{path}': {e}")
    if not isinstance(data, list):
        raise ValueError("Regions file must contain a JSON list of region objects.")
    for i, item in enumerate(data):
        if not isinstance(item, dict):
            raise ValueError(f"Regions file: element {i} is not an object.")
    return data


def parse_regions_from_cli(region_strings):
    """Parse --region occurrences (JSON strings). Raise ValueError on parse error."""
    parsed = []
    if not region_strings:
        return parsed
    for i, s in enumerate(region_strings):
        try:
            obj = json.loads(s)
        except Exception as e:
            raise ValueError(f"Error parsing --region argument #{i}: invalid JSON ({e})")
        if not isinstance(obj, dict):
            raise ValueError(f"--region argument #{i} must be a JSON object.")
        parsed.append(obj)
    return parsed


def validate_region(region, index):
    """Validate a single region dict. Raises ValueError if invalid.

    Schemas (strict, no defaults):
      - circle: {shape, cx, cy, r1, r2, <field>: scalar|vector|[scalar|vector, ...]}
      - square: {shape, cx, cy, r1, r2, <field>: scalar|vector|[scalar|vector, ...]}
      - diamond: {shape, cx, cy, r1, r2, <field>: scalar|vector|[scalar|vector, ...]}
      - side: {shape, side, <field>: scalar|vector}
      - node_ids: {shape, ids, <field>: scalar|vector}

    For circle/square/diamond: field values can be scalar, vector, or list (gradient).
    For side/node_ids: field values must be scalar or vector (no gradients).
    """
    if "shape" not in region:
        raise ValueError(f"Region {index}: missing required field 'shape'.")
    shape = str(region["shape"]).lower()

    if shape in ("circle", "square", "diamond"):
        required = ["cx", "cy", "r1", "r2"]
    elif shape == "side":
        required = ["side"]
    elif shape == "node_ids":
        required = ["ids"]
    else:
        raise ValueError(f"Region {index}: unknown shape '{region.get('shape')}'. Supported: circle,square,diamond,side,node_ids.")

    missing = [k for k in required if k not in region]
    if missing:
        raise ValueError(f"Region {index} (shape='{shape}'): missing required fields {missing}.")

    # Additional validation for side and node_ids
    if shape == "side":
        side = str(region["side"]).lower()
        if side not in {"north", "south", "east", "west"}:
            raise ValueError(f"Region {index} (shape='side'): side must be one of north,south,east,west. Got '{side}'.")
    elif shape == "node_ids":
        ids = region["ids"]
        if not isinstance(ids, (list, tuple)):
            raise ValueError(f"Region {index} (shape='node_ids'): 'ids' must be a list or tuple.")
        if len(ids) == 0:
            raise ValueError(f"Region {index} (shape='node_ids'): 'ids' cannot be empty.")
        for i, nid in enumerate(ids):
            try:
                int(nid)
            except Exception:
                raise ValueError(f"Region {index} (shape='node_ids'): ids[{i}] must be an integer.")

    # Determine target fields
    known = set(required + ["shape"])
    target_fields = [k for k in region.keys() if k not in known]
    if not target_fields:
        raise ValueError(f"Region {index}: no target fields provided (e.g. 'restitution_model' or 'fibers_orientation').")

    # Unified validation for all shapes
    for field in target_fields:
        val = region[field]
        _validate_field_value(field, val, shape, index)


def _validate_field_value(field, val, shape, region_index):
    """
    Validate a single field value across all shape types.
    - For circle/square/diamond: allow scalar, vector, or list of values (gradient)
    - For side/node_ids: allow scalar or vector only (no gradients)
    """
    supports_gradient = shape in ("circle", "square", "diamond")

    if isinstance(val, (list, tuple)):
        if len(val) == 0:
            raise ValueError(f"Region {region_index}: field '{field}' cannot be empty.")

        # If any element is list/tuple/ndarray => vector entry (no gradient)
        contains_nested = any(isinstance(elem, (list, tuple, np.ndarray)) for elem in val)

        if supports_gradient and not contains_nested:
            # All elements are scalar: treat as gradient only if len > 1
            if len(val) == 1:
                # Single-scalar list: treated as uniform value
                try:
                    float(val[0])
                except Exception:
                    raise ValueError(f"Region {region_index}: field '{field}'[0] must be numeric.")
            else:
                # Gradient: all elements must be numeric
                for i, elem in enumerate(val):
                    try:
                        float(elem)
                    except Exception:
                        raise ValueError(f"Region {region_index}: field '{field}'[{i}] must be numeric.")
        else:
            # side/node_ids or nested list: interpreted as uniform vector
            try:
                for elem in val:
                    float(elem) if not isinstance(elem, (list, tuple, np.ndarray)) else None
            except Exception:
                raise ValueError(f"Region {region_index}: field '{field}' vector elements must be numeric.")
    else:
        try:
            float(val)
        except Exception:
            raise ValueError(f"Region {region_index}: field '{field}' must be numeric, vector, or list.")


def apply_region(grid, region):
    """Apply a validated region dict to the grid.

    For circle/square/diamond: supports gradients. Single values are applied uniformly;
    lists form gradients with interpolated radii between r1 and r2.
    For side: defines region by slab side.
    For node_ids: defines region by specific nodes.
    """
    shape = str(region["shape"]).lower()
    known = {"shape", "cx", "cy", "r1", "r2", "side", "ids"}
    targets = {k: region[k] for k in region.keys() if k not in known}

    if shape in ("circle", "square", "diamond"):
        cx = float(region["cx"])
        cy = float(region["cy"])
        center = (cx, cy)
        r1 = float(region["r1"])
        r2 = float(region["r2"])
        _apply_shape_with_gradient(grid, shape, center, r1, r2, targets)
    elif shape == "side":
        side = str(region["side"]).lower()
        _apply_side_region(grid, side, targets)
    elif shape == "node_ids":
        ids = [int(nid) for nid in region["ids"]]
        _apply_node_ids_region(grid, ids, targets)


def _apply_shape_with_gradient(grid, shape, center, r1, r2, targets):
    """
    Apply a geometric shape (circle, square, diamond) with optional gradient support.

    Parameters:
    - grid: pyvista.UnstructuredGrid
    - shape: "circle", "square", or "diamond"
    - center: (cx, cy)
    - r1: inner radius (for gradient start)
    - r2: outer radius, r1 <= r2 (for gradient end)
    - targets: dict { field_name: value }

    Behavior:
    - Scalar: applies uniformly value to entire region.
    - List: gradient from r1 to r2
    - If r1==r2, then only the first element is used.
    - If scalar is provided, applies uniformly using r2.
    """
    if r1 > r2:
        raise ValueError("r1 must be less than or equal to r2.")

    for field, val in targets.items():
        # Scalar or r1==r2 (single mask): apply only the first value
        if not isinstance(val, (list, tuple)) or r1 == r2:
            v = val[0] if isinstance(val, (list, tuple)) else val
            data = {field: v}
            set_region(grid, center=center, radius=r2, data=data, shape=shape)
        else:
            # List: gradient from r1 to r2
            n_layers = len(val)
            radii = np.linspace(r1, r2, n_layers)
            for i in range(n_layers - 1, -1, -1):
                data = {field: val[i]}
                set_region(grid, center=center, radius=radii[i], data=data, shape=shape)


def _apply_side_region(grid, side, targets):
    """
    Apply field values to an side (side: north,south,east,west).
    Uses the same logic as region_by_side from build_slab.
    """
    # Validate side
    if side not in {"north", "south", "east", "west"}:
        raise ValueError("side must be one of: north, south, east, west")

    # Determine interior sizes considering padding offset=1
    dims = grid.dimensions
    offset = 1
    nx = dims[0] - 2*offset
    ny = dims[1] - 2*offset
    nz = dims[2] - 2*offset

    # Map side to region_type and index
    if side == "north":
        region_type = "row"
        index_int = ny - 1
    elif side == "east":
        region_type = "column"
        index_int = nx - 1
    elif side == "west":
        region_type = "column"
        index_int = 0
    else:  # south
        region_type = "row"
        index_int = 0

    # Compute node ids (reuse helper from build_slab if available; otherwise inline)
    # For simplicity, inline the logic here
    x_coords = np.asarray(grid.x)
    y_coords = np.asarray(grid.y)

    if region_type == "row":
        # y fixed, x varies
        y_indices = [index_int]
        x_indices = list(range(nx))
    else:  # column
        # x fixed, y varies
        x_indices = [index_int]
        y_indices = list(range(ny))

    z_indices = list(range(nz))

    flat_inds_all = []
    for z in z_indices:
        for y in y_indices:
            for x in x_indices:
                idx = np.ravel_multi_index((x + offset, y + offset, z + offset), dims, order="F")
                flat_inds_all.append(idx)
    flat_inds_all = np.array(flat_inds_all)

    n_points = grid.number_of_points
    write_inds = _get_tissue_write_indices(grid, flat_inds_all, n_points)
    if write_inds is None:
        return

    # write targets to fields
    for field, val in targets.items():
        _write_field_values(grid, field, val, write_inds, n_points)


def _apply_node_ids_region(grid, ids, targets):
    """
    Apply field values to explicitly specified node IDs.
    """
    n_points = grid.number_of_points
    # filter tissue
    write_inds = _get_tissue_write_indices(grid, np.array(ids), n_points)
    if write_inds is None:
        return

    # write targets to fields
    for field, val in targets.items():
        _write_field_values(grid, field, val, write_inds, n_points)