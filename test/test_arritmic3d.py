import os
import shutil
import pyvista as pv
import matplotlib.pyplot as plt
from IPython.display import clear_output
import time
import arritmic3d as a3d

ranges = {
    "AP": [-80, 40],
    "State": [0, 2]
}

def plot_grid(grid,field="AP",plt_show=False,title = "") :
    rng = ranges.get(field,None)
    # Apply a threshold to the grid. Cells with restitution_model==0
    # are not part of the simulation domain
    grid = grid.threshold(0.5, scalars="restitution_model", all_scalars=True)
    # Setup plotter for a static image
    plotter = pv.Plotter(off_screen=True)
    plotter.add_mesh(grid, scalars=field, cmap="coolwarm", show_edges=True,rng=rng)
    plotter.view_xy()
    img = plotter.show(screenshot=True)
    # Display using matplotlib
    plt.imshow(img)
    plt.axis('off')
    if title != "":
        plt.title(title)
    if plt_show:
        plt.show()
    return img

def plot_vtk(file_path, field="AP",plt_show=False,title = ""):
    grid = pv.read(file_path)
    plot_grid(grid,field=field,plt_show=plt_show,title=title)

def plot_animation(case_dir, field="AP", init_time=None, end_time=None, step=None):
    config = a3d.load_case_config(case_dir)
    if init_time is None:
        init_time = config.get("VTK_OUTPUT_PERIOD")
    if step is None:
        step = config.get("VTK_OUTPUT_PERIOD")
    if end_time is None:
        end_time = config.get("SIMULATION_DURATION")

    for time_ms in range(init_time, end_time + 1, step):
        file_path = f"{case_dir}/slab_{time_ms:05d}.vtk"
        if os.path.exists(file_path):
            plot_vtk(file_path, field=field, plt_show=True, title=f"Time: {time_ms} ms")
            # Small pause to allow the UI to update
            time.sleep(0.01)

def delete_case_dir(case_dir):
    """Delete the case directory if it exists."""
    if os.path.exists(case_dir):
        print(f"Cleaning existing directory: {case_dir}")
        shutil.rmtree(case_dir)

def clean_case_dir(case_dir):
    """Clean the case directory by removing only the vtk output files."""
    if os.path.exists(case_dir):
        for file in os.listdir(case_dir):
            if file.endswith(".vtk"):
                os.remove(os.path.join(case_dir, file))


# --- STEP 1: Configure the case directory ---
case_dir = "out_test"
delete_case_dir(case_dir)

# --- STEP 2: Running initial test case ---
print("\n--- Running initial test case ---")
a3d.test_case(case_dir)
print("Simulation completed!")

# --- STEP 3: Visualize a single result ---
print("\n--- Visualizing a frame at 715ms ---")
plot_vtk("out_test/slab_00715.vtk",plt_show=True, title="t=715ms")

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
