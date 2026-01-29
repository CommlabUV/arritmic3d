import arritmic
import numpy as np
import pyvista as pv

# Conversion of int to CellType
def convert_to_cell_type(cell_type):
    if cell_type == 0:
        return arritmic.CellType.HEALTHY
    elif cell_type == 1:
        return arritmic.CellType.BORDER_ZONE
    else:
        return arritmic.CellType.CORE

# Conversion to tissue region
def convert_to_tissue_region(region):
    if region == 0:
        return arritmic.TissueRegion.ENDO
    elif region == 1:
        return arritmic.TissueRegion.MID
    else:
        return arritmic.TissueRegion.EPI

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
    tissue = arritmic.CardiacTissue(ncells_x, ncells_y, ncells_z, x_spacing, y_spacing, z_spacing)

    sensor_point = 60382
    initial_apd = 300.0
    v_apd = [initial_apd] * tissue.size()
    v_sensor = [0] * tissue.size()
    v_sensor[sensor_point] = 1  # Set the sensor point
    parameters = {"INITIAL_APD" : v_apd, "SENSOR" : v_sensor}
    #v_region = [arritmic.TissueRegion.ENDO] * (ncells_x * ncells_y * ncells_z)
    fiber_or = np.array(grid.point_data['fibers_OR'])
    print("Fibers OR:", fiber_or.shape)

    tissue.InitPy(v_type, v_region, parameters, fiber_or)
    print("tissue initialized", flush=True)

    # Remove innecessary data
    for key in grid.point_data.keys():
        if key != 'Cell_type':
            grid.point_data.remove(key)

    # Set the timer for saving the VTK files
    tissue.SetTimer(arritmic.SystemEventType.FILE_WRITE, 20)  # 20 ms

    # First activation
    initial_node = 12051 #tissue.GetIndex(2, 2, 1)
    beat = 0
    tissue.SetSystemEvent(arritmic.SystemEventType.EXT_ACTIVATION, 100)  # 100 ms for the first activation
    tissue.SetSystemEvent(arritmic.SystemEventType.EXT_ACTIVATION, 800)  # 800 ms for the second activation
    print(0)

    i = 1
    while tissue.GetTime() < 1000.0:
        tick = tissue.update(0)

        if tick == arritmic.SystemEventType.EXT_ACTIVATION:
            beat += 1
            print("APD error: ", tissue.GetAPDMeanError())
            tissue.ResetErrors()

            tissue.ExternalActivation([initial_node], tissue.GetTime(), beat)
            print("Beat at time:", tissue.GetTime())

        elif tick == arritmic.SystemEventType.FILE_WRITE:
            # Update the cell states
            grid.point_data['State'] = tissue.GetStates()
            grid.point_data['APD'] = tissue.GetAPD()
            grid.point_data['CV'] = tissue.GetCV()

            #grid.save(f"output/vent{int(tissue.GetTime())}.vtk")

        if i % 1000 == 0:
            print(i, tissue.GetTime())

        i += 1

    print("APD error: ", tissue.GetAPDMeanError())

    sensor_names = tissue.GetSensorDataNames()
    sensor_data = tissue.GetSensorInfo()
    print("Sensor names:", sensor_names)
    print("Sensor data:", sensor_data)

if __name__ == "__main__":
    main()
