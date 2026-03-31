import arritmic3d
import numpy as np
import pyvista as pv

# Conversion of CellType and TissueRegion from the VTK file to Ten Tusscher model id.
# Cell types: 0 = Healthy, 1 = Border Zone
# Tissue regions: 0 = Endo, 1 = Mid, 2 = Epi
def convert_to_cell_type(cell_type, region):
    if cell_type < 0 or cell_type > 1:
        type = 7  # Core
    else:
        type = 1 + 3 * int(cell_type) + int(region)  # Combine cell type and region to get a unique identifier
    return type

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
    v_type = list(map(convert_to_cell_type, np.array(grid.point_data['Cell_type']), np.array(grid.point_data['EndoToEpi'])))

    # Number of cells in each dimension
    ncells_x = dims[0]
    ncells_y = dims[1]
    ncells_z = dims[2]
    tissue = arritmic3d.CardiacTissue(ncells_x, ncells_y, ncells_z, x_spacing, y_spacing, z_spacing)

    sensor_point = 60382
    v_sensor = [0] * tissue.size()
    v_sensor[sensor_point] = 1  # Set the sensor point
    parameters = {"SENSOR" : v_sensor}
    #v_region = [arritmic3d.TissueRegion.ENDO] * (ncells_x * ncells_y * ncells_z)
    fiber_or = np.array(grid.point_data['fibers_OR'])
    print("Fibers OR:", fiber_or.shape)

    initial_apd = 300.0
    tissue.InitModels("restitutionModels/config_TenTuscher_APD.csv","restitutionModels/config_TenTuscher_CV.csv")
    tissue.SetInitialAPD(initial_apd)
    tissue.InitPy(v_type, parameters, fiber_or)
    print("tissue initialized", flush=True)

    # Remove innecessary data
    for key in grid.point_data.keys():
        if key != 'Cell_type':
            grid.point_data.remove(key)

    # Set the timer for saving the VTK files
    tissue.SetTimer(arritmic3d.SystemEventType.FILE_WRITE, 20)  # 20 ms

    # First activation
    initial_node = 12051 #tissue.GetIndex(2, 2, 1)
    beat = 0
    tissue.SetSystemEvent(arritmic3d.SystemEventType.EXT_ACTIVATION, 0)  # 0 ms for the first activation
    tissue.SetSystemEvent(arritmic3d.SystemEventType.EXT_ACTIVATION, 700)  # 700 ms for the second activation
    print(0)

    i = 1
    while tissue.GetTime() < 1000.0:
        tick = tissue.update(0)

        if tick == arritmic3d.SystemEventType.EXT_ACTIVATION:
            beat += 1
            print("Mean APD variation: ", tissue.GetAPDMeanVariation())
            tissue.ResetVariations()

            tissue.ExternalActivation([initial_node], tissue.GetTime(), beat)
            print("Beat at time:", tissue.GetTime())

        elif tick == arritmic3d.SystemEventType.FILE_WRITE:
            # Update the cell states
            grid.point_data['State'] = tissue.GetStates()
            grid.point_data['APD'] = tissue.GetAPD()
            grid.point_data['CV'] = tissue.GetCV()

            grid.save(f"output/vent{int(tissue.GetTime())}.vtk")

        if i % 1000 == 0:
            print(i, tissue.GetTime())

        i += 1

    print("Mean APD variation: ", tissue.GetAPDMeanVariation())

    sensor_names = tissue.GetSensorDataNames()
    sensor_data = tissue.GetSensorInfo()
    print("Sensor names:", sensor_names)
    print("Sensor data:", sensor_data)

if __name__ == "__main__":
    main()
