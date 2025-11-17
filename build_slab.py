import pyvista as pv
import numpy as np
import argparse
import json
import os

# Generate list of node ids to activate the tissue
def get_activation_node_ids(grid, region_type="row", index=0, z_mode="zero", front_mode="all", nx=None, ny=None, nz=None, add_layer=True):
    """
    Returns a list of node ids for the tissue region, using block size and add_layer info.

    Parameters:
        grid (pv.RectilinearGrid): The grid to search.
        region_type (str): Region type ("row", "column", "point").
        index (int, tuple, or str): Row/column index or point coordinates (can be string for parsing).
        z_mode (str or int): "zero" for Z=0, "center" for central Z, "all" for all Z, or an integer for specific Z.
        front_mode (str or int): "zero", "center", "all", or an integer for specific X/Y.
        nx, ny, nz (int): Number of divisions in X, Y, Z (without extra layer).
        add_layer (bool): Whether the grid has an extra layer.

    Returns:
        list: List of node ids (only tissue, never -1).
    """
    dims = grid.dimensions  # (nx+2*offset, ny+2*offset, nz+2*offset)
    offset = 1 if add_layer else 0

    # Infer nx, ny, nz if not provided
    if nx is None or ny is None or nz is None:
        nx = dims[0] - 2*offset
        ny = dims[1] - 2*offset
        nz = dims[2] - 2*offset

    # Parse index if string
    if region_type == "point":
        if isinstance(index, str):
            index = tuple(map(int, index.strip("()").split(",")))
        if not (isinstance(index, tuple) and len(index) == 3):
            raise ValueError("For 'point', index must be a tuple (x, y, z)")
    else:
        if isinstance(index, str):
            index = int(index)

    # Parse z_mode
    if isinstance(z_mode, str):
        try:
            z_mode_int = int(z_mode)
            z_mode = z_mode_int
        except ValueError:
            pass


    # Determine Z indices to use (all refer to tissue region)
    if isinstance(z_mode, int):
        z_indices = [z_mode]
    elif z_mode == "zero":
        z_indices = [0]
    elif z_mode == "center":
        z_indices = [nz // 2]
    elif z_mode == "all":
        z_indices = list(range(nz))
    else:
        raise ValueError("Invalid z_mode value")

    # Determine x/y indices to use (ranges depend on region_type and front_mode)
    if region_type == "row":
        if front_mode == "zero":
            x_indices = [0]
        elif front_mode == "center":
            x_indices = [nx // 2]
        elif front_mode == "all":
            x_indices = list(range(nx))
        else:
            try:
                front_mode_int = int(front_mode)
                x_indices = [front_mode_int]
            except ValueError:
                raise ValueError("Invalid front_mode value")
        # For row, y is fixed
        y_indices = [index]
    elif region_type == "column":
        if front_mode == "zero":
            y_indices = [0]
        elif front_mode == "center":
            y_indices = [ny // 2]
        elif front_mode == "all":
            y_indices = list(range(ny))
        else:
            try:
                front_mode_int = int(front_mode)
                y_indices = [front_mode_int]
            except ValueError:
                raise ValueError("Invalid front_mode value")
        # For column, x is fixed
        x_indices = [index]
    elif region_type == "point":
        x_indices = [index[0]]
        y_indices = [index[1]]
        z_indices = [index[2]]
    else:
        raise ValueError("Invalid region_type value")

    # Collect node ids
    ids = []
    for z in z_indices:
        for y in y_indices:
            for x in x_indices:
                idx = np.ravel_multi_index((x + offset, y + offset, z + offset), dims, order="F")
                ids.append(idx)
    return ids


def set_stimulation_sites(
    grid,
    region_type="row",
    index=0,
    z_mode="zero",
    front_mode="all",
    add_layer=True
):
    """
    Sets the 'stimulation_sites' field in the grid and returns the activation sites dictionary.

    Parameters:
        grid (pv.RectilinearGrid): The grid to modify.
        region_type (str): Region type ("row", "column", "point").
        index (int, tuple, or str): Row/column index or point coordinates.
        z_mode (str or int): Z mode for stim region.
        front_mode (str or int): "zero", "center", "all", or an integer for specific X/Y.
        nx, ny, nz (int): Number of divisions in X, Y, Z (without extra layer).
        add_layer (bool): Whether the grid has an extra layer.

    Returns:
        dict: Dictionary with activation node ids.
    """
    activation_node_ids = get_activation_node_ids(
        grid,
        region_type=region_type,
        index=index,
        z_mode=z_mode,
        front_mode=front_mode,
        add_layer=add_layer
    )
    print("Node ids to activate:", activation_node_ids)
    activation_node_ids = [int(i) for i in activation_node_ids]
    stimulation_sites = np.zeros(grid.number_of_points, dtype=int)
    for idx in activation_node_ids:
        stimulation_sites[idx] = 1
    grid.point_data["stimulation_sites"] = stimulation_sites
    return {"STIMULATION_SITES": activation_node_ids}


def set_center_square_restitution_model(grid, square_size=3):
    """
    Sets 'restitution_model' = 2 for a square region in the center of the XY plane at all Z layers,
    but only for tissue nodes (restitution_model != -1).
    """
    x_len = grid.dimensions[0]
    y_len = grid.dimensions[1]
    z_len = grid.dimensions[2]

    x_center = x_len // 2
    y_center = y_len // 2
    half = square_size // 2

    if "restitution_model" in grid.point_data:
        cell_type = grid.point_data["restitution_model"].copy()
    else:
        cell_type = np.zeros(grid.number_of_points, dtype=int)

    for k in range(z_len):
        for j in range(y_center - half, y_center + half + (square_size % 2)):
            for i in range(x_center - half, x_center + half + (square_size % 2)):
                if 0 <= i < x_len and 0 <= j < y_len:
                    idx = np.ravel_multi_index((i, j, k), (x_len, y_len, z_len), order="F")
                    if cell_type[idx] != 0:  # Only set if not exterior
                        cell_type[idx] = 2

    grid.point_data["restitution_model"] = cell_type

def generate_rectilinear_slab(ndivs, spacing=(1.0, 1.0, 1.0),
                              field_data=None, fallback_value_scalar=np.nan,
                              fallback_value_vector=0.0, field_defaults=None, add_layer=True):
    """
    Generates a rectilinear grid (slab) with specified number of divisions and assigns optional point data.

    Parameters:
        ndivs (tuple): (nx_divs, ny_divs, nz_divs) number of divisions along each axis.
        spacing (tuple): (dx, dy, dz) spacing between grid points.
        field_data (dict): Optional. Dictionary of field_name: ndarray to assign as point data.
        fallback_value_scalar (float): Default value for missing scalar data.
        fallback_value_vector (float): Default value for missing vector data.
        field_defaults (dict): Optional. Specific default values for fields.
        add_layer (bool): If True, adds an extra layer at each edge of each axis.

    Returns:
        pv.RectilinearGrid: The generated rectilinear grid.
    """
    def make_coords(n_divs, step):
        """Generates coordinates for one axis, adding extra layers if specified."""
        if add_layer:
            # n_divs + 2 nodes: from -1 to n_divs (exclusive)
            coords = np.arange(-1, n_divs + 1) * step
        else:
            # n_divs nodes: from 0 to n_divs-1
            coords = np.arange(n_divs) * step
        return coords

    def make_mask(n_divs):
        """Create an axis mask where 1 indicates tissue (interior) and 0 indicates exterior layer."""
        if add_layer:
            # interior nodes are ones, exterior padding (first and last) are zeros
            mask = np.concatenate(([0], np.ones(n_divs, dtype=int), [0]))
        else:
            # no extra layer: all nodes are tissue
            mask = np.ones(n_divs, dtype=int)
        return mask

    x_coords = make_coords(ndivs[0], spacing[0])
    y_coords = make_coords(ndivs[1], spacing[1])
    z_coords = make_coords(ndivs[2], spacing[2])

    x_mask = make_mask(ndivs[0])
    y_mask = make_mask(ndivs[1])
    z_mask = make_mask(ndivs[2])

    # Combine masks so that a cell is tissue only if it's interior on all three axes
    # use logical_and to get True only where all axis masks == 1
    mask_3d = np.logical_and.outer(
        np.logical_and.outer(x_mask, y_mask), z_mask
    ).astype(int)

    # cell_type is now 1 for tissue, 0 for exterior (no inversion needed)
    cell_type = mask_3d.ravel(order="F")

    # Create the rectilinear grid and assign cell type as a field
    grid = pv.RectilinearGrid(x_coords, y_coords, z_coords)
    n_points = grid.number_of_points

    if field_data is None:
        field_data = {}
    field_data.setdefault("restitution_model", cell_type)

    field_defaults = field_defaults or {}
    if field_data:
        for field_name, data in field_data.items():
            if data.shape[0] != n_points:
                # Fill with default if shape mismatch
                if field_name in field_defaults:
                    fill_value = field_defaults[field_name]
                elif data.ndim == 1:
                    fill_value = data[0]
                else:
                    fill_value = np.full(data.shape[1:], fallback_value_vector)
                values = np.full((n_points,) + data.shape[1:], fill_value)
            else:
                values = data
            grid[field_name] = values

    return grid

def get_argument_parser():
    """
    Configures and returns the argument parser for the script.
    """
    parser = argparse.ArgumentParser(description="Generate a rectilinear slab and save as VTK.")
    parser.add_argument("output_file", help="Output VTK file path.")
    parser.add_argument("--ndivs", type=int, nargs=3, required=True, help="Number of divisions per axis (nx,ny,nz).")
    parser.add_argument("--spacing", type=float, nargs=3, default=[0.05, 0.05, 0.05], help="Spacing (dx, dy, dz).")
    parser.add_argument("--fallback_scalar", type=float, default=np.nan, help="Fallback value for scalar fields. To be used if no specific default is provided.")
    parser.add_argument("--fallback_vector", type=float, default=0.0, help="Fallback value for vector fields. To be used if no specific default is provided.")
    parser.add_argument("--defaults", type=str, default="{}", help="A string with JSON dict of specific default values for fields.")
    parser.add_argument("--field_data", type=str, default="{}", help="JSON dict of field_name: list/array.")
    parser.add_argument("--add_no_layer", action="store_true", help="Do not add an extra layer beyond each face. In general, this layer is necessary for simulations.")
    parser.add_argument("--add_square", action="store_true", help="If set, add a square region with restitution_model=2 in the center.")
    parser.add_argument("--square_size", type=int, default=3, help="Size of the square region to set restitution_model=2 (used if --add_square).")

    # Arguments for stim sites
    parser.add_argument("--generate_stim_sites", action="store_true", help="If set, generate stim sites and label them in the stimulation_sites data field.")
    parser.add_argument("--stim_region_type", type=str, default="row", choices=["row", "column", "point"], help="Region type for stim sites.")
    parser.add_argument("--stim_index", type=str, default="0", help="Index for stim region (int for row/column index, tuple for point).")
    parser.add_argument("--stim_z_mode", type=str, default="zero", choices=["zero", "center", "all"], help="Z mode for stim region (zero, center, all, or int).")
    parser.add_argument("--stim_front_mode", type=str, default="all", choices=["zero", "center", "all"], help="X/Y mode for stim region (zero, center, all, or int).")
    return parser

def main():
    """
    Entry point for the script. Configures argument parsing and calls the slab generation function.
    """
    parser = get_argument_parser()
    args = parser.parse_args()

    try:
        field_defaults = json.loads(args.defaults)
    except json.JSONDecodeError:
        print("Error parsing the dictionary of default values. Ensure it is in valid JSON format.")
        return

    try:
        field_data_dict = json.loads(args.field_data)
        # Convert lists to numpy arrays
        field_data = {k: np.array(v) for k, v in field_data_dict.items()}
    except Exception:
        print("Error parsing field_data. Ensure it is a valid JSON dictionary of arrays.")
        return

    if 'fibers_orientation' not in field_data:
        field_data['fibers_orientation'] = np.array([[0,0,0]])

    grid = generate_rectilinear_slab(
        tuple(args.ndivs), tuple(args.spacing), field_data,
        args.fallback_scalar, args.fallback_vector,
        field_defaults, not args.add_no_layer
    )
    if args.add_square:
        set_center_square_restitution_model(grid, args.square_size)

    # Generate and save stim sites if requested
    if args.generate_stim_sites:
        stim_sites = set_stimulation_sites(
            grid,
            region_type=args.stim_region_type,
            index=args.stim_index,
            z_mode=args.stim_z_mode,
            front_mode=args.stim_front_mode,
            add_layer=not args.add_no_layer
        )
        # Build stimulation sites filename based on output_file
        output_root, _ = os.path.splitext(args.output_file)
        stimulation_sites_file = output_root + "_stimulation_sites.json"
        with open(stimulation_sites_file, "w") as f:
            json.dump(stim_sites, f, indent=4)

    grid.save(args.output_file)


if __name__ == "__main__":
    main()
