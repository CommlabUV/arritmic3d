import pyvista as pv
import numpy as np
import argparse
import json

def convert_to_rectilinear(input_filename, output_filename,
                           default_value_scalar=np.nan,
                           default_value_vector=0.0,
                           field_defaults=None,
                           add_layer=True):
    """
    Converts a VTK file of type UNSTRUCTURED_GRID to RECTILINEAR_GRID, preserving all point data fields.

    Parameters:
        input_filename (str): Path to the input VTK file (UNSTRUCTURED_GRID).
        output_filename (str): Path to the output VTK file (RECTILINEAR_GRID).
        default_value_scalar (float): Default value for missing scalar data points.
        default_value_vector (float): Default value for missing vector data points.
        field_defaults (dict): A dictionary with specific default values for individual fields.
    """
    # Load the UNSTRUCTURED_GRID file
    mesh = pv.read(input_filename)

    # Extract unique coordinates of the points
    points = mesh.points
    x_coords = np.unique(points[:, 0])  # Unique X coordinates
    y_coords = np.unique(points[:, 1])  # Unique Y coordinates
    z_coords = np.unique(points[:, 2])  # Unique Z coordinates

    # Add an extra layer at each edge of each axis
    def extend_coords(coords):
        """
        Extend the coordinates by adding an extra layer at each edge of the axis.
        """
        if not add_layer:
            return coords
        if len(coords) < 2:
            raise ValueError("An axis cannot be extended with less than 2 values.")
        step_start = coords[1] - coords[0]
        step_end = coords[-1] - coords[-2]
        extended = np.concatenate((
            [coords[0] - step_start],
            coords,
            [coords[-1] + step_end]
        ))
        return extended

    x_coords = extend_coords(x_coords)
    y_coords = extend_coords(y_coords)
    z_coords = extend_coords(z_coords)

    # Create a complete rectilinear grid
    rectilinear_grid = pv.RectilinearGrid(x_coords, y_coords, z_coords)

    # Get the grid points in PyVista's internal order
    grid_points = rectilinear_grid.points

    # Transfer all point data fields to the new grid
    field_defaults = field_defaults or {}
    for field_name in mesh.point_data:
        print(f"Processing field: {field_name}")
        # Retrieve the current field's data
        point_data = mesh.point_data[field_name]
        data_shape = point_data.shape[1:] if point_data.ndim > 1 else ()

        # Determine the specific or general default value
        if field_name in field_defaults:
            field_default = field_defaults[field_name]
        elif len(data_shape) == 0:  # Scalar
            field_default = default_value_scalar
        else:  # Vector or tensor
            field_default = np.full(data_shape, default_value_vector)

        # Create a dictionary for quick access to point values
        point_dict = {tuple(p): v for p, v in zip(points, point_data)}

        # Assign values to the grid points in the correct order
        values = np.array([point_dict.get(tuple(p), field_default) for p in grid_points])

        # Assign the values to the new grid
        rectilinear_grid[field_name] = values

    # Save the new RECTILINEAR_GRID
    rectilinear_grid.save(output_filename)
    print(f"Converted file saved to: {output_filename}")

def main():
    """
    Entry point for the script. Configures argument parsing and calls the conversion function.
    """
    # Configure argument parsing
    parser = argparse.ArgumentParser(description="Converts a UNSTRUCTURED_GRID VTK file to RECTILINEAR_GRID.")
    parser.add_argument("input_file", help="Path to the input VTK file (UNSTRUCTURED_GRID).")
    parser.add_argument("output_file", help="Path to the output VTK file (RECTILINEAR_GRID).")
    parser.add_argument("--default_scalar", type=float, default=np.nan,
                        help="Default value for scalar fields (missing points).")
    parser.add_argument("--default_vector", type=float, default=0.0,
                        help="Default value for vector fields (missing points).")
    parser.add_argument("--defaults", type=str, default="{}",
                        help="JSON-formatted dictionary of specific default values for fields.")
    parser.add_argument("--add_no_layer", action="store_true",
                        help="Doest not add an extra layer at each edge of each axis.")
    args = parser.parse_args()

    # Convert the dictionary of defaults
    try:
        field_defaults = json.loads(args.defaults)
    except json.JSONDecodeError:
        print("Error parsing the dictionary of default values. Ensure it is in valid JSON format.")
        return

    # Call the main conversion function
    convert_to_rectilinear(args.input_file, args.output_file,
                           default_value_scalar=args.default_scalar,
                           default_value_vector=args.default_vector,
                           field_defaults=field_defaults,
                           add_layer = not args.add_no_layer)

if __name__ == "__main__":
    main()
