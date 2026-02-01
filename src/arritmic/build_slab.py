import pyvista as pv
import numpy as np
import argparse
import json
import os
from .build_slab_regions import set_region, load_regions_from_file, parse_regions_from_cli, validate_region, apply_region


def generate_rectilinear_slab(nnodes, spacing=(1.0, 1.0, 1.0), field_data={}):
    """
    Generates a rectilinear grid (slab) with specified number of divisions and assigns optional point data.

    Parameters:
        ndivs (tuple): (nx_divs, ny_divs, nz_divs) number of divisions along each axis.
        spacing (tuple): (dx, dy, dz) spacing between grid points.
        field_data (dict): Optional. Dictionary of field_name: value to assign as point data.
                          Values can be scalar (applied uniformly) or array of interior points (ndivs[0]*ndivs[1]*ndivs[2]).

    Returns:
        pv.RectilinearGrid: The generated rectilinear grid.
    """
    def make_coords(n_nodes, step):
        """Generates coordinates for one axis, adding an extra layer of thickness 1."""
        # nnodes + 2 nodes: padding -1 .. nnodes-1 (exclusive)
        coords = np.arange(-1, n_nodes + 1) * step
        return coords

    def make_mask(n_nodes):
        """Create an axis mask where 1 indicates tissue (interior) and 0 indicates exterior layer."""
        # interior nodes are ones, exterior padding (first and last) are zeros
        mask = np.concatenate(([0], np.ones(n_nodes, dtype=int), [0]))
        return mask

    x_coords = make_coords(nnodes[0], spacing[0])
    y_coords = make_coords(nnodes[1], spacing[1])
    z_coords = make_coords(nnodes[2], spacing[2])

    x_mask = make_mask(nnodes[0])
    y_mask = make_mask(nnodes[1])
    z_mask = make_mask(nnodes[2])

    # Combine masks so that a cell is tissue only if it's interior on all three axes
    # use logical_and to get True only where all axis masks == 1
    mask_3d = np.logical_and.outer(
        np.logical_and.outer(x_mask, y_mask), z_mask
    ).astype(int)

    # Create the rectilinear grid
    grid = pv.RectilinearGrid(x_coords, y_coords, z_coords)
    n_points = grid.number_of_points
    n_interior = nnodes[0] * nnodes[1] * nnodes[2]
    interior_mask = mask_3d.ravel(order="F") != 0

    # Default fields
    default_field_data = {
        "restitution_model": 1,
        "fibers_orientation": [0, 0, 0]
    }

    # Merge with user-provided field_data (user values override defaults)
    merged_field_data = {**default_field_data, **field_data}

    # Process and assign fields
    for field_name, val in merged_field_data.items():
        if isinstance(val, (list, tuple)):
            val = np.array(val)

        if isinstance(val, np.ndarray):
            # Could be a vector (components) or an array of values
            if val.ndim == 1:
                # 1D array: could be components (len <= 3) or values for interior points
                if len(val) <= 3:
                    # Treat as vector: zeros for exterior, val for interior
                    values = np.zeros((n_points, len(val)))
                    values[interior_mask] = val
                elif len(val) == n_interior:
                    # Treat as values for interior points: expand with padding (0 for exterior)
                    full_val = np.zeros(n_points)
                    full_val[interior_mask] = val
                    values = full_val
                else:
                    raise ValueError(f"Field '{field_name}' has {len(val)} values, expected {n_interior} (interior points) or <= 3 (vector components).")
            elif val.ndim == 2:
                # 2D array: vector field (n_points, components) or (n_interior, components)
                if val.shape[0] == n_points:
                    values = val
                elif val.shape[0] == n_interior:
                    # Expand with zero vectors for exterior
                    values = np.zeros((n_points, val.shape[1]))
                    values[interior_mask] = val
                else:
                    raise ValueError(f"Field '{field_name}' has {val.shape[0]} rows, expected {n_interior} (interior) or {n_points} (all points).")
            else:
                raise ValueError(f"Field '{field_name}' has invalid shape {val.shape}.")
        else:
            # Scalar: 0 for exterior, val for interior
            values = np.zeros(n_points, dtype=int if isinstance(val, int) else float)
            values[interior_mask] = val

        grid[field_name] = values

    return grid


