import os
import arritmic3d as a3d
from plot_functions import (plot_vtk, plot_animation, delete_case_dir)

# We also need the argument parser for the build_slab utility
import arritmic3d.arr3D_build_slab as arr_build

# --- STEP 1: Configure the case directory ---
case_dir = "out_test/5.rotor"
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
    "--region", '{"shape" : "square", "cx" : 21.5, "cy" : 24.5, "r1" : 22.0, "r2" : 30.0, "activation_region" : 2}'
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
    "SIMULATION_DURATION": 4000,
    "VTK_OUTPUT_PERIOD": 10,
    "VTK_OUTPUT_INITIAL_TIME": 2400,
    "PROTOCOL": [
        {
            "ACTIVATION_REGION": 1,
            "N_STIMS_PACING": [6],
            "BCL": [500]
        }
    ],
    "ACTIVATE_NODES": [
        {
            "ACTIVATION_REGION": 2,
            "ACTIVATION_TIMES": [[2820,10]]
        }
    ]
}

# --- STEP 4: Run the simulation ---
print("\n--- Running simulation ---")
a3d.arritmic3d(case_dir, config=config)
print("Simulation completed!")

exit(0)

# --- STEP 5: Visualize a single result ---
print("\n--- Visualizing a frame at 500ms ---")
plot_vtk(os.path.join(case_dir, f"slab_00040.vtk"), plt_show=True, title="t=40ms")

# --- STEP 6: Show an animation of the simulation ---
print("\n--- Showing an animation of the simulation ---")
#plot_animation(case_dir, field="AP")

# Clean the case dir
delete_case_dir(case_dir)

# Ensure the subfolder for the slab exists
os.makedirs(os.path.join(case_dir, "input_data"), exist_ok=True)

# --- STEP 7: Set a slab with CORE region (model = 7) ---
print("\n--- Building the slab with CORE region ---")
slab_args = [
    slab_path,
    "--nnodes", "45", "45", "5",
    "--spacing", "0.1", "0.1", "0.1",
    "--region-by-side", "south", "1",
    "--field", "restitution_model", "2",
    "--region", '{"shape" : "square", "cx" : 2.25, "cy" : 2.25, "r1" : 1.0, "r2" : 1.0, "restitution_model" : 5}',
    "--region", '{"shape" : "square", "cx" : 2.25, "cy" : 2.25, "r1" : 0.5, "r2" : 0.5, "restitution_model" : 7}'
]

# Build and save the slab
a3d.build_slab(args_list=slab_args, save=True)
print("Slab generated successfully!")

# Visualize the slab
plot_vtk(slab_path, field="restitution_model", plt_show=True, title="Slab")

# --- STEP 8: Configure the simulation ---
print("\n--- Configuring simulation ---")
config = {
    "VTK_INPUT_FILE": slab_path,
    "APD_MODEL": "TenTuscher",
    "CV_MODEL": "TenTuscher",
    "SIMULATION_DURATION": 1000,
    "VTK_OUTPUT_PERIOD": 10,
    "PROTOCOL": [
        {
            "ACTIVATION_REGION": 1,
            "FIRST_ACTIVATION_TIME": 0,
            "N_STIMS_PACING": [3],
            "BCL": [400]
        }
    ]
}

# --- STEP 9: Run the simulation ---
print("\n--- Running simulation ---")
a3d.arritmic3d(case_dir, config=config)
print("Simulation completed!")

# --- STEP 10: Visualize the results ---
print("\n--- Visualizing a frame at 500ms ---")
plot_vtk(os.path.join(case_dir, f"slab_00040.vtk"), plt_show=True, title="t=40ms")
plot_vtk(os.path.join(case_dir, f"slab_00060.vtk"), plt_show=True, title="t=60ms")

# Show an animation of the simulation
print("\n--- Showing an animation of the simulation ---")
plot_animation(case_dir, field="AP")
