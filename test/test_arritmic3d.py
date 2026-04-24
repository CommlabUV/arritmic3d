import arritmic3d as a3d
from plot_functions import (plot_grid, plot_vtk, plot_animation, delete_case_dir, clean_case_dir)

# --- STEP 1: Configure the case directory ---
case_dir = "out_test/1.arritmic3d"
delete_case_dir(case_dir)

# --- STEP 2: Running initial test case ---
print("\n--- Running initial test case ---")
a3d.test_case(case_dir)
print("Simulation completed!")

# --- STEP 3: Visualize a single result ---
print("\n--- Visualizing a frame at 715ms ---")
plot_vtk(case_dir + "/slab_00715.vtk",plt_show=True, title="t=715ms")

# --- STEP 4: Re-run Case ---
# Arritmic3D is designed so that you can easily re-run a simulation
# using the configuration file saved in the case directory.
print("\n--- STEP 4: Re-running the case ---")
a3d.arritmic3d(case_dir)
print("Re-run completed!")

# --- STEP 5: Load and change the configuration file generated in STEP 2
config = a3d.load_case_config(case_dir)
config["SIMULATION_DURATION"] = 1000
config["VTK_OUTPUT_PERIOD"] = 10

# Re-run with the updated configuration
# First, clean the case directory, because the file numbering will be different
clean_case_dir(case_dir)
a3d.arritmic3d(case_dir,config=config)
print("Simulation finished.")

plot_vtk(case_dir+"/slab_00720.vtk",plt_show=True, title="t=720ms")

# --- STEP 6: Show an animation of the simulation
print("\n--- Showing an animation of the simulation ---")
plot_animation(case_dir, field="AP")
