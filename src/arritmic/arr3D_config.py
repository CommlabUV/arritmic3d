import sys
import argparse
import os
import json

def check_directory(output_dir):
    """Check if the output directory exists and return a configuration file path if present.
    - If output_dir does not exist: raise FileNotFoundError.
    - If no config JSON is found: return None.
    - If a config JSON exists: return its path.
    """
    if not os.path.isdir(output_dir):
        raise FileNotFoundError(f"The directory {output_dir} does not exist.")

    config_file = None
    for file in os.listdir(output_dir):
        if file.endswith('.json'):
            config_file = os.path.join(output_dir, file)
            break

    if config_file and os.path.exists(config_file):
        return config_file
    return None

def load_config_file(config_file, resolve_to_absolute=False, path_keys=["VTK_INPUT_FILE", "APD_MODEL_CONFIG_PATH", "CV_MODEL_CONFIG_PATH"]):
    """
    Load configuration from JSON file.
    If resolve_to_absolute=True, convert relative paths to absolute paths (relative to the JSON file location).
    If the file does not exist, return an empty dict (default parameters).
    """
    parameters = {}

    if config_file and os.path.exists(config_file):
        with open(config_file, 'r') as f:
            parameters = json.load(f)
    else:
        print(f"Configuration file {config_file} not found. Using default parameters.")

    if resolve_to_absolute and config_file:
        base_dir = os.path.dirname(os.path.abspath(config_file))
        for key in path_keys:
            if key in parameters and isinstance(parameters[key], str):
                val = parameters[key]
                # If absolute, keep it; if relative, resolve relative to the JSON
                parameters[key] = val if os.path.isabs(val) else os.path.abspath(os.path.join(base_dir, val))

    return parameters


def get_vectorial_parameters(tissue, dims, prms):

    ncells_x, ncells_y , ncells_z = dims
    initial_apd                 = prms['INITIAL_APD']
    initial_cvr                 = prms['COND_VELOC_TRANSVERSAL_REDUCTION']
    initial_cfapd               = prms['CORRECTION_FACTOR_APD']
    initial_cfcvbz              = prms['CORRECTION_FACTOR_CV']
    initial_electrotonic_effect = prms['ELECTROTONIC_EFFECT']
    initial_min_potential       = prms['MIN_POTENTIAL']
    initial_safety_factor       = prms['SAFETY_FACTOR']

    # vectorial parameters
    v_apd                       = [initial_apd] * (ncells_x * ncells_y * ncells_z)
    v_cvr                       = [initial_cvr] * (ncells_x * ncells_y * ncells_z)
    v_cfapd                     = [initial_cfapd] * (ncells_x * ncells_y * ncells_z)
    v_cfcvbz                    = [initial_cfcvbz] * (ncells_x * ncells_y * ncells_z)
    v_electrotonic_effect       = [initial_electrotonic_effect] * (ncells_x * ncells_y * ncells_z)
    v_min_potential             = [initial_min_potential] * (ncells_x * ncells_y * ncells_z)
    v_safety_factor             = [initial_safety_factor] * (ncells_x * ncells_y * ncells_z)

    vparameters = {}
    vparameters['INITIAL_APD']   = v_apd
    vparameters['COND_VELOC_TRANSVERSAL_REDUCTION'] = v_cvr
    vparameters['CORRECTION_FACTOR_APD'] = v_cfapd
    vparameters['CORRECTION_FACTOR_CV'] = v_cfcvbz
    vparameters['ELECTROTONIC_EFFECT'] = v_electrotonic_effect
    vparameters['MIN_POTENTIAL'] = v_min_potential
    vparameters['SAFETY_FACTOR'] = v_safety_factor

    return vparameters

def make_default_config():
    """
    Create a minimal default configuration dict to run without a JSON file.
    Paths are assumed relative to the current working directory; saving to JSON will convert to paths relative to output_dir.
    """
    return {
        "CV_MODEL_CONFIG_PATH": os.path.abspath("restitutionModels/config_TorOrd_CV.csv"),
        "APD_MODEL_CONFIG_PATH": os.path.abspath("restitutionModels/config_TorOrd_APD.csv"),
        "COND_VELOC_TRANSVERSAL_REDUCTION": 0.25,
        "CORRECTION_FACTOR_APD": 1.0,
        "CORRECTION_FACTOR_CV": 1.0,
        "ELECTROTONIC_EFFECT": 0.85,
        "INITIAL_APD": 300.0,
        "MIN_POTENTIAL": 0.0,
        "SAFETY_FACTOR": 1.0,
        "VTK_OUTPUT_SAVE": True,
        "VTK_OUTPUT_PERIOD": 20.0,
        "SENSORS_OUTPUT_SAVE": True,
        "SIMULATION_DURATION": 6000.0,
        # PROTOCOL / ACTIVATE_NODES intentionally omitted; can be provided via --config-param
    }