def get_argument_parser():
    """
    Configures and returns the argument parser for the script.
    """
    parser = argparse.ArgumentParser(
        description="Generate a rectilinear slab (VTK RectilinearGrid) with point data fields and regions.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Basic slab with default values
  python build_slab.py cases/slab.vtk --nnodes 20 20 5 --spacing 0.05 0.05 0.05

  # Slab with custom field values and regions
  python build_slab.py cases/slab.vtk --nnodes 20 20 5 --spacing 0.05 0.05 0.05 \\
    --field restitution_model 2 --field fibers_orientation '[1,0,0]' \\
    --region '{\"shape\":\"circle\",\"cx\":0.5,\"cy\":0.5,\"r1\":0.2,\"r2\":0.4,\"restitution_model\":3}'

  # Using regions file and CLI regions
  python build_slab.py cases/slab.vtk --nnodes 40 40 5 --spacing 0.05 0.05 0.05 \\
    --regions-file ./regions_base.json \\
    --region '{\"shape\":\"square\",\"cx\":1.0,\"cy\":1.0,\"r1\":0.1,\"r2\":0.2,\"restitution_model\":5}'
        """
    )

    # Positional arguments
    parser.add_argument(
        "output_file",
        help="Output VTK file path."
    )

    # Grid generation group
    grid_group = parser.add_argument_group("Grid generation", "Define the slab grid dimensions and spacing.")
    grid_group.add_argument(
        "--nnodes",
        type=int,
        nargs=3,
        required=True,
        metavar=("NX", "NY", "NZ"),
        help="Number of nodes per axis. Each dimension will have NX, NY, NZ points respectively."
    )
    grid_group.add_argument(
        "--spacing",
        type=float,
        nargs=3,
        default=[0.05, 0.05, 0.05],
        metavar=("DX", "DY", "DZ"),
        help="Spacing (world units) between grid points along each axis (default: 0.05 0.05 0.05)."
    )

    # Point data fields group
    field_group = parser.add_argument_group("Point data fields", "Set default values for grid point data fields.")
    field_group.add_argument(
        "--field",
        action="append",
        nargs=2,
        metavar=("FIELD_NAME", "VALUE"),
        help="Add or override a point data field. VALUE can be a scalar or JSON array. "
             "Example: --field restitution_model 1 or --field fibers_orientation '[1,0,0]'. "
             "Can be used multiple times."
    )

    # Region modification group
    region_group = parser.add_argument_group("Geometric regions", "Define regions to modify field values.")
    region_group.add_argument(
        "--regions-file",
        type=str,
        metavar="PATH",
        help="Path to JSON file containing a list of region objects. Regions are applied in order. "
             "Each region must specify 'shape' and required parameters (cx, cy, r1, r2 for circle/square/diamond; "
             "side for edge; ids for node_ids). Field values can be scalar, vector, or gradient lists."
    )
    region_group.add_argument(
        "--region",
        action="append",
        type=str,
        metavar="JSON",
        help="Add a region as a JSON object string. Supported shapes: circle, square, diamond, edge, node_ids. "
             "Example: '{\"shape\":\"circle\",\"cx\":0.5,\"cy\":0.5,\"r1\":0.2,\"r2\":0.4,\"restitution_model\":3}'. "
             "Can be used multiple times. CLI regions override file regions on overlap."
    )

    # Tissue activation group
    activation_group = parser.add_argument_group("Tissue activation definition", "Define activation regions on the slab.")
    activation_group.add_argument(
        "--region-by-side",
        action="append",
        nargs=2,
        metavar=("SIDE", "REGION_ID"),
        help="Define an entire slab side as an activation region. SIDE must be one of: north, south, east, west. "
             "REGION_ID is the integer value to assign to activation_region field. "
             "Can be used multiple times to define multiple sides as activation regions."
    )
    activation_group.add_argument(
        "--region-by-node-ids",
        action="append",
        nargs='+',
        metavar=("NODE_ID", "REGION_ID"),
        help="Define an activation region on specific nodes by their IDs. All arguments except the last are node IDs; "
             "the last argument is REGION_ID (integer). "
             "Example: --region-by-node-ids 100 200 300 1 defines nodes 100, 200, 300 as a region with activation_region=1. "
             "Can be used multiple times to create multiple activation groups."
    )

    return parser


def build_slab(args,save=True):
    """
    Build the slab grid based on parsed arguments and optionally save to file.

    Parameters:
        args: Parsed arguments from argparse (see documentation).
        save (bool): If True, saves the generated slab to the specified output file.

    Returns:
        pv.RectilinearGrid: The generated rectilinear grid.
    """

    # Parse field data from CLI
    field_data = {}
    if getattr(args, "field", None):
        for field_name, value_str in args.field:
            try:
                # Try to parse as JSON (for arrays)
                value = json.loads(value_str)
            except (json.JSONDecodeError, ValueError):
                # Try to parse as scalar
                try:
                    value = float(value_str)
                except ValueError:
                    value = value_str
            field_data[field_name] = value

    grid = generate_rectilinear_slab(
        tuple(args.nnodes),
        tuple(args.spacing),
        field_data=field_data
    )

    # Regions handling (strict validation, precedence: regions-file first, then --region entries).
    # Later regions overwrite earlier ones on overlap. No defaults or interactivity: invalid inputs raise.
    regions = []
    if getattr(args, "regions_file", None):
        regions += load_regions_from_file(args.regions_file)
    if getattr(args, "region", None):
        regions += parse_regions_from_cli(args.region)

    # Convert --region-by-side entries to region dicts
    if getattr(args, "region_by_side", None):
        for side, region_id_str in args.region_by_side:
            region_id = int(region_id_str)
            regions.append({
                "shape": "side",
                "side": side,
                "activation_region": region_id
            })

    # Convert --region-by-node-ids entries to region dicts
    if getattr(args, "region_by_node_ids", None):
        for node_list in args.region_by_node_ids:
            region_id = int(node_list[-1])
            node_ids = [int(nid) for nid in node_list[:-1]]
            regions.append({
                "shape": "node_ids",
                "ids": node_ids,
                "activation_region": region_id
            })

    # Apply all regions (including activations)
    for idx, reg in enumerate(regions, start=1):
        validate_region(reg, idx)
        apply_region(grid, reg)

    if save:
        print(f"Saving slab to {args.output_file}...")
        grid.save(args.output_file)
        print("Slab saved.")

    return grid


def main():
    """
    Entry point for the script. Configures argument parsing and calls the slab generation function.
    """
    parser = get_argument_parser()
    args = parser.parse_args()


    grid = build_slab(args, save=True)


if __name__ == "__main__":
    main()
