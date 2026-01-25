import os
import sys
import json
import shutil
import subprocess
import filecmp
from pathlib import Path

import pyvista as pv
import numpy as np

OUTDIR = Path("/tmp/arr3d_test")
REPO_ROOT = Path(__file__).resolve().parents[1]  # .../arritmic3D
BUILD_SCRIPT = REPO_ROOT / "build_slab.py"

EXAMPLES = [
    # 0: basic slab
    {
        "name": "basic",
        "args": [
            "--nnodes", "10", "10", "5",
            "--spacing", "0.05", "0.05", "0.05",
            "--field", "restitution_model", "1"
        ]
    },
    # 1: regions-file example (we'll create a small regions file)
    {
        "name": "regions_file",
        "args": [
            "--nnodes", "20", "20", "5",
            "--spacing", "0.05", "0.05", "0.05",
            "--regions-file", "REGIONS_FILE_PLACEHOLDER"
        ],
        "regions_file_content": [
            { "shape":"circle", "cx":0.5, "cy":0.5, "r1":0.2, "r2":0.4, "restitution_model":[2, 3] },
            { "shape":"square", "cx":1.0, "cy":1.0, "r1":0.15, "r2":0.3, "restitution_model":[4, 5] }
        ]
    },
    # 2: multiple regions via CLI
    {
        "name": "multiple_cli",
        "args": [
            "--nnodes", "20", "20", "5",
            "--spacing", "0.05", "0.05", "0.05",
            "--region", '{"shape":"square","cx":0.5,"cy":0.5,"r1":0.05,"r2":0.1,"restitution_model":7}',
            "--region", '{"shape":"circle","cx":1.0,"cy":1.0,"r1":0.3,"r2":0.6,"restitution_model":[6,3]}'
        ]
    },
    # 3: combined file + CLI (create base file and pass extra region)
    {
        "name": "file_plus_cli",
        "args": [
            "--nnodes", "20", "20", "5",
            "--spacing", "0.05", "0.05", "0.05",
            "--regions-file", "REGIONS_BASE_PLACEHOLDER",
            "--region", '{"shape":"circle","cx":1.0,"cy":1.0,"r1":0.5,"r2":0.8,"restitution_model":[9,4]}'
        ],
        "regions_base_content": [
            { "shape":"square","cx":0.5,"cy":0.5,"r1":0.05,"r2":0.1,"restitution_model":7 }
        ]
    }
]

EXPLANATIONS = {
	"basic": "Basic slab with default restitution_model across tissue and default fiber orientation.",
	"regions_file": "Regions from a JSON file: demonstrates circle + square region assignments and gradients.",
	"multiple_cli": "Multiple regions passed via CLI: demonstrates multiple --region entries and precedence.",
	"file_plus_cli": "Combination of regions-file plus CLI regions: CLI overrides file where overlapping."
}

def ensure_outdir():
    if OUTDIR.exists():
        shutil.rmtree(OUTDIR)
    OUTDIR.mkdir(parents=True, exist_ok=True)

def run_cli(output_path, args):
    cmd = [sys.executable, str(BUILD_SCRIPT), str(output_path)] + args
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"CLI run failed:\nCMD: {' '.join(cmd)}\nSTDOUT: {proc.stdout}\nSTDERR: {proc.stderr}")
    return proc

def run_import(output_path, args):
    # import build_slab module by path; ensure repo root on sys.path
    sys.path.insert(0, str(REPO_ROOT))
    import importlib
    mod = importlib.import_module("build_slab")
    # prepare argv as if called from CLI: program, output_path, + args
    old_argv = sys.argv[:]
    sys.argv = [str(BUILD_SCRIPT), str(output_path)] + args
    try:
        mod.main()
    finally:
        sys.argv = old_argv
        # remove inserted path to avoid side-effects
        if sys.path[0] == str(REPO_ROOT):
            sys.path.pop(0)

def compare_files(a, b):
    return filecmp.cmp(a, b, shallow=False)

