import os
import numpy as np
import pyvista as pv
import tissue_module
from arr3D_config import check_directory, get_parameters, load_config_file
import sys


def load_grid(vtk_file):
    """
    Load the VTK file and return the grid.
    """
    print(f"Reading file: {vtk_file}", flush=True)
    grid = pv.read(vtk_file)

    # Remove innecessary data
    for key in grid.point_data.keys():
        if key not in ['restitution_model', 'fibers_orientation', 'stimulation_sites']:
            grid.point_data.remove(key)

    return grid

def create_tissue(grid):
    """ Create a tissue object from the grid.
    The grid is expected to have the following point data:
    - restitution_model: the type of each cell (e.g., 'Endocardium', 'Epicardium', 'Myocardium')
    - fibers_orientation: the orientation of the fibers in the cell. Optional. If not present, isotropic conduction is assumed.
    """

    print("Creating tissue from grid", flush=True)
    if 'restitution_model' not in grid.point_data:
        raise ValueError("The grid does not contain 'restitution_model' in point data. Please check the VTK file.")

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

    v_type = list(np.array(grid.point_data['restitution_model']))

    # Number of cells in each dimension
    ncells_x = dims[0]
    ncells_y = dims[1]
    ncells_z = dims[2]

    tissue = tissue_module.CardiacTissue(ncells_x, ncells_y, ncells_z, x_spacing, y_spacing, z_spacing)

    vparams, params = get_parameters(tissue, dims, config_file)
    print("Parameters:", params, flush=True)
    fiber_or = list(map(list,grid.point_data['fibers_orientation']))
    tissue.InitModels("restitutionModels/config_TenTuscher_APD.csv","restitutionModels/config_TenTuscher_CV.csv")
    print("Types of arguments passed to InitPy():", type(v_type), type(vparams), type(fiber_or), flush=True)
    tissue.InitPy(v_type, vparams, fiber_or)
    print("tissue initialized", flush=True)

    return tissue

def schedule_activation(cfg,grid,tissue):

    activations = {}

    # Pacing protocols (S1,S2,...,SN) with (BCL1, BCL2, ..., BCLN)
    # Activation by pacing protocol
    """
    "PROTOCOL" : [ {
        "STIMULATION_SITES": [
            10713,
            10714,
            10715,
            10716,
            10717
        ],
        "FIRST_ACTIVATION_TIME" : 400,
        "N_STIMS_PACING": [6,3],
        "BCL": [600,400,300,200]
    } ],

    """
    if "PROTOCOL" in cfg:
        protocols = cfg['PROTOCOL']
        beat = 0
        for protocol in protocols:

            # Activation. The initial node can be an index (int) or a list of indexes.
            # If it is a list, we assume it is already in the correct format
            if isinstance(protocol['STIMULATION_SITES'], list):
                initial_nodes = protocol['STIMULATION_SITES']
            elif isinstance(protocol['STIMULATION_SITES'], int):
                # We seek for nodes with 'stimulation_sites' field equal to that value
                stim_field = np.array(grid.point_data['stimulation_sites'])
                initial_nodes = np.where(stim_field == protocol['STIMULATION_SITES'])[0].tolist()
            else:
                raise ValueError("STIMULATION_SITES must be an int or a list of ints.")

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
                    "STIMULATION_SITES" : [1,2,3],
                    "ACTIVATION_TIMES" : [[500,1],[550,2]]
                },
                {
                    "STIMULATION_SITES" : [100],
                    "ACTIVATION_TIMES" : [[1500,4],[1550,5]]
                }
            ]
    """
    if "ACTIVATE_NODES" in cfg:
        for activation in cfg['ACTIVATE_NODES']:
            if isinstance(activation['STIMULATION_SITES'], list):
                initial_nodes = activation['STIMULATION_SITES']
            elif isinstance(activation['STIMULATION_SITES'], int):
                initial_nodes = np.where(stim_field == protocol['STIMULATION_SITES'])[0].tolist()
            else:
                raise ValueError("STIMULATION_SITES must be an int or a list of ints.")

            for time, beat in activation['ACTIVATION_TIMES']:
                tissue.SetSystemEvent(tissue_module.SystemEventType.EXT_ACTIVATION, time)
                if time not in activations:
                    activations[time] = [initial_nodes, beat]
                else:
                    activations[time][0].extend(initial_nodes)
                    activations[time][1] = beat # Overwrite the beat number

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
    activations = schedule_activation(cfg, grid, tissue)

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
