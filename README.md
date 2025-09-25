# Arritmic3D
A fast Eikonal computational model for electrophysiology simulation (Ventricle version).

The simulator has three versions in separate branches: branch `main` (this branch), with a development version, branch `ventricle`, with the ventricle version, and branch `atria`, with the atria version. Checkout the desired branch before proceeding.

The original version of the solver was developed in the Java environment Processing (https://processing.org/). Now it is being migrated to C++ and provided with a Python interface.

**Warning**: This development branch is not fully functional and has some differences in the diffusion model with respect to the other two branches. Thus, **this branch is not validated and should not be used for research purposes yet**.

This README covers
* [Installation](#installation)
* [Usage](#usage)
* [Publications](#publications)



## Installation
There is an istall script for ubuntu machines. Right now, it has two versions, one for users with Python 3.11 and the other for users with Python 3.12. Review the install script before executing it and use it at your own discretion.
For other distros, the inspection of the script will provide a list of requirements.

The install script will install the prerequisites for compilation, namely the C++ and Python build infrastructure.

For now, we recommend using the solver in a Python virtual environment. Once the build prerequisites are installed, you can run the install_venv.sh script, which will create a virtual env (based on `venv` module) and install the requirements. If you prefer building your own, the required packages are listed in the corresponding requirements.txt file.

## Usage

To run `arritmic3D.py`, you need:

- A VTK file containing a `RectilinearGrid` with at least the required point data fields: `Cell_type`, `EndoToEpi`, and `fibers_OR`.
- An output directory containing the configuration file (`arr3D_config.json`).

The program is invoked from the command line by passing the output directory as an argument:

```bash
python arritmic3D.py <output_directory>
```

The output directory must contain the configuration JSON file. The simulation will read the VTK file specified in the configuration, perform the computations, and save the results in the output directory.

### Configuration parameters (`arr3D_config.json`)

- **COND_VELOC_TRANSVERSAL_REDUCTION**: Reduction factor for transversal conduction velocity.
- **CORRECTION_FACTOR_APD**: Correction factor for action potential duration (APD).
- **CORRECTION_FACTOR_CV_BORDER_ZONE**: Correction factor for conduction velocity in border zones.
- **ELECTROTONIC_EFFECT**: Factor accounting for electrotonic effects in tissue.
- **INITIAL_APD**: Initial action potential duration (in ms).

- **VTK_OUTPUT_SAVE**: If `true`, saves VTK output files.
- **VTK_OUTPUT_PERIOD**: Time interval (in ms) between VTK outputs.
- **VTK_INPUT_FILE**: Path to the input VTK file.

- **SIMULATION_DURATION**: Total duration of the simulation (in ms).
- **PROTOCOL**: Stimulation protocol settings:
    - **INITIAL_NODE_ID**: List of node IDs where stimulation is applied.
    - **N_STIMS_PACING**: List with the number of stimuli for each pacing site.
    - **BCL**: List of basic cycle lengths (BCL) for each pacing

### Generating an example slab

You can generate a sample rectilinear grid (slab) for testing using the `build_slab.py` utility. This script creates a VTK file with the required fields for simulation.

For example, to generate a slab with 20 divisions in X, 20 in Y, and 5 in Z, with default spacing of 1.0 mm, run:

```bash
python build_slab.py cases/slab.vtk --nx 20 --ny 20 --nz 5
```

This will create a VTK file at `cases/slab.vtk` with the necessary structure and point data fields (`Cell_type`, `EndoToEpi`, `fibers_OR`).
You can customize the grid size, spacing, and additional options using the script arguments.

To generate a JSON file with the list of pacing (stimulus) node IDs for your slab, use the `--generate_stim_sites` option when running `build_slab.py`. For example:

```bash
python build_slab.py cases/slab.vtk --nx 20 --ny 20 --nz 5 --generate_stim_sites
```

This will create a file named `cases/slab_pacing_sites.json` containing the node IDs for stimulation, which can be used

For more advanced configurations, see the help message:

```bash
python build_slab.py --help
```


## Publications
Please, cite as:
* Serra, D., Romero, P., Garcia-Fernandez, I., Lozano, M., Liberos, A., Rodrigo, M., ... & Sebastian, R. (2022). An automata-based cardiac electrophysiology simulator to assess arrhythmia inducibility. Mathematics, 10(8), 1293. https://doi.org/10.3390/math10081293

Related research:
* Serra, D., Romero, P., Franco, P., Bernat, I., Lozano, M., Garcia-Fernandez, I., ... & Sebastian, R. (2025). Unsupervised stratification of patients with myocardial infarction based on imaging and in-silico biomarkers. IEEE Transactions on Medical Imaging. https://doi.org/10.1109/tmi.2025.3582383
* Romitti, G. S., Liberos, A., Termenón-Rivas, M., de Arcaya, J. B. Á., Serra, D., Romero, P., ... & Rodrigo, M. (2025). Implementation of a Cellular Automaton for efficient simulations of atrial arrhythmias. Medical Image Analysis, 103484. https://doi.org/10.1016/j.media.2025.103484
* Serra, D., Romero, P., Lozano, M., Garcia-Fernandez, I., Penela, D., Berruezo, A., ... & Sebastian, R. (2023, October). Patient Stratification Based on Fast Simulation of Cardiac Electrophysiology on Digital Twins. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 35-43). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-52448-6_4
* Serra, D., Franco, P., Romero, P., Romitti, G., García-Fernández, I., Lozano, M., ... & Sebastian, R. (2023, July). Assessment of Risk for Ventricular Tachycardia based on Extensive Electrophysiology Simulations. In 2023 45th Annual International Conference of the IEEE Engineering in Medicine & Biology Society (EMBC) (pp. 1-4). IEEE. https://doi.org/10.1109/EMBC40787.2023.10340169
* Serra, D., Franco, P., Romero, P., García-Fernández, I., Lozano, M., Soto, D., ... & Sebastian, R. (2022, September). Personalized Fast Electrophysiology Simulations to Evaluate Arrhythmogenicity of Ventricular Slow Conduction Channels. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 56-64). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-23443-9_6