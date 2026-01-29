import arritmic
import numpy as np
import pyvista as pv

def main():
    print("0", flush=True)
    vtk_file = "output/test0.vtk"
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
    v_type = list(map(arritmic.CellType, np.array(grid.point_data['Type'])))

    # Number of cells in each dimension
    ncells_x = dims[0]
    ncells_y = dims[1]
    ncells_z = dims[2]
    tissue = arritmic.CardiacTissue(ncells_x, ncells_y, ncells_z, x_spacing, y_spacing, z_spacing)
    #v_type = [arritmic.CellType.HEALTHY] * (ncells_x * ncells_y * ncells_z)

    initial_apd = 100.0
    #v_apd = np.full((6 * 6 * 6), initial_apd)
    v_apd = [initial_apd] * (ncells_x * ncells_y * ncells_z)
    parameters = {"INITIAL_APD" : v_apd}
    v_region = [arritmic.TissueRegion.ENDO] * (ncells_x * ncells_y * ncells_z)

    tissue.InitPy(v_type, v_region, parameters)
    print("tissue initialized", flush=True)

    initial_node = tissue.GetIndex(2, 2, 1)
    tissue.ExternalActivation([initial_node], 0.0)
    tissue.SaveVTK("output/test0.vtk")
    print(0)
    # Aplicamos threshold
    #subgrid = grid.threshold(value=(0, 1), scalars='Type')
    # Set opacity per cell
    v_opacity = list(map(lambda x: 0.1 if x == 2 else 0.9, np.array(grid.point_data['Type'])))
    #print("Opacity:", v_opacity)
    plotter = pv.Plotter()
    #plotter.add_mesh(grid, scalars='State', show_edges = True, clim=(0,2), cmap='coolwarm', show_scalar_bar=True, opacity=v_opacity)
    plotter.add_mesh(grid, scalars='State', show_edges = True, clim=(0,2), cmap='coolwarm', show_scalar_bar=True, opacity = 0.8)
    plotter.show(auto_close=False)

    for i in range(1, 141):
        if i == 120:
            tissue.ExternalActivation([initial_node], tissue.GetTime())
        tissue.update(1)
        print(i, tissue.GetTime())
        if i % 4 == 0:
            # Update the cell states
            grid.point_data['State'] = tissue.GetStates()
            #mesh_actor.mesh.cell_data['State'] = grid.cell_data['State']
            plotter.update_scalars(grid.point_data['State'], render=True)
            #plotter.show(auto_close=False)
            #tissue.SaveVTK(f"output/test{i}.vtk")
            grid.save(f"output/test{i}.vtk")

if __name__ == "__main__":
    main()
