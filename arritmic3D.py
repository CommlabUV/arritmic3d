import os
import numpy as np
import pyvista as pv
import tissue_module
from arr3D_config import check_directory, get_parameters, load_config_file, convert_to_cell_type, convert_to_tissue_region
import sys


def load_grid(vtk_file):
    """
    Load the VTK file and return the grid.
    """
    print(f"Reading file: {vtk_file}", flush=True)
    grid = pv.read(vtk_file)

    # Remove innecessary data
    for key in grid.point_data.keys():
        if key not in ['Cell_type', 'EndoToEpi', 'fibers_OR']:
            grid.point_data.remove(key)

    return grid

def create_tissue(grid):
    """ Create a tissue object from the grid.
    The grid is expected to have the following point data:
    - Cell_type: the type of each cell (e.g., 'Endocardium', 'Epicardium', 'Myocardium')
    - EndoToEpi: a flag indicating whether the cell is endocardial or epicardial
    - fibers_OR: the orientation of the fibers in the cell
    """

    print("Creating tissue from grid", flush=True)
    """
    if 'Cell_type' not in grid.point_data:
        raise ValueError("The grid does not contain 'Cell_type' in point data. Please check the VTK file.")
    if 'EndoToEpi' not in grid.point_data:
        raise ValueError("The grid does not contain 'EndoToEpi' in point data. Please check the VTK file.")
    if 'fibers_OR' not in grid.point_data:
        raise ValueError("The grid does not contain 'fibers_OR' in point data. Please check the VTK file.")
    """
    dims = grid.dimensions
    x_coords = np.unique(grid.points[:, 0])
    y_coords = np.unique(grid.points[:, 1])
    z_coords = np.unique(grid.points[:, 2])

    # We assume the spacing is uniform
    x_spacing = x_coords[1] - x_coords[0]
    y_spacing = y_coords[1] - y_coords[0]
    z_spacing = z_coords[1] - z_coords[0]
    print("CA-Dimensions:", dims)
    print("CA-Spacing:", x_spacing, y_spacing, z_spacing)
    print("VTK-Scalars:", grid.point_data.keys())

    v_type = list(map(convert_to_cell_type, np.array(grid.point_data['Cell_type'])))
    v_region = list(map(convert_to_tissue_region, np.array(grid.point_data['EndoToEpi'])))

    # Number of cells in each dimension
    ncells_x = dims[0]
    ncells_y = dims[1]
    ncells_z = dims[2]

    tissue = tissue_module.CardiacTissue(ncells_x, ncells_y, ncells_z, x_spacing, y_spacing, z_spacing)

    vparams, params = get_parameters(tissue, dims, config_file)
    #print("Parameters:", params, flush=True)
    fiber_or = np.array(grid.point_data['fibers_OR'])
    tissue.InitPy(v_type, v_region, vparams, fiber_or)
    print("tissue initialized", flush=True)

    return tissue

