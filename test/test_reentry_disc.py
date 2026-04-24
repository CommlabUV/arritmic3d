import os
import arritmic3d as a3d
from plot_functions import (plot_vtk, plot_animation, delete_case_dir)
"""
Here are the *main characteristics of the domain*:
- Size: 215 x 215 x 1 elements, with an edge length of 0.35 mm (hexahedral mesh → final dimensions: 75.25 x 75.25 x 0.35 mm)
- Number of cells: 46,225
- Number of points: 93,312
- Mesh type: isotropic
- The geometry corresponds to a Healthy x ENDO x TorORd model, except for a central region with a radius of 2.5 cm (which, in this case is a HCM Moderate PoM 17 x ENDO x TorORd model. The PoM 17 refers to a set of simulation that I'm doing for a parallel project, don't focus on it for the moment).

*Stimulation protocol:*
- I apply 3 x S1 stimuli (cycle length = 1000 ms) from the bottom line (see attached screenshot)
- The S2 stimulus is applied from a point as shown in the attached figure. In my case, this corresponds to point ID 46265
- With S2 at 377 ms, you should observe a re-entry
- With S2 at 382 ms, you should observe ventricular tachycardia (i.e., more than one re-entry)
"""

# We also need the argument parser for the build_slab utility
import arritmic3d.arr3D_build_slab as arr_build

# --- STEP 1: Configure the case directory ---
case_dir = "out_test/4.reentry_disc"
delete_case_dir(case_dir)

# Ensure the subfolder for the slab exists
os.makedirs(os.path.join(case_dir, "input_data"), exist_ok=True)

# --- STEP 2: Build the slab ---
# We configure a slab similar to the default test_case, but with two regions:
# 1. Background field set to Healthy (model 2)
# 2. Square region set to Border Zone (model 5)
print("\n--- Building the slab with two regions ---")
slab_path = os.path.join(case_dir, "input_data", "slab.vtk")
slab_args = [
    slab_path,
    "--nnodes", "215", "215", "2",
    "--spacing", "0.35", "0.35", "0.35",
    "--region-by-side", "south", "1",
    "--field", "restitution_model", "1",
    "--region", '{"shape" : "circle", "cx" : 37.65, "cy" : 37.65, "r1" : 31.0, "r2" : 31.0, "restitution_model" : 2}',
    "--region", '{"shape" : "circle", "cx" : 37.65, "cy" : 4.0,  "r1" : 0.4,   "r2" : 0.4, "activation_region" : 2}'
]

# Build and save the slab
a3d.build_slab(args_list=slab_args, save=True)
print("Slab generated successfully!")

# Visualize the slab
plot_vtk(slab_path, field="restitution_model", plt_show=True, title="Slab")

# --- STEP 3: Configure the simulation ---
print("\n--- Configuring simulation ---")
config = {
    "VTK_INPUT_FILE": slab_path,
    "APD_MODEL_CONFIG_PATH": "restitution/configDisc_APD.csv",
    "CV_MODEL_CONFIG_PATH": "restitution/configDisc_CV.csv",
    "CORRECTION_FACTOR_CV": 1.0,
    "CORRECTION_FACTOR_APD": 1.0,
    "ELECTROTONIC_EFFECT": 0.0,
    "INITIAL_APD": 430,
    "SIMULATION_DURATION": 3500,
    "VTK_OUTPUT_PERIOD": 10,
    "VTK_OUTPUT_INITIAL_TIME": 2000,
    "PROTOCOL": [
        {
            "ACTIVATION_REGION": 1,
            "N_STIMS_PACING": [3],
            "BCL": [1000]
        }
    ],
    "ACTIVATE_NODES": [
        {
            "ACTIVATION_REGION": 2,
            "ACTIVATION_TIMES": [[2358,4]]
        }
    ]
}

# --- STEP 4: Run the simulation ---
print("\n--- Running simulation ---")
a3d.arritmic3d(case_dir, config=config)
print("Simulation completed!")

# --- STEP 5: Visualize a single result ---
print("\n--- Visualizing a frame at 2850ms ---")
plot_vtk(os.path.join(case_dir, f"slab_02850.vtk"), plt_show=True, title="t=2850ms")
