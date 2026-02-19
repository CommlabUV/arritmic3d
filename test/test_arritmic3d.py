import os
import shutil
import pyvista as pv
import matplotlib.pyplot as plt
import arritmic3d as a3d

def plot_grid(grid,field="AP",plt_show=False) :
    # Setup plotter for a static image
    plotter = pv.Plotter(off_screen=True)
    plotter.add_mesh(grid, scalars=field, cmap="coolwarm", show_edges=True)
    plotter.view_xy()
    img = plotter.show(screenshot=True)
    # Display using matplotlib
    plt.imshow(img)
    plt.axis('off')
    if plt_show:
        plt.show()
    return img

def plot_vtk(file_path, field="AP",plt_show=False):
    grid = pv.read(file_path)
    grid = grid.threshold(0.5, scalars="restitution_model", all_scalars=True)
    plot_grid(grid,field=field,plt_show=plt_show)

def delete_case_dir(case_dir):
    """Delete the case directory if it exists."""
    if os.path.exists(case_dir):
        print(f"Cleaning existing directory: {case_dir}")
        shutil.rmtree(case_dir)

# --- STEP 1: Configure the case directory ---
case_dir = "out_test"
delete_case_dir(case_dir)

# --- STEP 2: Running initial test case ---
print("\n--- Running initial test case ---")
a3d.test_case(case_dir)
print("Simulation completed!")

# --- STEP 3: Visualize a single result ---
print("\n--- Visualizing a frame at 715ms ---")

# Load the grid (later, we will use a function to do this)
grid = pv.read("out_test/slab_00715.vtk")

# Filter out regions where restitution_model is 0
# all_scalars=True ensures only cells where ALL points are > 0.5 are kept
grid = grid.threshold(0.5, scalars="restitution_model", all_scalars=True)

# Plot the action potential
plot_grid(grid,"AP",plt_show=True)

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
delete_case_dir(case_dir)
a3d.arritmic3d(case_dir,config=config)
print("Simulation finished.")

plot_vtk(case_dir+"/slab_00720.vtk",plt_show=True)