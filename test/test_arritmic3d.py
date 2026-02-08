import arritmic
import pyvista as pv
import numpy as np
import os
import shutil

"""
This script demonstrates the basic usage of Arritmic3D.
It is designed for newcomers to Python and cardiac simulation.

Steps performed:
1. Prepares a directory to save the results.
2. Creates and runs a test simulation (a tissue slab with stimulation).
3. Loads the results and displays them in an interactive window.
4. Shows how to re-run a case using its existing configuration file.
"""

from arritmic import load_case_config

def visualize_case(case_dir, time_ms, field = "State"):
    """
    Loads results from a simulation case and displays them in a 2D window.

    Arguments:
    - case_dir: The directory where the simulation was run.
    - time_ms: The time point (in milliseconds) of the simulation to visualize.
    """
    print(f"Loading results from {case_dir} at {time_ms} ms...")

    # Load the input tissue (the "heart" slab)
    path_input = os.path.join(case_dir, "input_data", "slab.vtk")
    if not os.path.exists(path_input):
        print(f"Error: Input file {path_input} not found.")
        return
    grid_input = pv.read(path_input)

    # Load the output file for the requested time
    path_output = os.path.join(case_dir, f"slab_{time_ms:05d}.vtk")
    if not os.path.exists(path_output):
        print(f"Error: Output file {path_output} not found.")
        return
    grid_output = pv.read(path_output)

    # --- Pre-processing: Remove non-tissue nodes (restitution_model = 0) ---
    # We apply this threshold so that non-tissue padding is never shown.
    # We use all_scalars=True to ensure only cells entirely within the tissue are kept.
    tissue_grid = grid_input.threshold(0.5, scalars="restitution_model", all_scalars=True)
    activation_grid = grid_output.threshold(0.5, scalars="restitution_model", all_scalars=True)

    # Create the visualization window using PyVista
    # We show a 2x2 grid with all key data fields
    plotter = pv.Plotter(shape=(2, 2), title=f"Arritmic3D Results - {time_ms} ms")

    # --- TOP-LEFT: Tissue Regions ---
    plotter.subplot(0, 0)
    plotter.add_text("Restitution Model (Tissue)", font_size=10)
    # Mapping discrete IDs: we use clim and disable interpolation for sharp transitions
    vals = tissue_grid["restitution_model"]
    plotter.add_mesh(tissue_grid, scalars="restitution_model", cmap="coolwarm",
                     show_edges=True, categories=True)
    plotter.view_xy()

    # --- TOP-RIGHT: Activation Regions (Stimulation Sites) ---
    plotter.subplot(0, 1)
    plotter.add_text("Activation Region (Pacing Sites)", font_size=10)
    # Use categories=True for stimulation region IDs
    vals_act = tissue_grid["activation_region"]
    plotter.add_mesh(tissue_grid, scalars="activation_region", cmap="coolwarm",
                     show_edges=True, categories=True)
    plotter.view_xy()

    # --- BOTTOM-LEFT: Fiber Orientation ---
    plotter.subplot(1, 0)
    plotter.add_text("Fibers Orientation", font_size=10)

    # Check if we have fibers (some components are non-zero)
    fibers = tissue_grid['fibers_orientation']
    is_isotropic = np.all(fibers == 0)

    if is_isotropic:
        # If isotropic, just show the tissue mesh and a message
        plotter.add_mesh(tissue_grid, color="lightgray", show_edges=True)
        plotter.add_text("Isotropic (All fibers are zero)", font_size=9, color="blue", position="lower_left")
    else:
        # If fibers exist, show them as small arrows (glyphs)
        # We use a subset of points for the arrows so it's not too crowded
        sampled = tissue_grid.glyph(orient="fibers_orientation", scale=False, factor=0.04)
        plotter.add_mesh(tissue_grid, color="lightgray", opacity=0.3)
        plotter.add_mesh(sampled, cmap="coolwarm")
    plotter.view_xy()

    # --- BOTTOM-RIGHT: Simulation Field ---
    plotter.subplot(1, 1)
    plotter.add_text(f"Simulation Field {field} at {time_ms} ms", font_size=10)
    plotter.add_mesh(activation_grid, scalars=field, cmap="coolwarm", show_edges=True, categories=True)
    plotter.view_xy()

    print("Opening visualization window...")
    plotter.show()


# --- STEP 1: Case Directory Setup ---
# Name of the directory where simulation results will be saved
case_dir = "test_arritmic3d"

# If the directory already exists, we delete it to avoid errors
if os.path.exists(case_dir):
    print(f"Cleaning existing directory: {case_dir}")
    shutil.rmtree(case_dir)

# --- STEP 2: Run the Simulation ---
# The 'test_case' function handles the setup and execution:
# - Creates a tissue slab of 20x20x5 nodes.
# - Applies an S1-S2 stimulation protocol (electric pulses).
# - Runs the simulation for 3500 milliseconds.
print("Starting simulation... This may take a few seconds.")
arritmic.test_case(case_dir)
print("Simulation completed successfully!")

# --- STEP 3: Visualization of Results ---
# We use our new function to visualize the result at the end of the simulation (3500 ms)
visualize_case(case_dir, 104)

# --- STEP 4: Re-run Case ---
# Arritmic3D is designed so that you can easily re-run a simulation
# using the configuration file saved in the case directory.
print("\n--- STEP 4: Re-running the case ---")
arritmic.arritmic3d(case_dir)
print("Re-run completed!")


# --- STEP 5: Load and change the configuration file generated in STEP 2
print(f"Loading existing configuration from: {case_dir}")
config = load_case_config(case_dir)
print("Configuration loaded successfully!")


