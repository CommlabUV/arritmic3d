import pyvista as pv
import numpy as np
import argparse
import json
import os

# Generate list of node ids to activate the tissue
def get_activation_node_ids(grid, region_type="row", index=0, z_mode="zero", nx=None, ny=None, nz=None, add_layer=True):
    """
    Returns a list of node ids for the tissue region, using block size and add_layer info.

    Parameters:
        grid (pv.RectilinearGrid): The grid to search.
        region_type (str): Region type ("row", "column", "point").
        index (int, tuple, or str): Row/column index or point coordinates (can be string for parsing).
        z_mode (str or int): "zero" for Z=0, "center" for central Z, "all" for all Z, or an integer for specific Z.
        nx, ny, nz (int): Number of divisions in X, Y, Z (without extra layer).
        add_layer (bool): Whether the grid has an extra layer.

    Returns:
        list: List of node ids (only tissue, never -1).
    """
    dims = grid.dimensions  # (nx+1+2*offset, ny+1+2*offset, nz+1+2*offset)
    offset = 1 if add_layer else 0

    # Infer nx, ny, nz if not provided
    if nx is None or ny is None or nz is None:
        nx = dims[0] - 1 - 2*offset
        ny = dims[1] - 1 - 2*offset
        nz = dims[2] - 1 - 2*offset

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
        z_indices = list(range(nz + 1))
    else:
        raise ValueError("Invalid z_mode value")

    ids = []
    if region_type == "row":
        y = index
        for z in z_indices:
            for x in range(nx + 1):
                idx = np.ravel_multi_index((x + offset, y + offset, z + offset), dims, order="F")
                ids.append(idx)
    elif region_type == "column":
        x = index
        for z in z_indices:
            for y in range(ny + 1):
                idx = np.ravel_multi_index((x + offset, y + offset, z + offset), dims, order="F")
                ids.append(idx)
    elif region_type == "point":
        idx = np.ravel_multi_index(
            (index[0] + offset, index[1] + offset, index[2] + offset), dims, order="F"
        )
        ids.append(idx)
    return ids


