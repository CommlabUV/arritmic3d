import os
import numpy as np
import pyvista as pv
import sys
import argparse
import json
import copy
import csv
import shutil

import arritmic3d
from . import arr3D_build_slab

from .arr3D_config import check_directory, get_vectorial_parameters, load_config_file, load_case_config, make_default_config, resolve_models_in_parameters
from .arr3D_activations import schedule_activation


def load_grid(vtk_file):
    """
    Load the VTK file and return the grid.
    """
    print(f"Reading file: {vtk_file}", flush=True)
    grid = pv.read(vtk_file)

    # Remove innecessary data
    for key in grid.point_data.keys():
        if key not in ['restitution_model', 'fibers_orientation', 'activation_region']:
            grid.point_data.remove(key)

    return grid

def create_tissue(grid, params):
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
    print("Grid Dimensions:", dims)
    print("Grid Spacing:", x_spacing, y_spacing, z_spacing)
    print("VTK-Scalars:", grid.point_data.keys())

    v_type = list(np.array(grid.point_data['restitution_model']))

    # Number of cells in each dimension
    ncells_x = dims[0]
    ncells_y = dims[1]
    ncells_z = dims[2]

    tissue = arritmic3d.CardiacTissue(ncells_x, ncells_y, ncells_z, x_spacing, y_spacing, z_spacing)

    vparams = get_vectorial_parameters(tissue, dims, params)
    print("Parameters:", params, flush=True)
    # Ensure fibers_orientation exists (default to [0,0,0] if missing)
    if 'fibers_orientation' in grid.point_data:
        fiber_or = list(map(list, grid.point_data['fibers_orientation']))
    else:
        fiber_or = [[0, 0, 0]] * (ncells_x * ncells_y * ncells_z)


    # Resolve CV/APD model names to package paths if provided (does not overwrite existing *_CONFIG_PATH)
    params = resolve_models_in_parameters(params)

    # Ensure restitution model paths are provided in the params dict
    # If not present, raise an informative error
    if 'APD_MODEL_CONFIG_PATH' not in params or 'CV_MODEL_CONFIG_PATH' not in params:
        raise ValueError("params must include 'APD_MODEL_CONFIG_PATH' and 'CV_MODEL_CONFIG_PATH' (or 'APD_MODEL'/'CV_MODEL' names resolvable to module-installed models).")
    apd_cfg = params['APD_MODEL_CONFIG_PATH']
    cv_cfg = params['CV_MODEL_CONFIG_PATH']
    # Initialize restitution and CV models using the provided file paths
    tissue.InitModels(apd_cfg, cv_cfg)
    print("Types of arguments passed to InitPy():", type(v_type), type(vparams), type(fiber_or), flush=True)
    tissue.InitPy(v_type, vparams, fiber_or)
    print("tissue initialized", flush=True)

    return tissue


def run_simulation(case_dir, cfg):

    vtk_file = cfg['VTK_INPUT_FILE']

    # keep the original file name for saving the output
    out_file_name = os.path.splitext(os.path.basename(vtk_file))[0]

    # Load the grid from the VTK file
    grid = load_grid(vtk_file)

    # Create the tissue from the grid, passing the loaded configuration dict
    tissue = create_tissue(grid, cfg)

    # Set the timer for saving the VTK files
    tissue.SetTimer(arritmic3d.SystemEventType.FILE_WRITE, cfg['VTK_OUTPUT_PERIOD'])  # time in ms

    # Schedule the activation protocol
    activations = schedule_activation(cfg, grid, tissue)

    time = tissue.GetTime()

    while time < cfg['SIMULATION_DURATION']:
        tick = tissue.update(0)
        time = tissue.GetTime()

        if tick == arritmic3d.SystemEventType.EXT_ACTIVATION:
            if time in activations:
                initial_nodes = activations[time][0]
                beat = activations[time][1]
                tissue.ExternalActivation(initial_nodes, time, beat)
                print("Beat at time:", time, flush=True)

        elif tick == arritmic3d.SystemEventType.FILE_WRITE:
            # Update the cell states
            grid.point_data['State'] = tissue.GetStates()
            grid.point_data['APD'] = tissue.GetAPD()
            grid.point_data['DI'] = tissue.GetLastDI()
            grid.point_data['CV'] = tissue.GetCV()
            grid.point_data['AP'] = tissue.GetAP()
            grid.point_data['LAT'] = tissue.GetLAT()
            grid.point_data['Beat'] = tissue.GetBeat()


            grid.save(f"{os.path.join(case_dir, out_file_name)}_{int(time):05d}.vtk")


