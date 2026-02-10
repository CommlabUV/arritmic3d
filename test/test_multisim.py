import arritmic3d
import numpy as np
import pyvista as pv
import os

# Conversion of int to CellType
def convert_to_cell_type(cell_type):
    if cell_type == 0:
        return arritmic3d.CellType.HEALTHY
    elif cell_type == 1:
        return arritmic3d.CellType.BORDER_ZONE
    else:
        return arritmic3d.CellType.CORE

# Conversion to tissue region
def convert_to_tissue_region(region):
    if region == 0:
        return arritmic3d.TissueRegion.ENDO
    elif region == 1:
        return arritmic3d.TissueRegion.MID
    else:
        return arritmic3d.TissueRegion.EPI

def main():
    vtk_file = "casos/ventricle_Tagged_2.vtk"
    print(f"Reading file: {vtk_file}", flush=True)
    grid = pv.read(vtk_file)
    print(grid, flush=True)
    dims = grid.dimensions
    x_coords = np.unique(grid.points[:, 0])
    y_coords = np.unique(grid.points[:, 1])
    z_coords = np.unique(grid.points[:, 2])
    # We assume the spacing is uniform
    x_spacing = x_coords[1] - x_coords[0]
    y_spacing = y_coords[1] - y_coords[0]
    z_spacing = z_coords[1] - z_coords[0]
    print("Dimensions:", dims)
    print("Spacing:", x_spacing, y_spacing, z_spacing)

    print("Campos disponibles en point_data:", grid.point_data.keys())
    v_type = list(map(convert_to_cell_type, np.array(grid.point_data['Cell_type'])))
    v_region = list(map(convert_to_tissue_region, np.array(grid.point_data['EndoToEpi'])))

    # Number of cells in each dimension
    ncells_x = dims[0]
    ncells_y = dims[1]
    ncells_z = dims[2]
    tissue = arritmic3d.CardiacTissue(ncells_x, ncells_y, ncells_z, x_spacing, y_spacing, z_spacing)

    initial_apd = 300.0
    v_apd = [initial_apd] * (ncells_x * ncells_y * ncells_z)
    parameters = {"INITIAL_APD" : v_apd}
    fiber_or = np.array(grid.point_data['fibers_OR'])

    tissue.InitPy(v_type, v_region, parameters, fiber_or)
    print("tissue initialized", flush=True)

    p_sites = grid.point_data['34_pacing']
    p_sites_values = np.argwhere(p_sites == 1).flatten()
    print(len(p_sites), len(p_sites_values))
    print(p_sites_values)

    # Remove innecessary data
    for key in grid.point_data.keys():
        if key != 'Cell_type':
            grid.point_data.remove(key)

    # Set the timer for saving the VTK files and succesive activations
    tissue.SetTimer(20)  # 20 ms
    n_tick = 0

    # First activation
    for i_pacing, initial_node in enumerate(p_sites_values):
        #initial_node = 12051 #tissue.GetIndex(2, 2, 1)
        tissue = arritmic3d.CardiacTissue(ncells_x, ncells_y, ncells_z, x_spacing, y_spacing, z_spacing)
        tissue.InitPy(v_type, v_region, parameters, fiber_or)

        tissue.SetTimer(20)  # 20 ms
        n_tick = 0
        beat = 0

        tissue.ExternalActivation([initial_node], 0.0, beat)
        print(0)

        pacing_dir = f"pacing_site_{i_pacing}"
        output_dir = f"output/{pacing_dir}"
        print("Initial node: ", initial_node)
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
            print("Current Output :: ", output_dir)

        i = 1
        while tissue.GetTime() < 2000.0:
            tick = tissue.update(0)
            if tick:
                n_tick += 1
                if n_tick % 40 == 0:
                    beat += 1
                    tissue.ExternalActivation([initial_node], tissue.GetTime(), beat)
                    print("Beat at time:", tissue.GetTime())

                # Update the cell states
                grid.point_data['State'] = tissue.GetStates()


                grid.save(f"{output_dir}/vent{int(tissue.GetTime()):04d}.vtk")

            #if i % 1000 == 0:
            #    print(i, tissue.GetTime())

            i += 1


if __name__ == "__main__":
    main()
