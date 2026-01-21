# Arritmic3D
A fast Eikonal computational model for electrophysiology simulation.

The simulator has three versions in separate branches: branch `main` (this branch), with a development version, branch `ventricle`, with the ventricle version, and branch `atria`, with the atria version. Checkout the desired branch before proceeding.

The original version of the solver was developed in the Java environment Processing (https://processing.org/). Now it is being migrated to C++ and provided with a Python interface.

**Warning**: This development branch is not fully functional and has some differences in the diffusion model with respect to the other two branches. Thus, **this branch is not validated and should not be used for research purposes yet**.

This README covers
* [Installation](#installation)
* [Usage](#usage)
* [Publications](#publications)



## Installation
There is an install script for ubuntu machines. Review the script before executing it and use it at your own discretion.
For other distros, the inspection of the script will provide a list of requirements.

The install script will install the prerequisites for compilation, namely the C++ and Python build infrastructure.

For now, we recommend using the solver in a Python virtual environment. Once the build prerequisites are installed, you can run the install_venv.sh script, which will create a virtual env (based on `venv` module) and install the requirements. If you prefer building your own, the required packages are listed in the corresponding requirements.txt file. The solver is tested with Python 3.12.

## Usage

To run `arritmic3D.py`, you need:

- A VTK file containing a `RectilinearGrid` with at least the required **point data** field `restitution_model`.
- An output directory containing the configuration file (`arr3D_config.json`).

The program is invoked from the command line by passing the output directory as an argument:

```bash
python arritmic3D.py <output_directory>
```

The output directory must contain the configuration JSON file. The simulation will read the VTK file specified in the configuration, perform the computations, and save the results in the output directory.

### Input VTK file requirements

The input VTK file must be a `RectilinearGrid` and include the following point data field:
- **restitution_model**: An integer field indicating the restitution model to be used for each cell. This field is used to define the cells that belong to tissue and to select the restitution model for each cell. Nodes with `restitution_model` equal to 0 are considered non-tissue (void). The value for tissue regions starts from 1 upwards. The mapping between `restitution_model` values and the corresponding restitution model files is defined in a separate CSV file specified in the configuration, as described later.

Additional optional point data fields:
- **fibers_orientation**: A 3-component vector field representing the orientation of the fibers in each cell. To define isotropic conduction, set all components to zero. If this field is not present, isotropic conduction is assumed for all cells.
- **stimulation_sites**: A list of node IDs where stimulation is applied. This field is used to define the pacing sites for the simulation. If a node has a 0 value, it is not a pacing site; if it has a 1 value, it is a pacing site. Different values can be used to define groups of nodes that configure a pacing site (i.e. that share the same pacing protocol). If this field is not present, the pacing sites must be defined in the configuration file.


### Configuration parameters (`arr3D_config.json`)

The configuration file is a JSON file with the following parameters:

- **COND_VELOC_TRANSVERSAL_REDUCTION**: Reduction factor for transversal conduction velocity.
- **CORRECTION_FACTOR_APD**: Correction factor for action potential duration (APD).
- **CORRECTION_FACTOR_CV**: Correction factor for conduction velocity.
- **ELECTROTONIC_EFFECT**: Factor accounting for electrotonic effects in tissue.
- **INITIAL_APD**: Initial action potential duration (in ms).

- **VTK_OUTPUT_SAVE**: If `true`, saves VTK output files.
- **VTK_OUTPUT_PERIOD**: Time interval (in ms) between VTK outputs.
- **VTK_INPUT_FILE**: Path to the input VTK file.

- **SIMULATION_DURATION**: Total duration of the simulation (in ms).
- **PROTOCOL**: Stimulation protocol settings, using an S1,S2,...,Sn protocol. It is a list of dictionaries, each defining a pacing site with the following fields:
    - **STIMULATION_SITES**: List of node IDs where stimulation is applied. If it is not a list, but an integer $i$, it is considered to refer to the nodes that have value $i$ in the `stimulation_sites` field of the VTK file.
    - **N_STIMS_PACING**: List with the number of stimuli for each pacing site.
    - **BCL**: List of basic cycle lengths (BCL) for each pacing.
    - **FIRST_ACTIVATION_TIME**: Time (in ms) of the first activation for that pacing site. The first stimulus will be applied at this time, and subsequent stimuli will follow according to the BCL. Optional; if not provided, the first activation time is set to the first BCL.

- **ACTIVATE_NODES**: List of dictionaries defining specific activation times for nodes. Each dictionary has the following fields:
    - **STIMULATION_SITES**: List of node IDs or integer referring to the nodes that have that value in the `stimulation_sites` field of the VTK file.
    - **ACTIVATION_TIMES**: List of pairs `[time, beat]`, where `time` is the activation time in ms, and `beat` is the beat number to assign to that activation.


#### Stimulation protocol examples
```json
"PROTOCOL": {
    "STIMULATION_SITES": [100, 200, 300],
    "N_STIMS_PACING": [8, 2, 1, 1],
    "BCL": [600, 400, 300, 300]
}
```

This example defines a protocol with pacing site formed by nodes 100, 200, and 300. The protocol consists of 8 stimuli at a BCL of 600 ms, followed by 2 stimuli at a BCL of 400 ms, then 1 stimulus at 400 ms, and finally 1 stimulus at 300 ms.

"N_STIMS_PACING" and "BCL" lists do not need to have the same length. If they differ, the last value of the shorter list is repeated until both lists have the same length. For instance, the following is equivalent to the previous example:

```json
"PROTOCOL": {
    "STIMULATION_SITES": [100, 200, 300],
    "N_STIMS_PACING": [8, 2, 1],
    "BCL": [600, 400, 300, 300]
}
```

#### Activation by node id + time example

    ```json
            "ACTIVATE_NODES" : [
                {
                    "STIMULATION_SITES" : [1,2,3],
                    "ACTIVATION_TIMES" : [[500,1],[550,2]]
                },
                {
                    "STIMULATION_SITES" : 1,
                    "ACTIVATION_TIMES" : [[1500,4],[1550,5]]
                }
            ]
    ```
This example defines specific activation times for nodes 1, 2, and 3 at 500 ms (beat 1) and 550 ms (beat 2), and for nodes with `stimulation_sites` value 1 at 1500 ms (beat 4) and 1550 ms (beat 5).


### Generating an example slab

You can generate a sample rectilinear grid (slab) for testing using the `build_slab.py` utility. This script creates a VTK file with the required fields for simulation.

For example, to generate a slab with 20 divisions in X, 20 in Y, and 5 in Z, with spacing of 0.05 mm, run:

```bash
python build_slab.py cases/slab.vtk --ndivs 20 20 5 --spacing 0.05 0.05 0.05
```

This will create a VTK file at `cases/slab.vtk` with the necessary structure and a point data field named `restitution_model` with a value of 1.
You can customize the grid size, spacing, and additional options using the script arguments.

To generate pacing labels for your slab, use the `--generate_stim_sites` option when running `build_slab.py`. For instance:

```bash
python build_slab.py cases/slab.vtk --ndivs 20 20 5 --spacing 0.05 0.05 0.05 --generate_stim_sites
```

This will add the 'stimulation_sites' point data field to the generated VTK file.

For more advanced configurations, see the help message:

```bash
python build_slab.py --help
```

## Restitution models

The restitution models can be defined by means of restitution curves or surfaces, as described in (Serra et al., 2022; Romitti et al., 2025). A restitution curve is a function that provides the next Action Potential Duration (APD) of a cell as a function of the Diastolic Interval (DI) at activation. A restitution surface is a function that provides the next APD of a cell as a function of its last APD and the DI at activation.

The simulator needs a restitution model for each tissue type (representing, e.g. tissue region, as in endo/mid/epi, or type, such as healthy/border zone). The restitution models (curves or surfaces) are encoded as a table in a CSV file. Another CSV file must indicate the tissue region-type that is modeled in each table. Actually, all the models are stared as surfaces, even if they are curves. In that case, the table will have a single row for each previous APD value, with an ourput APD value for each DI.

The following CSV corresponds to a restitution model
``` csv
0.0  , 30.0 , 35.0 , 40.0 , 45.0 , 50.0
95.5 , -1.0 , 89.44, 89.67, 89.90, 90.13
99.5 , 89.64, 89.87, 90.10, 90.32, 90.52
103.5, 90.73, 90.95, 91.17, 91.38, 91.59
107.5, 91.67, 91.88, 92.09, 92.30, 92.50
```

For the restitution model CSV table shown above, the first row contains the Diastolic Interval (DI) values measured in milliseconds. The first column lists the previous Action Potential Duration (APD) values, also in milliseconds. The remaining cells in the table contain the computed next APD values in milliseconds. Note the use of -1.0 to indicate that no activation occurs for that combination of previous APD and DI.

To specify which tissue regions use which restitution models, a separate CSV file maps the region types to table indices. For example:
``` csv
1,TenTuscher_APD_BZ_Mid.csv
2,TenTuscher_APD_BZ_Epi.csv
3,TenTuscher_APD_Healthy_Mid.csv
```

As discussed, index '0' is reserved for non-tissue regions and cannot be used.

Dead (Core Zone) tissue can be modeled by setting the restitution model index to 0. But, if desired, it can also be mapped to a table that always returns -1.0 for any input values.
``` csv
0.0  , 30.0
100  , -1.0
```
The actual values for DI and previous APD are not relevant in this case, as no activation will ever occur.

### Mapping tissue region-types to restitution model files

The field `restitution_model` in the input VTK file indicates the tissue region-type for each cell. The value of this field is used to select the restitution model for each cell according to the mapping provided in the second CSV file.

Important considerations for the restitution table format:
The table must include the minimum DI value that allows cell activation, along with the preceding DI value. For the preceding value, use -1 to indicate no activation occurs. That is, in every row, at least the first column must contain -1 for the DI value that precedes the minimum activation DI value. This ensures that the model can correctly identify the threshold for activation.

For APD or DI input values that fall outside the table's defined ranges, the model applies flat extrapolation to determine the output values.

## Advanced (mixed) restitution models

The simulator also supports mixed restitution models, where a cell can use a convex combination of two or more restitution models. This is useful for modeling heterogeneous tissue properties.

The mixture of models is defined as an additional data field in the input VTK file, `model_weights`. Each cell contains weights for each restitution model, indicating the contribution of each model to the final APD calculation. The weights must sum to 1.0 for each cell. If a cell will be using mixed models, the `restitution_model` field must be set to the total number of models plus one. For example, if there are 3 restitution models defined in the mapping CSV file, a cell using a mixture of these models will have its `restitution_model` set to 4.

Note that the use of mixed models can pose a performance penalty, as multiple model evaluations are required for each cell activation. For this reason, cells that do not use mixed models should avoid the overhead by using a specific restitution model directly. This will also help reduce memory consumption, as the `model_weights` field is not stored for those cells. Single-model and mixed-model cells can coexist in the same simulation.

## Publications
Please, cite as:
* Serra, D., Romero, P., Garcia-Fernandez, I., Lozano, M., Liberos, A., Rodrigo, M., ... & Sebastian, R. (2022). An automata-based cardiac electrophysiology simulator to assess arrhythmia inducibility. Mathematics, 10(8), 1293. https://doi.org/10.3390/math10081293

Related research:
* Serra, D., Romero, P., Franco, P., Bernat, I., Lozano, M., Garcia-Fernandez, I., ... & Sebastian, R. (2025). Unsupervised stratification of patients with myocardial infarction based on imaging and in-silico biomarkers. IEEE Transactions on Medical Imaging. https://doi.org/10.1109/tmi.2025.3582383
* Romitti, G. S., Liberos, A., Termenón-Rivas, M., de Arcaya, J. B. Á., Serra, D., Romero, P., ... & Rodrigo, M. (2025). Implementation of a Cellular Automaton for efficient simulations of atrial arrhythmias. Medical Image Analysis, 103484. https://doi.org/10.1016/j.media.2025.103484
* Serra, D., Romero, P., Lozano, M., Garcia-Fernandez, I., Penela, D., Berruezo, A., ... & Sebastian, R. (2023, October). Patient Stratification Based on Fast Simulation of Cardiac Electrophysiology on Digital Twins. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 35-43). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-52448-6_4
* Serra, D., Franco, P., Romero, P., Romitti, G., García-Fernández, I., Lozano, M., ... & Sebastian, R. (2023, July). Assessment of Risk for Ventricular Tachycardia based on Extensive Electrophysiology Simulations. In 2023 45th Annual International Conference of the IEEE Engineering in Medicine & Biology Society (EMBC) (pp. 1-4). IEEE. https://doi.org/10.1109/EMBC40787.2023.10340169
* Serra, D., Franco, P., Romero, P., García-Fernández, I., Lozano, M., Soto, D., ... & Sebastian, R. (2022, September). Personalized Fast Electrophysiology Simulations to Evaluate Arrhythmogenicity of Ventricular Slow Conduction Channels. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 56-64). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-23443-9_6