def get_arg_parser():
    """
    Configures and returns the argument parser for the script.
    """
    parser = argparse.ArgumentParser(
        description="Run Arritmic3D cardiac electrophysiology simulation.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run simulation with default config search in case directory
  python arritmic3D.py /path/to/case_dir

  # Quick slab build + run (no pre-existing VTK), using defaults
  python arritmic3D.py --slab /path/to/case_dir --nnodes 20 20 5 --spacing 0.05 0.05 0.05

  # Quick slab build + run using a provided config file as base
  python arritmic3D.py --slab -c /path/to/arr3D_config.json /path/to/case_dir --nnodes 20 20 5 --spacing 0.05 0.05 0.05

  # Specify VTK input file and config file
  python arritmic3D.py /path/to/case_dir \\
    --input-file /path/to/tissue.vtk \\
    --config-file /path/to/config.json

  # Override specific configuration parameters
  python arritmic3D.py /path/to/case_dir \\
    --input-file /path/to/tissue.vtk \\
    --config-param SIMULATION_DURATION=8000 \\
    --config-param VTK_OUTPUT_PERIOD=50
        """
    )

    # Positional arguments
    parser.add_argument(
        "case_dir",
        help="Output directory where results will be saved."
    )

    # Input/Output group
    io_group = parser.add_argument_group("Input/Output", "Configure input tissue file and case directory.")
    io_group.add_argument(
        "--input-file",
        "-i",
        dest="input_file",
        metavar="FILE",
        default=None,
        help="Path to input tissue file (VTK format). If omitted, VTK_INPUT_FILE is taken from the configuration. "
             "When using --slab, this option is not allowed."
    )
    io_group.add_argument(
        "--slab",
        dest="slab",
        action="store_true",
        help="Generate a slab VTK and run the simulation."
            "You can configure the slab using any of the build_slab script options after --slab (run build_slab --help for details)."
            "When using --slab, the generated VTK you are not allowed to use the --input-file option and any VTK_INPUT_FILE present in the JSON is ignored."
             "The slab is saved to case_dir/cases/slab.vtk."
    )
    io_group.add_argument(
        "--output-run-config",
        dest="output_run_config",
        action="store_true",
        default=True,
        help="Save the actual run configuration to the case directory for reproducibility."
            "A configuration JSON file is generated with all the parameters that have been modified to run the simulation."
            "The configuration file is saved to case_dir/arr3D_config_run.json (default: enabled)."
    )
    io_group.add_argument(
        "--no-output-run-config",
        dest="output_run_config",
        action="store_false",
        help="Do not save the run configuration."
    )

    # Configuration group
    config_group = parser.add_argument_group("Configuration", "Load and override configuration parameters.")
    config_group.add_argument(
        "--config-file",
        "-c",
        dest="config_file",
        metavar="FILE",
        default=None,
        help="Path to configuration JSON file. If omitted, searches for arr3D_config.json in case_dir."
    )
    config_group.add_argument(
        "--config-param",
        "-p",
        action="append",
        dest="config_params",
        metavar="KEY=VALUE",
        default=None,
        help="Override a single configuration parameter (KEY=VALUE format). Can be used multiple times. "
             "Example: --config-param SIMULATION_DURATION=8000 --config-param VTK_OUTPUT_PERIOD=50"
    )
    parser.add_argument(
        "--test",
        action="store_true",
        help="Run a built-in S1-S2 test case in the specified case_dir. The directory must not exist or must be empty."
    )

    return parser

def apply_config_overrides(cfg, args):
    """
    Apply CLI overrides on top of the loaded cfg.
    - KEY=VALUE entries from --config-param (-p)
    - --input-file (-i) override for VTK_INPUT_FILE
    VALUE is parsed with json.loads when possible; falls back to string.
    All paths are resolved to absolute and stored as-is (for runtime use with cwd).
    """
    config_params = getattr(args, "config_params", None) or []

    path_keys = {"VTK_INPUT_FILE", "APD_MODEL_CONFIG_PATH", "CV_MODEL_CONFIG_PATH"}

    for kv in config_params:
        if "=" not in kv:
            raise ValueError(f"Invalid --config-param format (expected KEY=VALUE): {kv}")
        key, value = kv.split("=", 1)
        try:
            parsed = json.loads(value)
        except Exception:
            parsed = value

        # If it's a path key, resolve to absolute
        if key in path_keys and isinstance(parsed, str):
            abs_path = parsed if os.path.isabs(parsed) else os.path.abspath(parsed)
            cfg[key] = abs_path
        else:
            cfg[key] = parsed

    # Explicit --input-file override takes precedence over -p VTK_INPUT_FILE
    if getattr(args, "input_file", None):
        inp = args.input_file
        cfg["VTK_INPUT_FILE"] = inp if os.path.isabs(inp) else os.path.abspath(inp)

    return cfg


def ensure_abs_paths(cfg):
    """Convert known path keys in cfg to absolute paths in-place."""
    path_keys = {"VTK_INPUT_FILE", "APD_MODEL_CONFIG_PATH", "CV_MODEL_CONFIG_PATH"}
    for key in path_keys:
        if key in cfg and isinstance(cfg[key], str):
            cfg[key] = cfg[key] if os.path.isabs(cfg[key]) else os.path.abspath(cfg[key])


def resolve_input_file(cfg):
    """
    Validate that VTK_INPUT_FILE in cfg exists.
    Paths are interpreted as absolute or relative to cwd.
    Raises FileNotFoundError if the file does not exist.
    """
    vtk_path_cfg = cfg.get("VTK_INPUT_FILE")
    if not vtk_path_cfg:
        raise FileNotFoundError("VTK input file not specified. Provide --input-file or set VTK_INPUT_FILE in config.")

    vtk_abs = vtk_path_cfg if os.path.isabs(vtk_path_cfg) else os.path.abspath(vtk_path_cfg)
    if not os.path.isfile(vtk_abs):
        raise FileNotFoundError(f"Input VTK file not found: {vtk_abs}")

    return vtk_abs

def prepare_cfg_for_output(cfg, case_dir):
    """
    Return a copy of cfg with paths relative to case_dir (for saving to case_dir).
    Original cfg retains absolute paths (for runtime).
    """
    path_keys = ["VTK_INPUT_FILE", "APD_MODEL_CONFIG_PATH", "CV_MODEL_CONFIG_PATH"]
    cfg_for_output = copy.deepcopy(cfg)
    for key in path_keys:
        if key in cfg_for_output:
            val = cfg_for_output[key]
            abs_val = val if os.path.isabs(val) else os.path.abspath(val)
            cfg_for_output[key] = os.path.relpath(abs_val, case_dir)
    return cfg_for_output

def get_config_file_path(args):
    """
    Resolve the configuration file path given CLI args.
    - If --config-file is provided: use it as-is (error if missing).
    - If not provided: use check_directory(case_dir) which returns a path or None.
    """
    if args.config_file:
        provided = args.config_file
        if not os.path.isfile(provided):
            raise FileNotFoundError(f"Config file not found: {provided}")
        return provided
    return check_directory(args.case_dir)

def ensure_vtk_input(cfg):
    """
    Ensure VTK_INPUT_FILE exists and print the resolved path. Exits on error.
    """
    try:
        vtk_file_abs = resolve_input_file(cfg)
        print(f"Using VTK file: {vtk_file_abs}", flush=True)
        return vtk_file_abs
    except FileNotFoundError as e:
        print(f"Error ensuring VTK input: {e}", flush=True)
        raise


def save_run_configuration(cfg, case_dir):
    """
    Saves the run configuration (with paths relative to case_dir) in case_dir/arr3D_config_run.json.
    Also copies the model config files and all files listed in them to restitutionModels/.
    Updates the JSON to point to those relative paths.
    Only copies files if they are not already inside the output directory.
    """
    used_path = os.path.join(case_dir, "arr3D_config_run.json")
    cfg_for_output = prepare_cfg_for_output(cfg, case_dir)

    import csv

    model_keys = ["APD_MODEL_CONFIG_PATH", "CV_MODEL_CONFIG_PATH"]
    models_dir = os.path.join(case_dir, "restitutionModels")
    os.makedirs(models_dir, exist_ok=True)

    for key in model_keys:
        if key in cfg and os.path.isfile(cfg[key]):
            src = os.path.abspath(cfg[key])
            dst = os.path.join(models_dir, os.path.basename(src))
            abs_case_dir = os.path.abspath(case_dir)

            # Only copy if not already in output dir
            try:
                if not os.path.commonpath([src, abs_case_dir]) == abs_case_dir:
                    shutil.copy2(src, dst)
                else:
                    dst = src  # Already in output dir, don't copy
            except ValueError:
                # Assume different drives on Windows, treat as not in output dir
                shutil.copy2(src, dst)

            # Read the CSV and copy the files from the second column
            with open(src, "r") as f:
                reader = csv.reader(f)
                for row in reader:
                    if not row or row[0].strip().startswith("#"):
                        continue
                    if len(row) < 2:
                        continue
                    model_file = row[1].strip()
                    if not model_file:
                        continue
                    src_model_file = os.path.abspath(os.path.join(os.path.dirname(src), model_file))
                    dst_model_file = os.path.join(models_dir, os.path.basename(model_file))
                    # Only copy if not already in output dir
                    if not os.path.commonpath([src_model_file, abs_case_dir]) == abs_case_dir:
                        if os.path.isfile(src_model_file):
                            shutil.copy2(src_model_file, dst_model_file)
                    # else: already in output dir, do not copy
            # Update the path in the JSON to be relative from case_dir
            cfg_for_output[key] = os.path.relpath(dst, abs_case_dir)

    with open(used_path, "w") as fh:
        json.dump(cfg_for_output, fh, indent=2)
    print(f"Saved run configuration to {used_path}", flush=True)

def generate_slab_to_output(case_dir, slab_args):
    """
    Parse build_slab options and generate the slab VTK into case_dir/cases/slab.vtk.
    Returns absolute path to the generated VTK.
    """
    cases_dir = os.path.join(case_dir, "input_data")
    os.makedirs(cases_dir, exist_ok=True)
    slab_path = os.path.abspath(os.path.join(cases_dir, "slab.vtk"))

    # Parse build_slab args with forced positional output_file set to our default path
    bs_parser = arr3D_build_slab.get_argument_parser()
    # Ensure our default output_file is used; do not accept alternative output path
    bs_ns = bs_parser.parse_args([slab_path] + slab_args)

    grid = arr3D_build_slab.build_slab(bs_ns, save = True)

    return slab_path


def run_arritmic3D(case_dir, config : dict = {}, save_run_config=True):
    """
    Run the Arritmic3D simulation in the given case directory with the provided configuration dict

    Arguments:
    - case_dir: Output directory where results will be saved.
    - config: Configuration dictionary with simulation parameters (see documentation). Fields provided here override those in any configuration file found in case_dir. If no configuration file is found, this config is applied on top of defaults.
    - save_run_config: If True (default), saves the actual run configuration to case_dir/arr3D_config_run.json.
    """

    # Read configuration from case directory
    case_config = load_case_config(case_dir)

    # If no configuration found raise an error
    if not config and not case_config:
        raise FileNotFoundError("No configuration file found. Provide --config-file or ensure arr3D_config.json exists in case_dir.")

    # Apply overrides from config argument on top of case_config (if exists) or defaults (if not). Paths are resolved to absolute.
    if not case_config:
        config = make_default_config() | config
    else:
        config = case_config | config


    # Validate that VTK input file exists
    ensure_vtk_input(config)

    # Ensure model names are resolved to concrete config paths and paths are absolute
    # (idempotent: if already resolved/absolute, no change)
    resolve_models_in_parameters(config)
    ensure_abs_paths(config)

    # Save simulation configuration (relative paths) if requested
    if save_run_config:
        save_run_configuration(config, case_dir)

    # Run simulation with runtime config (absolute paths)
    run_simulation(case_dir, config)
    print("Simulation finished", flush=True)

def run_test_case(output_dir):
    """
    Generate and run a built-in S1-S2 test case in the given output directory.
    Uses build_slab and the standard config/output logic.

    Arguments:
    - output_dir: Output directory where results will be saved. Must not exist or be empty
    """
    if os.path.exists(output_dir) and os.listdir(output_dir):
        raise FileExistsError(f"Output directory '{output_dir}' must not exist or must be empty.")
    os.makedirs(output_dir, exist_ok=True)

    # Prepare slab args: south side activation, activation_region=1
    slab_args = [
        "--nnodes", "15", "15", "5",
        "--spacing", "0.4", "0.4", "0.4",
        "--region-by-side", "south", "1",
        "--field", "restitution_model", "2",
        "--region", '{"shape" : "square", "cx" : 3.0, "cy" : 3.0, "r1" : 2.0, "r2" : 2.0, "restitution_model" : 5}'
    ]
    # Generate slab using the standard build_slab logic (handles regions, fields, etc.)
    slab_vtk = generate_slab_to_output(output_dir, slab_args)

    # Prepare minimal config for S1-S2 protocol
    config = make_default_config()
    config["VTK_INPUT_FILE"] = slab_vtk
    config["APD_MODEL"] = "TenTuscher"
    config["CV_MODEL"] = "TenTuscher"
    config["SIMULATION_DURATION"] = 5000
    config["VTK_OUTPUT_PERIOD"] = 5
    config["PROTOCOL"] = [
        {
            "ACTIVATION_REGION": 1,
            "FIRST_ACTIVATION_TIME": 100,
            "N_STIMS_PACING": [6,3],
            "BCL": [600,400]
        }
    ]

    # Use the standard config/output logic (with path conversion)
    run_arritmic3D(output_dir, config, save_run_config=True)
    print("Test case finished", flush=True)

def main():

    parser = get_arg_parser()
    # Parse known args for arritmic3D; remainder belongs to build_slab if --slab is set
    args, remainder = parser.parse_known_args()

    # Handle --test option
    if args.test:
        run_test_case(args.case_dir)
        return

    # With --slab, do not allow --input-file (slab provides the VTK). Allow --config-file as base cfg.
    if args.slab and args.input_file:
        raise ValueError("--slab cannot be used together with --input-file. The slab generator will create its own VTK.")

    # Resolve configuration file path and load configuration
    config_file = get_config_file_path(args)

    # If no config file found, raise exception only if not using --slab (which can work with defaults)
    if not config_file and not args.slab:
        raise FileNotFoundError("No configuration file found. Provide --config-file or ensure arr3D_config.json exists in case_dir.")
    else:
        cfg = load_config_file(config_file) if config_file else make_default_config()

    # Apply all CLI overrides (paths resolved to absolute), including --input-file if provided
    cfg = apply_config_overrides(cfg, args)

    # Slab generation and VTK override (slab wins over any previous VTK)
    if args.slab:
        slab_vtk = generate_slab_to_output(args.case_dir, remainder)
        cfg["VTK_INPUT_FILE"] = slab_vtk

    # Execute the simulation with the prepared configuration
    run_arritmic3D(args.case_dir, config = cfg, save_run_config = args.output_run_config)

if __name__ == "__main__":
    main()