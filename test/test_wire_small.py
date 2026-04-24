import os
import arritmic3d as a3d
from plot_functions import (plot_vtk, plot_animation, delete_case_dir)

# We also need the argument parser for the build_slab utility
import arritmic3d.arr3D_build_slab as arr_build

# --- STEP 1: Configure the case directory ---
case_dir = "out_test/6.wire"
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
    "--nnodes", "2", "25", "2",
    "--spacing", "0.2", "0.2", "0.2",
    "--region-by-side", "south", "1",
    "--field", "restitution_model", "2",
    "--region", '{"shape" : "square", "cx" : 0.0, "cy" : 5.0, "r1" : 2.0, "r2" : 2.0, "restitution_model" : 5}'
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
    "APD_MODEL": "TenTuscher",
    "CV_MODEL": "TenTuscher",
    "CORRECTION_FACTOR_CV": 1.0,
    "CORRECTION_FACTOR_APD": 1.0,
    "ELECTROTONIC_EFFECT": 0.0,
    "INITIAL_APD" : 360,
    "SIMULATION_DURATION": 7000,
    "VTK_OUTPUT_PERIOD": 1,
    "VTK_OUTPUT_INITIAL_TIME": 5400,
    "PROTOCOL": [
        {
            "ACTIVATION_REGION": 1,
            "N_STIMS_PACING": [10,3],
            "BCL": [600,375]
        }
    ]
}

# --- STEP 4: Run the simulation ---
print("\n--- Running simulation ---")
a3d.arritmic3d(case_dir, config=config)
print("Simulation completed!")
