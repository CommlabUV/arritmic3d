import pyvista as pv
import numpy as np
import argparse
import ast

Endo2Epi = {
    0 : "Endo",
    1 : "Mid",
    2 : "Epi"
}

CellType = {
    0 : "Healthy",
    1 : "BZ",
    2 : "Core"
}

"""
1,    HE_Endo
2,    HE_Mid
3,    HE_Epi
4,    BZ_Endo
5,    BZ_Mid
6,    BZ_Epi
7,    Core
"""
TenTusscherRestitutionModels = {
    "Healthy" : {
        "Endo" : 1,
        "Mid" : 2,
        "Epi" : 3
    },
    "BZ" : {
        "Endo" : 4,
        "Mid" : 5,
        "Epi" : 6
    },
    "Core" :  {
        "Endo" : 7,
        "Mid" : 7,
        "Epi" : 7
    },
}

def convert_to_rectilinear(input_filename, output_filename,
                           default_value_scalar=0.0,
                           default_value_vector=np.array([0.0, 0.0, 0.0]),
                           field_defaults=None,
                           activation=[],
                           add_layer=True):
    """
    Converts a VTK file of type UNSTRUCTURED_GRID to RECTILINEAR_GRID, preserving all point data fields.

    Parameters:
        input_filename (str): Path to the input VTK file (UNSTRUCTURED_GRID).
        output_filename (str): Path to the output VTK file (RECTILINEAR_GRID).
        default_value_scalar (float): Default value for missing scalar data points.
        default_value_vector (float): Default value for missing vector data points.
        field_defaults (dict): A dictionary with specific default values for individual fields.
        activation (list[dict]): A list of dictionaries with specific default values for individual fields.
        add_layer (bool): Whether to add an extra layer at each edge of each axis.
    """
    # Load the UNSTRUCTURED_GRID file
    mesh = pv.read(input_filename)

    # Extract unique coordinates of the points
    points = mesh.points
    x_coords = np.unique(points[:, 0])  # Unique X coordinates
    y_coords = np.unique(points[:, 1])  # Unique Y coordinates
    z_coords = np.unique(points[:, 2])  # Unique Z coordinates

    # Convert EndoToEpi + Cell_type labels to restitution_model
    if "EndoToEpi" in mesh.point_data and "Cell_type" in mesh.point_data:
        restitution_model = np.zeros(len(points), dtype=int)
        for i, p in enumerate(points):
            endo2epi = Endo2Epi[int(mesh.point_data["EndoToEpi"][i])]
            cell_type = CellType[int(mesh.point_data["Cell_type"][i])]
            restitution_model[i] = TenTusscherRestitutionModels[cell_type][endo2epi]
        mesh.point_data["restitution_model"] = restitution_model
    else:
        raise ValueError("The input file does not contain the fields 'EndoToEpi' and 'Cell_type'.")

    # Transfer fiber orientation, from 'fibers_OR' to 'fibers_orientation'
    if "fibers_OR" in mesh.point_data:
        mesh.point_data["fibers_orientation"] = mesh.point_data["fibers_OR"]
    else:
        # Set to [0,0,0] -> isotropic
        mesh.point_data["fibers_orientation"] = np.zeros((len(points), 3))

    # Add activation sites
    activation_region = np.zeros(len(points), dtype=int)
    # First, for each node with 34_pacing >0, set a different activation region
    # We get the indices of nodes that have 34_pacing > 0 and set them as different activation regions
    if "34_pacing" in mesh.point_data:
        pacing_sites = np.where(mesh.point_data["34_pacing"] > 0)[0]
        i = 1
        for site_index in pacing_sites:
            activation_region[site_index] = i
            i+=1

    # Then, process input
    for act in (activation or []):
        try:
            act_dict = ast.literal_eval(act)
        except (ValueError, SyntaxError):
            raise ValueError(f"Error parsing activation region: {act}")
        for region_id, nodes in act_dict.items():
            activation_region[nodes] = region_id

    mesh.point_data["activation_region"] = activation_region

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
    parser.add_argument("--add_no_layer", action="store_true",
                        help="Doest not add an extra layer at each edge of each axis.")
    parser.add_argument("--activation", action = "append",
                        help="define an activation region by node ids. The input must be a dictionary with key an integer (region id) and value a list of node ids (ints), that form that region. For example: --activation '{1 : [100, 101, 102]}'")

    args = parser.parse_args()

    # Call the main conversion function
    convert_to_rectilinear(args.input_file, args.output_file,
                           activation = args.activation,
                           add_layer = not args.add_no_layer)

if __name__ == "__main__":
    main()