def visualize(vtk_path, png_prefix):
    grid = pv.read(str(vtk_path))

    # try to get a mid-z value for a 2D slice; fallback to plotting whole grid
    try:
        z_coords = np.asarray(grid.z)
        if z_coords.size > 0:
            mid_z = float(z_coords[len(z_coords)//2])
            slice2d = grid.slice(origin=(0.0, 0.0, mid_z), normal=(0, 0, 1))
            use_slice = slice2d.n_points > 0
        else:
            use_slice = False
    except Exception:
        use_slice = False

    # helper to write screenshot
    def _screenshot(mesh, fname, categorical=False, cmap=None):
        p = pv.Plotter(off_screen=True, window_size=(900,700))
        # PyVista does not currently accept the 'categorical' kwarg in add_mesh.
        # Use an appropriate colormap and let add_mesh handle coloring; for discrete values
        # prefer "tab20" by default.
        cmap_use = (cmap or ("tab20" if categorical else "viridis"))
        p.add_mesh(mesh, scalars=mesh.active_scalars_name, show_edges=True, cmap=cmap_use)
        p.add_scalar_bar(title=mesh.active_scalars_name)
        p.camera_position = 'xy'
        p.screenshot(fname)
        p.close()

    # Show restitution_model (categorical) on the slice if possible
    if "restitution_model" in grid.point_data:
        if use_slice:
            # ensure integer values for categorical plotting
            rm = np.asarray(slice2d.point_data["restitution_model"]).astype(int)
            slice2d.point_data["restitution_model"] = rm
            slice2d.active_scalars_name = "restitution_model"
            _screenshot(slice2d, str(png_prefix) + "_restitution.png", categorical=True, cmap="tab20")
        else:
            # fallback: use full grid (cast to int for display only)
            rm_full = np.asarray(grid.point_data["restitution_model"]).astype(int)
            grid.point_data["restitution_model"] = rm_full
            grid.active_scalars_name = "restitution_model"
            _screenshot(grid, str(png_prefix) + "_restitution.png", categorical=True, cmap="tab20")

    # Show fibers magnitude on the slice if present
    if "fibers_orientation" in grid.point_data:
        # compute magnitude on slice if using it, else on full grid
        target = slice2d if use_slice else grid
        fo = target.point_data.get("fibers_orientation")
        if fo is None:
            # if slice doesn't contain the field, try projecting from full grid
            if not use_slice:
                fo = grid.point_data["fibers_orientation"]
            else:
                fo = None
        if fo is not None:
            mag = (np.asarray(fo)**2).sum(axis=1)**0.5
            target.point_data["fibers_mag"] = mag
            target.active_scalars_name = "fibers_mag"
            _screenshot(target, str(png_prefix) + "_fibers_mag.png", categorical=False, cmap="coolwarm")

def prepare_regions_file(content, filename):
    path = OUTDIR / filename
    with open(path, "w") as fh:
        json.dump(content, fh, indent=2)
    return str(path)

def run_case(case):
    name = case["name"]
    print(f"Running case: {name}")
    cli_out = OUTDIR / f"{name}_cli.vtk"
    imp_out = OUTDIR / f"{name}_imp.vtk"

    args = list(case["args"])  # copy

    # handle placeholders for region files
    if "regions_file_content" in case:
        rf = prepare_regions_file(case["regions_file_content"], f"{name}_regions.json")
        for i, a in enumerate(args):
            if a == "REGIONS_FILE_PLACEHOLDER":
                args[i] = rf
    if "regions_base_content" in case:
        rb = prepare_regions_file(case["regions_base_content"], f"{name}_regions_base.json")
        for i, a in enumerate(args):
            if a == "REGIONS_BASE_PLACEHOLDER":
                args[i] = rb

    # run CLI
    run_cli(cli_out, args)
    # run import-based
    run_import(imp_out, args)

    # compare
    same = compare_files(cli_out, imp_out)
    if not same:
        print(f"[FAIL] Outputs differ for case {name}: {cli_out} vs {imp_out}")
        return False

    # visualize one of them
    png_prefix = OUTDIR / f"{name}"
    visualize(cli_out, png_prefix)

    # print explanation and generated PNG paths
    expl = EXPLANATIONS.get(name, "")
    print(f"Example '{name}': {expl}")
    pngs = sorted([str(p) for p in OUTDIR.glob(f"{name}_*.png")])
    if pngs:
        print("Generated images:")
        for p in pngs:
            print("  -", p)
    else:
        print("No images generated for this case.")

    print(f"[OK] Case {name} passed. PNGs: {png_prefix}_*.png")
    return True

def main():
    ensure_outdir()
    all_ok = True
    for case in EXAMPLES:
        ok = run_case(case)
        all_ok = all_ok and ok

    if not all_ok:
        print("Some cases failed. Inspect /tmp/arr3d_test/")
        sys.exit(2)
    print("All examples matched. Results in /tmp/arr3d_test/")
    sys.exit(0)

if __name__ == "__main__":
    main()