def set_activation_sites(
    grid,
    region_type,
    index,
    z_mode,
    nx,
    ny,
    nz,
    add_layer
):
    """
    Sets the 'pacing' field in the grid and returns the activation sites dictionary.

    Parameters:
        grid (pv.RectilinearGrid): The grid to modify.
        region_type (str): Region type ("row", "column", "point").
        index (int, tuple, or str): Row/column index or point coordinates.
        z_mode (str or int): Z mode for stim region.
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
        nx=nx,
        ny=ny,
        nz=nz,
        add_layer=add_layer
    )
    print("Node ids to activate:", activation_node_ids)
    activation_node_ids = [int(i) for i in activation_node_ids]
    pacing = np.zeros(grid.number_of_points, dtype=int)
    for idx in activation_node_ids:
        pacing[idx] = 1
    grid.point_data["pacing"] = pacing
    return {"INITIAL_NODE_ID": activation_node_ids}


def set_center_square_cell_type(grid, square_size=3):
    """
    Sets 'Cell_type' = 1 for a square region in the center of the XY plane at all Z layers,
    but only for tissue nodes (Cell_type != -1).
    """
    x_len = grid.dimensions[0]
    y_len = grid.dimensions[1]
    z_len = grid.dimensions[2]

    x_center = x_len // 2
    y_center = y_len // 2
    half = square_size // 2

    if "Cell_type" in grid.point_data:
        cell_type = grid.point_data["Cell_type"].copy()
    else:
        cell_type = np.zeros(grid.number_of_points, dtype=int)

    for k in range(z_len):
        for j in range(y_center - half, y_center + half + (square_size % 2)):
            for i in range(x_center - half, x_center + half + (square_size % 2)):
                if 0 <= i < x_len and 0 <= j < y_len:
                    idx = np.ravel_multi_index((i, j, k), (x_len, y_len, z_len), order="F")
                    if cell_type[idx] != -1:
                        cell_type[idx] = 1

    grid.point_data["Cell_type"] = cell_type

def generate_rectilinear_slab(x_divs, y_divs, z_divs, spacing=(1.0, 1.0, 1.0),
                              field_data=None, default_value_scalar=np.nan,
                              default_value_vector=0.0, field_defaults=None, add_layer=True):
    """
    Generates a rectilinear grid (slab) with specified number of divisions and assigns optional point data.

    Parameters:
        x_divs (int): Number of divisions along X axis.
        y_divs (int): Number of divisions along Y axis.
        z_divs (int): Number of divisions along Z axis.
        spacing (tuple): (dx, dy, dz) spacing between grid points.
        field_data (dict): Optional. Dictionary of field_name: ndarray to assign as point data.
        default_value_scalar (float): Default value for missing scalar data.
        default_value_vector (float): Default value for missing vector data.
        field_defaults (dict): Optional. Specific default values for fields.
        add_layer (bool): If True, adds an extra layer at each edge of each axis.

    Returns:
        pv.RectilinearGrid: The generated rectilinear grid.
    """
    def make_coords(n_divs, step):
        coords = np.arange(n_divs + 1) * step
        if add_layer and len(coords) > 1:
            coords = np.concatenate(([coords[0] - step], coords, [coords[-1] + step]))
        return coords

    def make_mask(n_divs):
        mask = np.zeros(n_divs + 1, dtype=int)
        if add_layer and len(mask) > 1:
            mask = np.concatenate(([1], mask, [1]))
        return mask

    x_coords = make_coords(x_divs, spacing[0])
    y_coords = make_coords(y_divs, spacing[1])
    z_coords = make_coords(z_divs, spacing[2])

    x_mask = make_mask(x_divs)
    y_mask = make_mask(y_divs)
    z_mask = make_mask(z_divs)

    mask_3d = np.logical_or.outer(
        np.logical_or.outer(x_mask, y_mask), z_mask
    ).astype(int)
    cell_type = mask_3d.ravel(order="F")
    cell_type = np.where(cell_type == 1, -1, 0)
    if field_data is None:
        field_data = {}
    field_data.setdefault("Cell_type", cell_type)
    # Create the rectilinear grid and assign cell type as a field
    grid = pv.RectilinearGrid(x_coords, y_coords, z_coords)
    n_points = grid.number_of_points

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
                    fill_value = np.full(data.shape[1:], default_value_vector)
                values = np.full((n_points,) + data.shape[1:], fill_value)
            else:
                values = data
            grid[field_name] = values

    return grid


def main():
    """
    Entry point for the script. Configures argument parsing and calls the slab generation function.
    """
    parser = argparse.ArgumentParser(description="Generate a rectilinear slab and save as VTK.")
    parser.add_argument("output_file", help="Output VTK file path.")
    parser.add_argument("--nx", type=int, required=True, help="Number of divisions for X axis.")
    parser.add_argument("--ny", type=int, required=True, help="Number of divisions for Y axis.")
    parser.add_argument("--nz", type=int, required=True, help="Number of divisions for Z axis.")
    parser.add_argument("--spacing", type=float, nargs=3, default=[1.0, 1.0, 1.0], help="Spacing (dx, dy, dz).")
    parser.add_argument("--default_scalar", type=float, default=np.nan, help="Default value for scalar fields.")
    parser.add_argument("--default_vector", type=float, default=0.0, help="Default value for vector fields.")
    parser.add_argument("--defaults", type=str, default="{}", help="JSON dict of specific default values for fields.")
    parser.add_argument("--field_data", type=str, default="{}", help="JSON dict of field_name: list/array.")
    parser.add_argument("--add_no_layer", action="store_true", help="Do not add an extra layer beyond each face.")
    parser.add_argument("--add_square", action="store_true", help="If set, add a square region with Cell_type=1 in the center.")
    parser.add_argument("--square_size", type=int, default=3, help="Size of the square region to set Cell_type=1 (used if --add_square).")
    # New arguments for stim sites
    parser.add_argument("--generate_stim_sites", action="store_true", help="If set, generate stim sites and save to arr3D_stim_sites.json.")
    parser.add_argument("--stim_region_type", type=str, default="row", choices=["row", "column", "point"], help="Region type for stim sites.")
    parser.add_argument("--stim_index", type=str, default="0", help="Index for stim region (int for row/column, tuple for point).")
    parser.add_argument("--stim_z_mode", type=str, default="zero", help="Z mode for stim region (zero, center, all, or int).")
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
    if 'EndoToEpi' not in field_data:
        field_data['EndoToEpi'] = np.array([1])
    if 'fibers_OR' not in field_data:
        field_data['fibers_OR'] = np.array([[0,0,0]])

    grid = generate_rectilinear_slab(
        args.nx, args.ny, args.nz,
        tuple(args.spacing), field_data,
        args.default_scalar, args.default_vector,
        field_defaults, not args.add_no_layer
    )
    if args.add_square:
        set_center_square_cell_type(grid, args.square_size)

    # Generate and save stim sites if requested
    if args.generate_stim_sites:
        stim_sites = set_activation_sites(
            grid,
            region_type=args.stim_region_type,
            index=args.stim_index,
            z_mode=args.stim_z_mode,
            nx=args.nx,
            ny=args.ny,
            nz=args.nz,
            add_layer=not args.add_no_layer
        )
        # Build pacing sites filename based on output_file
        output_root, _ = os.path.splitext(args.output_file)
        pacing_sites_file = output_root + "_pacing_sites.json"
        with open(pacing_sites_file, "w") as f:
            json.dump(stim_sites, f, indent=4)

    grid.save(args.output_file)


if __name__ == "__main__":
    main()
