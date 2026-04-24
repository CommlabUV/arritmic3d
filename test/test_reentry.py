import os
import arritmic3d as a3d
from plot_functions import (plot_vtk, plot_animation, delete_case_dir)

# We also need the argument parser for the build_slab utility
import arritmic3d.arr3D_build_slab as arr_build

# --- STEP 1: Configure the case directory ---
case_dir = "out_test/3.reentry"
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
    "--nnodes", "70", "70", "2",
    "--spacing", "0.1", "0.1", "0.1",
    "--region-by-side", "south", "1",
    "--field", "restitution_model", "2",
    "--region", '{"shape" : "square", "cx" : 3.5, "cy" : 3.5, "r1" : 2.0, "r2" : 2.0, "restitution_model" : 5}'
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
    "ELECTROTONIC_EFFECT": 0.0,
    "CV_MEMORY_COEFF": 0.05,
    "APD_MEMORY_COEFF": 0.0,
    "SIMULATION_DURATION": 3700,
    "VTK_OUTPUT_PERIOD": 1,
    "VTK_OUTPUT_INITIAL_TIME": 3000,
    "PROTOCOL": [
        {
            "ACTIVATION_REGION": 1,
            "FIRST_ACTIVATION_TIME": 0,
            "N_STIMS_PACING": [7,1],
            "BCL": [500,350] # BZ activates backwards: 375-380. Block: <375
        }
    ]
}

# --- STEP 4: Run the simulation ---
print("\n--- Running simulation ---")
a3d.arritmic3d(case_dir, config=config)
print("Simulation completed!")

plot_vtk(os.path.join(case_dir,"slab_03355.vtu"), title = "3355ms", plt_show=True)
plot_vtk(os.path.join(case_dir,"slab_03365.vtu"), title = "3365ms", plt_show=True)
plot_vtk(os.path.join(case_dir,"slab_03375.vtu"), title = "3375ms", plt_show=True)
plot_vtk(os.path.join(case_dir,"slab_03385.vtu"), title = "3385ms", plt_show=True)


# --- STEP 5: Build a larger slab ---

case_dir = "/home/ignacio/tmp/out_test/3.reentry_2"
delete_case_dir(case_dir)

# Ensure the subfolder for the slab exists
os.makedirs(os.path.join(case_dir, "input_data"), exist_ok=True)

slab_path = os.path.join(case_dir, "input_data", "slab.vtk")
slab_args = [
    slab_path,
    "--nnodes", "60", "60", "2",
    "--spacing", "0.3", "0.3", "0.3",
    "--region-by-side", "south", "1",
    "--field", "restitution_model", "2",
    "--region", '{"shape" : "square", "cx" : 9.0, "cy" : 9.0, "r1" : 6.0, "r2" : 6.0, "restitution_model" : 5}'
]

# Build and save the slab
a3d.build_slab(args_list=slab_args, save=True)
print("Slab generated successfully!")

# Visualize the slab
plot_vtk(slab_path, field="restitution_model", plt_show=True, title="Slab")

# --- Configure the simulation ---
print("\n--- Configuring simulation ---")
config = {
    "VTK_INPUT_FILE": slab_path,
    "APD_MODEL": "TenTuscher",
    "CV_MODEL": "TenTuscher",
    "ELECTROTONIC_EFFECT": 0.0,
    "CV_MEMORY_COEFF": 0.05,
    "CORRECTION_FACTOR_CV": 0.9,
    "APD_MEMORY_COEFF": 0.0,
    "SIMULATION_DURATION": 4800,
    "VTK_OUTPUT_PERIOD": 1,
    "VTK_OUTPUT_INITIAL_TIME": 3000,
    "PROTOCOL": [
        {
            "ACTIVATION_REGION": 1,
            "FIRST_ACTIVATION_TIME": 0,
            "N_STIMS_PACING": [8,2],
            "BCL": [420,320] # 319, block before reentry. 322 late for reentry
        }
    ]
}


# --- STEP 4: Run the simulation ---
print("\n--- Running simulation ---")
a3d.arritmic3d(case_dir, config=config)
print("Simulation completed!")

plot_vtk(os.path.join(case_dir,"slab_03800.vtu"), title = "03800ms", plt_show=True)
plot_vtk(os.path.join(case_dir,"slab_03900.vtu"), title = "03900ms", plt_show=True)
plot_vtk(os.path.join(case_dir,"slab_04000.vtu"), title = "04000ms", plt_show=True)
plot_vtk(os.path.join(case_dir,"slab_04100.vtu"), title = "04100ms", plt_show=True)

