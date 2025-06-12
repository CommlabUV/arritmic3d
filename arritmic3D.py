import numpy as np
import pyvista as pv
import tissue_module
from arr3D_config import check_directory, get_parameters, load_config_file, convert_to_cell_type, convert_to_tissue_region
import sys


def run_simulation(output_dir, config_file):

    cfg = load_config_file(config_file)
    vtk_file = cfg['VTK_INPUT_FILE']
    print(f"Reading file: {vtk_file}", flush=True)
    grid = pv.read(vtk_file)

    dims = grid.dimensions
    x_coords = np.unique(grid.points[:, 0])
    y_coords = np.unique(grid.points[:, 1])
    z_coords = np.unique(grid.points[:, 2])
    # We assume the spacing is un"CONFIG_FILE_NAME": "arr3D_config.json",iform
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

    # Remove innecessary data
    for key in grid.point_data.keys():
        if key != 'Cell_type':
            grid.point_data.remove(key)

    # Set the timer for saving the VTK files and succesive activations
    tissue.SetTimer(cfg['VTK_OUTPUT_PERIOD'])

    # First activation. The initial node can be an index (int) or a list of indexes.
    # If it is a list, we assume it is already in the correct format
    if isinstance(cfg['INITIAL_NODE_ID'], list):
        initial_node = cfg['INITIAL_NODE_ID']
    else:
        # If it is an int, we convert it to a list with one element.
        initial_node = [cfg['INITIAL_NODE_ID']]
    beat = 0
    last_activation_time = 0.0
    # Activate the tissue at the initial node
    tissue.ExternalActivation(initial_node, last_activation_time, beat)

    n_stims_sn = []
    bcl_sn = []
    # Pacing protocol (S1,S2,...,SN) with (BCL1, BCL2, ..., BCLN)
    if 'N_STIMS_PACING' in cfg and 'BCL' in cfg:
        # Again, can be a list or a single value
        if isinstance(cfg['N_STIMS_PACING'], list):
            n_stims_sn = cfg['N_STIMS_PACING']
        else:
            n_stims_sn = [cfg['N_STIMS_PACING']]

        if isinstance(cfg['BCL'], list):
            bcl_sn = cfg['BCL']
        else:
            bcl_sn = [cfg['BCL']]

        # If the number of stimuli and BCLs are not the same, we complete the shorter one with the last value
        if len(n_stims_sn) < len(bcl_sn):
            n_stims_sn += [n_stims_sn[-1]] * (len(bcl_sn) - len(n_stims_sn))
        elif len(bcl_sn) < len(n_stims_sn):
            bcl_sn += [bcl_sn[-1]] * (len(n_stims_sn) - len(bcl_sn))
        # Print pacing protocol in several lines for S1, S2, etc
        print("Pacing protocol:", flush=True)
        for i, (n_stim, bcl) in enumerate(zip(n_stims_sn, bcl_sn), 1):
            print(f"S{i}: N_STIMS={n_stim}, BCL={bcl}", flush=True)
    else:
        n_stims_sn = [0]
        bcl_sn = [0]

    n_stims = n_stims_sn.pop(0) if n_stims_sn else 0
    bcl = bcl_sn.pop(0) if bcl_sn else 0
    print(f"Initial stimulus: N_STIMS={n_stims}, BCL={bcl}", flush=True)

    while tissue.GetTime() < cfg['SIMULATION_DURATION']:
        tick = tissue.update(0)
        if tick:
            if n_stims > 0 and tissue.GetTime() >= last_activation_time + bcl:
                n_stims -= 1
                if n_stims <= 0:
                    # If there are no more stimuli, we move to the next S
                    n_stims = n_stims_sn.pop(0) if n_stims_sn else 0
                    bcl = bcl_sn.pop(0) if bcl_sn else 0
                    if n_stims > 0 and bcl > 0:
                        print(f"Moving to next stimulus: N_STIMS={n_stims}, BCL={bcl}", flush=True)
                # Every time we reach the BCL, we activate the tissue again
                last_activation_time = tissue.GetTime()
                beat += 1
                # Activate the tissue at the initial nodes
                tissue.ExternalActivation(initial_node, last_activation_time, beat)
                print("Beat at time:", last_activation_time, flush=True)

            # Update the cell states
            grid.point_data['State'] = tissue.GetStates()
            grid.point_data['APD'] = tissue.GetAPD()
            grid.point_data['CV'] = tissue.GetCV()


            grid.save(f"{output_dir}/vent{int(tissue.GetTime())}.vtk")


if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Usage: python arrythmic3D.py <output_directory>")
        print("       Please, provide an output directory containing the configuration (.json) file.")
        sys.exit(1)
    else:
        output_dir, config_file = check_directory(sys.argv[1])
        run_simulation(output_dir, config_file)
        print("Simulation finished", flush=True)
