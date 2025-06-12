import sys
import argparse
import os
import json
import tissue_module  # Assuming this is the module containing CardiacTissue and related classes

def check_directory(output_dir):
    """Check if the output directory exists and contains a configuration file."""
    if not os.path.isdir(output_dir):
        print(f"Error: The directory {output_dir} does not exist.")
        print("        Please, create the output directory and introduce the configuration file (.json) inside .") 
        exit(1)
    else:
        for file in os.listdir(output_dir):
            if file.endswith('.json'):
                config_file = os.path.join(output_dir, file)
                break   
        if not os.path.exists(config_file):
            print(f"Error: No configuration file (.json) found in {output_dir}.")
            exit(1)
        else:
            # If the config file is found, proceed with the simulation     
            print(f"Configuration file found: {config_file}")
            # Clean the vtk files in the output directory
            #for file in os.listdir(output_dir):
            #    if file.endswith('.vtk'):
            #        os.remove(os.path.join(output_dir, file))
            return output_dir, config_file    


def load_config_file(config_file):
    parameters = {}
    """Load parameters from a JSON file or return default parameters."""
    try:
        # If the json file exists, we load the parameters from it
        if os.path.exists(config_file):
            with open(config_file, "r") as f:
                parameters = json.load(f)
    except FileNotFoundError:   
        print(f"Configuration file {config_file} not found. Using default parameters.")
    
    return parameters


def get_parameters(tissue, dims, config_file):

    prms = load_config_file(config_file)
    ncells_x, ncells_y , ncells_z = dims
    initial_apd                 = prms['INITIAL_APD']
    initial_cvr                 = prms['COND_VELOC_TRANSVERSAL_REDUCTION']
    initial_cfapd               = prms['CORRECTION_FACTOR_APD']
    initial_cfcvbz              = prms['CORRECTION_FACTOR_CV_BORDER_ZONE']
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
    vparameters['CORRECTION_FACTOR_CV_BORDER_ZONE'] = v_cfcvbz
    vparameters['ELECTROTONIC_EFFECT'] = v_electrotonic_effect
    vparameters['MIN_POTENTIAL'] = v_min_potential
    vparameters['SAFETY_FACTOR'] = v_safety_factor

    return vparameters, prms

# Conversion of int to CellType
def convert_to_cell_type(cell_type):
    if cell_type == 0:
        return tissue_module.CellType.HEALTHY
    elif cell_type == 1:
        return tissue_module.CellType.BORDER_ZONE
    else:
        return tissue_module.CellType.CORE

# Conversion to tissue region
def convert_to_tissue_region(region):
    if region == 0:
        return tissue_module.TissueRegion.ENDO
    elif region == 1:
        return tissue_module.TissueRegion.MID
    else:
        return tissue_module.TissueRegion.EPI