def schedule_activation(cfg,tissue):

    activations = {}

    # Pacing protocol (S1,S2,...,SN) with (BCL1, BCL2, ..., BCLN)
    # Activation by pacing protocol
    """
    "PROTOCOL" : {
        "INITIAL_NODE_ID": [
            10713,
            10714,
            10715,
            10716,
            10717
        ],
        "FIRST_ACTIVATION_TIME" : 400,
        "N_STIMS_PACING": [6,3],
        "BCL": [600,400,300,200]
    },

    """
    if "PROTOCOL" in cfg:
        protocol = cfg['PROTOCOL']

        # Activation. The initial node can be an index (int) or a list of indexes.
        # If it is a list, we assume it is already in the correct format
        if isinstance(protocol['INITIAL_NODE_ID'], list):
            initial_nodes = protocol['INITIAL_NODE_ID']
        else:
            # If it is an int, we convert it to a list with one element.
            initial_nodes = [protocol['INITIAL_NODE_ID']]

        beat = 0
        activation_time = 0.0

        if 'N_STIMS_PACING' in protocol and 'BCL' in protocol:
            # Again, can be a list or a single value
            if isinstance(protocol['N_STIMS_PACING'], list):
                n_stims_sn = protocol['N_STIMS_PACING']
            else:
                n_stims_sn = [protocol['N_STIMS_PACING']]

            if isinstance(protocol['BCL'], list):
                bcl_sn = protocol['BCL']
            else:
                bcl_sn = [protocol['BCL']]

            # If the number of stimuli and BCLs are not the same, we complete the shorter one with the last value
            if len(n_stims_sn) < len(bcl_sn):
                n_stims_sn += [n_stims_sn[-1]] * (len(bcl_sn) - len(n_stims_sn))
            elif len(bcl_sn) < len(n_stims_sn):
                bcl_sn += [bcl_sn[-1]] * (len(n_stims_sn) - len(bcl_sn))
            # Print pacing protocol in several lines for S1, S2, etc
            print("Pacing protocol:", flush=True)
            for i, (n_stim, bcl) in enumerate(zip(n_stims_sn, bcl_sn), 1):
                print(f"S{i}: N_STIMS={n_stim}, BCL={bcl}", flush=True)

            # We create a dict where the key is the stimulus time and the value is
            # the list of nodes to activate at that time
            if 'FIRST_ACTIVATION_TIME' in protocol:
                activation_time = protocol['FIRST_ACTIVATION_TIME']
            else:
                # If not specified, we start at time 0
                activation_time = bcl_sn[0]  # Start with the first BCL

            for bcl, n_stim in zip(bcl_sn, n_stims_sn):
                for _ in range(n_stim):
                    # Schedule the activation at the current activation time
                    tissue.SetSystemEvent(tissue_module.SystemEventType.EXT_ACTIVATION, activation_time)
                    if activation_time not in activations:
                        activations[activation_time] = [initial_nodes, beat]
                    else:
                        activations[activation_time][0].extend(initial_nodes)
                        activations[activation_time][1] = beat # Overwrite the beat number
                    activation_time += bcl
                    beat += 1

    """ Activate by node id + time
            "ACTIVATE_NODES" : [
                {
                    "INITIAL_NODE_ID" : [1,2,3],
                    "ACTIVATION_TIMES" : [[500,1],[550,2]]
                },
                {
                    "INITIAL_NODE_ID" : [100],
                    "ACTIVATION_TIMES" : [[1500,4],[1550,5]]
                }
            ]
    """
    if "ACTIVATE_NODES" in cfg:
        for activation in cfg['ACTIVATE_NODES']:
            initial_nodes = activation['INITIAL_NODE_ID']
            for time, beat in activation['ACTIVATION_TIMES']:
                tissue.SetSystemEvent(tissue_module.SystemEventType.EXT_ACTIVATION, time)
                if time not in activations:
                    activations[time] = [initial_nodes, beat]
                else:
                    activations[time][0].extend(initial_nodes)
                    activations[time][1] = beat

    return activations

def run_simulation(output_dir, cfg):

    vtk_file = cfg['VTK_INPUT_FILE']

    # keep the original file name for saving the output
    out_file_name = os.path.splitext(os.path.basename(vtk_file))[0]

    # Load the grid from the VTK file
    grid = load_grid(vtk_file)

    # Create the tissue from the grid
    tissue = create_tissue(grid)

    # Set the timer for saving the VTK files
    tissue.SetTimer(tissue_module.SystemEventType.FILE_WRITE, cfg['VTK_OUTPUT_PERIOD'])  # time in ms

    # Schedule the activation protocol
    activations = schedule_activation(cfg, tissue)

    time = tissue.GetTime()

    while time < cfg['SIMULATION_DURATION']:
        tick = tissue.update(0)
        time = tissue.GetTime()

        if tick == tissue_module.SystemEventType.EXT_ACTIVATION:
            if time in activations:
                initial_nodes = activations[time][0]
                beat = activations[time][1]
                tissue.ExternalActivation(initial_nodes, time, beat)
                print("Beat at time:", time, flush=True)

        elif tick == tissue_module.SystemEventType.FILE_WRITE:
            # Update the cell states
            grid.point_data['State'] = tissue.GetStates()
            grid.point_data['APD'] = tissue.GetAPD()
            grid.point_data['DI'] = tissue.GetDI()
            grid.point_data['LastDI'] = tissue.GetLastDI()
            grid.point_data['CV'] = tissue.GetCV()
            grid.point_data['LAT'] = tissue.GetLAT()
            grid.point_data['LifeTime'] = tissue.GetLT()
            grid.point_data['Beat'] = tissue.GetBeat()


            grid.save(f"{os.path.join(output_dir, out_file_name)}_{int(time)}.vtk")


if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Usage: python arrythmic3D.py <output_directory>")
        print("       Please, provide an output directory containing the configuration (.json) file.")
        sys.exit(1)
    else:
        output_dir, config_file = check_directory(sys.argv[1])
        cfg = load_config_file(config_file)
        # Here you can modify pacing/activation procedure
        run_simulation(output_dir, cfg)
        print("Simulation finished", flush=True)
