---
title : "Arritmic3D: A fast Eikonal computational model for electrophysiology simulation"
author : "CoMMLab"
date : "January 2026"
tags : ["electrophysiology", "eikonal", "simulation", "cardiac modeling", "computational model"]
header-includes:
  - \usepackage[a4paper,margin=1.8cm]{geometry}

---


Arritmic3D is a fast Eikonal computational model for electrophysiology simulation.

The simulator has three versions in separate branches: branch `main` (this branch), with a development version, branch `ventricle`, with the ventricle version, and branch `atria`, with the atria version. Checkout the desired branch before proceeding.

**Warning**: This development branch is not fully functional and has some differences in the diffusion model with respect to the other two branches. Thus, **this branch is not validated and should not be used for research purposes yet**.

The original version of the solver was developed in the Java environment Processing (https://processing.org/). Now it is being migrated to C++ and provided with a Python interface.

This document covers the following topics:

* [Installation](#installation)
* [An overview of Arritmic3D](#an-overview-of-arritmic3d)
* [Quick start example](#quick-start-example)
* [Simulation output](#simulation-output)
* [Runing simulations](#runing-simulations)
* [The configuration file](#the-configuration-file)
* [Definition of restitution models](#definition-of-restitution-models)
* [Generating tissue slabs](#generating-tissue-slabs)
* [Publications](#publications)



# Installation

There is an install script for Ubuntu machines. Review the script before executing it, and use it at your own discretion.
For other distros, the inspection of the script will provide a list of requirements.

The install script will install the prerequisites for compilation, namely the C++ and Python build infrastructure.

For now, we recommend using the solver in a Python virtual environment. Once the build prerequisites are installed, you can run the `install_venv.sh` script, which will create a virtual env (based on the `venv` module) and will install the Python requirements. If you prefer building your own environment, the required packages are listed in the corresponding `requirements.txt` file. The solver is tested with Python 3.12.

# An overview of Arritmic3D

The solver is presented as a C++ library with a Python interface. The main script to run simulations is `arritmic3D.py`. Typically, you will need a VTK file with the definition of the tissue to simulate and a configuration file.

The simulator expects a rectangular 3-dimensional domain, defined by a uniform grid. Each node in this grid is taken as a portion of cardiac tissue with its activation and conduction properties. These properties, which define the dynamics of electrophysiology of the tissue, are determined by a set of restitution models, that describe how the tissue behaves after a depolarization.

To use complex geometric domains, such as anatomical models of the heart, the rectangular domain can include void regions that do not belong to the tissue. These regions are defined by assigning a special restitution model index (0) to the corresponding nodes.

The simulator relies on a catalog of restitution models and each node is assigned a model from this catalog. Arritmic3D comes with two models (Ten Tuscher and TorOrd), but new models can be added as discussed below.
The simulator requires a scalar field defined on the simulation domain that establishes which model has to be used in each node of the domain.

Cardiac tissue is known for having an anisotropic behavior. Fiber orientation can also be set per node, indicatig the direction along which activation propagates fastest. This parameter is optional, and isotropic behavior is simulated in its absence.

Finally, the simulation requires a series of external activations at certain tissue locations to start the propagation. In order to ease this task, activation regions can be defined, along with different stimulation protocols to be applied on a particular region.

A detailed description of the configuration parameters and input requirements is provided later. However, first we present a brief overview of how to run some basic examples.




# Quick start example

The python script allows the execution of a simulation on a rectilinear grid (slab) with an S1-S2 protocol. You can just run the following command:

```bash
python arritmic3D.py --test test_case/
```

This command will run a simulation on a slab with some predefined parameters, applying an S1-S2 protocol with a first base cycle length (BCL) of 800 ms and a second BCL of 400 ms. The simulation will last for 3500 ms, and the results will be saved in the `test_case/` directory in the form of a series of VTK files. Note that the case directory must not exist prior to running the command or it must be empty.

By running this example, you will have an idea of how the simulator works and the type of output it generates. In addition to the simulation result, `arritmic3D.py` will generate a configuration JSON file in the case directory, as well as a vtk with the input simulation domain in `test_case/input_data`, which you can use as a starting point for your own simulations.
You can modify the configuration parameters to explore different scenarios.

## Inspection of the test case

You can inspect the generated VTK files using ParaView (https://www.paraview.org/). Open the input VTK file, located in `test_case/input_data/slab_input.vtk`, to see the simulation domain. The slab is a rectangular grid with a uniform distribution of nodes. You can visualize the `restitution_model` point data field to see which restitution model is assigned to each node.

The domain is a rectangle that spans in the XY plane, with a small thickness in the Z direction. The slab has an outer layer of nodes with `restitution_model` equal to 0, which represent non-tissue. You can view the tissue region by applying a threshold filter to show only nodes with `restitution_model` greater than 0.
The rest of the nodes have `restitution_model` equal to 1, indicating that they belong to tissue and will use the Ten Tuscher restitution model for healthy endocardium.
The following image shows the slab with the threshold applied. The arrows indicate the parameters that need to be set to visualize the tissue region.

![The input slab, visualized in Paraview. You will need to apply a threshold filter to see the tissue region that is hidden under a layer of non-tissue nodes (with `restitution_model` equal to 0).](doc/input_slab.png)

If we change the coloring to the `activation_region` field, we can see the activation region defined for the S1-S2 protocol. In this case, the south side of the slab (the nodes with lowest Y value) has been assigned `activation_region` equal to 1, indicating that it is the pacing site for the protocol. The following image shows the slab with the activation region highlighted.

![The input slab visualized in ParaView, showing the activation region. The lower portion of the slab is highlighted in orange/red, indicating the activation region assigned ID 1.](doc/input_slab_region.png)

Now, we can open the sequence of VTK files generated as output of the simulation. These files are located in the `test_case/` directory. Each file corresponds to an instant of time in the simulation and contains different point data fields. If we apply the same threshold as before, we can visualize the activation wavefront propagating through the tissue. Make sure to hide the input slab. You can press the _play_ button in ParaView to see the time evolution of the activation.
The following image shows the slab with the activation wavefront visible.

![A snapshot of the activation wavefront propagating through the tissue, visualized in ParaView.](doc/output_slab_wavefront.png)

In the test case you will also find the configuration file used for the simulation, located at `test_case/arr3D_config_run.json`.
In it, you will find parameters to determine the restutution models, the input file or the simulation duration, among others.
The stimulation protocol is defined in the `PROTOCOL` section of the configuration file.

## Running existing cases

Now that we have a a basic case, we can use it as a template for other simulations. The first thing we can do is to run the same case again, as is. To do so, we can run the following command:

```bash
python arritmic3D.py test_case/
```

This command will run the simulation again, using the same configuration file and input data as before. The results will be saved in the `test_case/` directory, overwriting the previous results. Yoy can open the new VTK files in ParaView to see the results, that should be identical to the previous run.

Now, let's modify some parameters in the configuration file to see how they affect the simulation. For example, we can change the simulation duration to 2500 ms and the VTK output period to 5 ms. and reduce the BCL of the pacing protocol to 600ms and 300ms.
Then, run again the simulation with the same command. Since we now have higher temporal resolution in the output, we will be able to see the activation wavefront more clearly in ParaView.


# Simulation output

During the simulation, VTK files are written to the case directory with the pattern:

- `<input_basename>_<time>.vtk` (e.g., `slab_100.vtk`, `slab_200.vtk`, ...)

Each VTK contains point data fields:

- `State`: current cell state (integer)
- `APD`: action potential duration
- `DI`: diastolic interval
- `LastDI`: last diastolic interval used
- `CV`: conduction velocity
- `LAT`: local activation time
- `LifeTime`: cumulative time since last activation
- `Beat`: beat counter per cell

The output cadence is controlled by `TK_OUTPUT_PERIOD` (ms). Disable/enable via `VTK_OUTPUT_SAVE`.




# Runing simulations

Once we have an overall idea of how Arritmic3D works, we present a more detailed explanation of how to run simulations. To simulate a case using `arritmic3D.py`, you need:

- A VTK file containing a `RectilinearGrid` with at least the required **point data** field `restitution_model`.
- A case directory to write results.
- A configuration file in JSON format.
- A set of restitution models, in the form of several CSV files.

The program is invoked from the command line by passing the case directory as an argument:

```bash
python arritmic3D.py <case_directory>
```

It will look for the configuration file inside `case_directory`. In the configuration file, one of the options sets the input file that defines the simulation domain.

**Units**: The simulator is unit agnostic, and you can use any set of units provided that you are consistent across all inputs and parameters. However, note that the default configuration **and restitution** models for APD and CV are defined in milliseconds and in millimeters per millisecond. Thus, if you want to change the units, you will need to adapt the restitution models accordingly.

## Definition of the simulation domain

For the Python interface, this domain definition is done using an input VTK file. The input VTK file has to store a `RectilinearGrid` that includes, at least, the following point data field:

- **restitution_model**: An integer field indicating the restitution model to be used for each cell. This field is used to define the cells that belong to tissue and to select the restitution model for each cell. Nodes with `restitution_model` equal to 0 are considered non-tissue (void). The value for tissue regions starts from 1 upwards. The mapping between `restitution_model` values and the corresponding restitution model files is defined in a separate CSV file specified in the configuration, as described later.

Additionally the following optional point data fields can be defined:

- **fibers_orientation**: A 3-component vector field representing the orientation of the fibers in each cell. To define isotropic conduction, set all components to zero. If this field is not present, isotropic conduction is assumed for all cells.
- **activation_region**: A scalar field (integers) that define different regions on the tissue where activation is wanted. This field is used to define activation regions, such as the pacing sites for a pacing protocol or the PMJs. The activation times are defined in the configuration file and can refer to these regions by ID. If this field is not present in the tissue, activations can be explicitly prescribed at certain node IDs, when defining the activation in the configuration file.



## Setting configuration parameters

You can set the configuration parameters in two ways: using a configuration JSON file or passing configuration options via the command line interface (CLI). By default, the program looks for a configuration JSON file in the case directory (see below). But you can also specify a different configuration file using the `--config-file` option:

```bash
python arritmic3D.py <case_directory> --config-file path/to/config.json
```

Configuration files are handled as follows:

- If `--config-file` is provided, it is used (error if it does not exist).
- If not provided, the program looks in `<case_directory>` (prefers `arr3D_config.json`, otherwise the first `*.json`).
- If no JSON is found, built‑in defaults are used so that you can run without a config file.

Addtionally, you can override specific configuration parameters directly from the command line using the `-p KEY=VALUE` option. This option can be used multiple times to set different parameters.
For instance, when we ran the test case we modified the configuration file located in the case directory to change the frequenciy at which VTK files are written. Instead of modifying the file, we could have changed this an other parameters directly from the command line as follows:

```bash
python arritmic3D.py test_case/ -p VTK_OUTPUT_PERIOD=5 -p SIMULATION_DURATION=2500
```

Arritmic3D would have used the configuration file in `test_case/`, but the value of `VTK_OUTPUT_PERIOD` and of `SIMULATION_DURATION` would have been overridden to 5 ms and 2500 ms respectively. In this way, you can easily modify specific parameters without changing the configuration file.


Arritmic3D uses the following precedence rules when combining configuration sources: CLI overrides have priority over JSON values (i.e. `-p KEY=VALUE` overrides the same key in the JSON), and `--input-file` overrides `VTK_INPUT_FILE` from the JSON.

## Backup of the run configuration

The actual configuration used in the execution is the result of combining the base JSON (if any) with all CLI overrides (including --input-file). We call this the “run configuration”. For reproducibility, this exact configuration is written to disk so the run can be replicated.
The configuration used for the run is saved to `<case_directory>/arr3D_config_run.json`.
You can disable saving of the run configuration with `--no-output-run-config` if you do not want to write the file.

### Paths and reproducibility
In order to facilitate reproducibility and sharing of cases, paths are handled consistently as follows:

- All paths passed as command line arguments (e.g., `--config-file`, `--input-file`, or `-p KEY=VALUE` for path values) are interpreted relative to the current working directory.
- Paths read from JSON files are interpreted relative to the location of the JSON file being loaded (whether from case_dir or via `--config-file`).
- Consistently, the _run configuration_ saved to `case_dir/arr3D_config_run.json` converts paths and stores them relative to the case directory, to ease sharing and reproducibility.


# The configuration file

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
- **PROTOCOL**: Stimulation protocol settings, using an $S_1,S_2,...,S_n$ pacing protocol. It is a list of dictionaries, each defining a pacing site or region, with the following fields:
    - **ACTIVATION_REGION**: An integer `i`, indicating the activation region ID defined by the field `activation_region` in the tissue. Alternatively, it can be a list of integers that will be interpreted as node IDs where the activation will be applied.
    - **N_STIMS_PACING**: List with the number of stimuli for the pacing region.
    - **BCL**: List of base cycle lengths (BCL) for each pacing.
    - **FIRST_ACTIVATION_TIME**: Time (in ms) of the first activation for that pacing site. The first stimulus will be applied at this time, and subsequent stimuli will follow according to the BCL. Optional; if not provided, the first activation time is set to the first BCL.
    - **FIRST_BEAT_NUM**: Beat number to assign to the first activation at that pacing site. Optional; if not provided, the first beat number is set to 1.

- **ACTIVATE_NODES**: List of dictionaries defining specific activation times for nodes. Each dictionary has the following fields:
    - **ACTIVATION_REGION**: An integer, referring to the nodes that have that value in the `activation_region` field of the VTK file, or a list of node IDs.
    - **ACTIVATION_TIMES**: List of pairs `[time, beat]`, where `time` is the activation time in ms, and `beat` is the beat number to assign to that activation.

The definition of large activation regions and cumbersome protocols can lead to unreadable configuration files.
To facilitate the definition of stimulation protocols involving many nodes with different activation times, it is possible to use **external JSON files**. In the `PROTOCOL` dictionary, they can be used in place of the `ACTIVATION_REGION` and `FIRST_ACTIVATION_TIME` fields. In this case, the files must contain a single JSON list with the node IDs and the first activation time, respectively. Both lists must have the same length.
In the `ACTIVATE_NODES` dictionaries, the `ACTIVATION_REGION` field can also be replaced by an external JSON file containing a list of lists with the form `[ID,time,beat]`, where the `ID` is a node ID. In this case, the `ACTIVATION_TIMES` field is not needed and is ignored if present.

## Defining stimulation protocols

Next you will find some examples of how to define stimulation protocols in the configuration file.

Let's start with a simple S1-S2-S3-S4 protocol example.

```json
"PROTOCOL": [
  {
    "ACTIVATION_REGION": [100, 200, 300],
    "N_STIMS_PACING": [8, 2, 1, 1],
    "BCL": [600, 400, 300, 200]
  }
]
```

This example defines a protocol with pacing site formed by nodes 100, 200, and 300. The protocol consists of 8 stimuli at a BCL of 600 ms, followed by 2 stimuli at a BCL of 400 ms, then 1 stimulus at 300 ms, and finally 1 stimulus at 200 ms.

`N_STIMS_PACING` and `BCL` lists do not need to have the same length. If they differ, the last value of the shorter list is repeated until both lists have the same length. For instance, the following is equivalent to the previous example.

```json
"PROTOCOL": [
  {
    "ACTIVATION_REGION": [100, 200, 300],
    "N_STIMS_PACING": [8, 2, 1],
    "BCL": [600, 400, 300, 200]
  }
]
```

 Note that here, `N_STIMS_PACING` has 3 values, while `BCL` has 4 values; thus, the last `N_STIMS_PACING` value (1) is repeated to match the length of the `BCL` list.

The activation time of the first stimulus can be set using the `FIRST_ACTIVATION_TIME` field. For example, the following protocol starts the first activation at 50 ms.

```json
"PROTOCOL": [
  {
    "ACTIVATION_REGION": [100, 200, 300],
    "N_STIMS_PACING": [8, 2, 1],
    "BCL": [600, 400, 300],
    "FIRST_ACTIVATION_TIME": 50
  }
]
```

Note that using this option will not change the numer of activations, only the time at which the first activation occurs.I this case, the first activation will occur 50 ms after the simulation starts.  This can generate an apparent mismatch between the first activation time and the BCL schedule.
This option is an advanced setup, and is only meant to add flexibility to the protocol definition if the starting state of the tissue considers a certain activation state. That is, the previous example could be useful with a tissue that was activated 550ms before the simulation started, so that the first activation must occur at 50 ms (600ms after the last activation).
If this field is not provided (which is the recommended option), the first activation time is set to the value of the first BCL (which would be 600 ms after the simulation start in the previous example).


External files can be used to reduce the size of the configuration file when the activation must be defined for each node separately. The first example can be rewritten as follows:

```json
"PROTOCOL":  [
  {
    "ACTIVATION_REGION": "cases/slab_test_protocol_sites_beat1.json",
    "FIRST_ACTIVATION_TIME": "cases/slab_test_protocol_first_activation_beat1.json",
    "N_STIMS_PACING": [8, 2, 1, 1],
    "BCL": [600, 400, 300, 300]
  }
]
```

Where `slab_test_protocol_sites_beat1.json` contains:

```json
[100, 200, 300]
```

And `slab_test_protocol_first_activation_beat1.json` contains:

```json
[50, 55, 72]
```

This example shows how to define different first activation times for each node in the pacing site. This can be useful, for instance, when we have the activation time at PMJs in ventricle.

The following example shows a protocol with two different pacing sites, each with its own activation schedule. It also illustrates the use of the `FIRST_BEAT_NUM` field to customize beat numbering.

```json
"PROTOCOL":
[
         {
            "ACTIVATION_REGION": 1,
            "FIRST_ACTIVATION_TIME": 100,
            "N_STIMS_PACING": [3, 2],
            "BCL": [800, 500]
        },
        {
            "ACTIVATION_REGION": 2,
            "FIRST_ACTIVATION_TIME": 600,
            "N_STIMS_PACING": [3, 2],
            "BCL": [800, 500],
            "FIRST_BEAT_NUM": 6
        }
]
```


Each activation is assigned a beat number, which can be used to detect reentries.
When using `PROTOCOL` to activate tissue, the first activation at each pacing site is assigned beat number 1, and subsequent activations increment the beat number by 1.
However, you can customize the beat numbering by using the `FIRST_BEAT_NUM` field to decide the starting beat number. This can help distinguish activations from different pacing sites or protocols. In the previous example, the beats from the first pacing site are numbered in the range 1 to 5, while the beats from the second pacing site start from beat number 6.


## Activating by node id + time

Activation times can be prescribed explicitly for each node using the `ACTIVATE_NODES` parameter. Here is an example:

```json
"ACTIVATE_NODES" : [
    {
        "ACTIVATION_REGION" : [1,2,3],
        "ACTIVATION_TIMES" : [[500,1],[550,2]]
    },
    {
        "ACTIVATION_REGION" : 1,
        "ACTIVATION_TIMES" : [[1500,4],[1550,5]]
    }
]
```

This example defines specific activation times for nodes 1, 2, and 3 at 500 ms (beat 1) and 550 ms (beat 2), and for nodes with `activation_region` value 1 at 1500 ms (beat 4) and 1550 ms (beat 5).
Note the difference between using a list of node IDs and an integer referring to the `activation_region` field in the VTK file.
When using `ACTIVATE_NODES`, you must always explicitly set the beat number for each activation in the `ACTIVATION_TIMES` list.

External files can also be used as follows:

```json
"ACTIVATE_NODES" : [
    {
        "ACTIVATION_REGION" : "cases/slab_test_stim_nodes_beat1.json"    }
]
```

where `slab_test_stim_nodes_beat1.json` contains:

```json
[[1,500,1],[2,550,2],[3,600,3]]
```

In this case, the first value in each tuple contains a node ID, the second value the activation time, and the third value the beat number.

# Definition of restitution models

The restitution models can be defined by means of restitution curves or surfaces, as described in (Serra et al., 2022; Romitti et al., 2025). A restitution curve is a function that provides the next Action Potential Duration (APD) of a cell as a function of the Diastolic Interval (DI) at activation. A restitution surface is a function that provides the next APD of a cell as a function of its last APD and the DI at activation.

The simulator needs a restitution model for each tissue type (representing, e.g. tissue region, as in endo/mid/epi, or type, such as healthy/border zone). The restitution models (curves or surfaces) are encoded as a table in a CSV file. Another CSV file must indicate the tissue region-type that is modeled in each table. Actually, all the models are treated as surfaces, even if they are curves. In that case, the table will have a single row for each previous APD value, with an ourput APD value for each DI.

The following CSV corresponds to a restitution model

```
0.0  , 30.0 , 35.0 , 40.0 , 45.0 , 50.0
95.5 , -1.0 , 89.44, 89.67, 89.90, 90.13
99.5 , 89.64, 89.87, 90.10, 90.32, 90.52
103.5, 90.73, 90.95, 91.17, 91.38, 91.59
107.5, 91.67, 91.88, 92.09, 92.30, 92.50
```

For the restitution model CSV table shown above, the first row contains the Diastolic Interval (DI) values measured in milliseconds. The first column lists the previous Action Potential Duration (APD) values, also in milliseconds. The remaining cells in the table contain the computed next APD values in milliseconds. Note the use of -1.0 to indicate that no activation occurs for that combination of previous APD and DI.

Dead (Core Zone) tissue can be modeled by setting the restitution model index to 0. But, if desired, it can also be mapped to a table that always returns -1.0 for any input values.

```
0.0  , 30.0
100  , -1.0
```

The actual values for DI and previous APD are not relevant in this case, as no activation will ever occur.

Important considerations for the restitution table format:

- The table must include the minimum DI value that allows cell activation, along with the preceding DI value. For the preceding value, use -1 to indicate no activation occurs. That is, in every row, at least the first column must contain -1 for the DI value that precedes the minimum activation DI value. This ensures that the model can correctly identify the threshold for activation.
- For APD or DI input values that fall outside the table's defined ranges, the model applies flat extrapolation to determine the output values.

## Mapping tissue region-types to restitution model files

The field `restitution_model` in the input VTK file indicates the tissue region-type for each cell. The value of this field is used to select the restitution model for each cell according to the mapping provided in the second CSV file.

To specify which tissue regions use which restitution models, a separate CSV file maps the region types to table indices. For example:

```
1 , TenTuscher_APD_BZ_Mid.csv
2 , TenTuscher_APD_BZ_Epi.csv
3 , TenTuscher_APD_Healthy_Mid.csv
```

As discussed, index '0' is reserved for non-tissue regions and cannot be used.


# Generating tissue slabs

Using the `build_slab.py` utility, you can generate a rectangular tissue domain (a tissue slab) for testing. This script creates a VTK file with the required fields for simulation.

For example, to generate a slab with 10 nodes in X, 10 in Y, and 5 in Z, with spacing of 0.05 mm, run:

```bash
python build_slab.py cases/slab.vtk --nnodes 10 10 5 --spacing 0.05 0.05 0.05
```

This will create a VTK file at `cases/slab.vtk` with the necessary structure and a point data field named `restitution_model` with a value of 1.
You can customize the grid size, spacing, and additional options using the script arguments.

Note that a value of `nnodes` equal to 10 means that the slab will have 10 points in each dimension (0 to 9), resulting in a grid size of 9 * spacing in each dimension.


## Setting default values for point data fields

You can set default values for point data fields using the `--field FIELD_NAME VALUE` option. This option can be used multiple times to set different fields. For example, to set the default restitution model to 1 and the fibers orientation to [1,0,0], run:

```bash
python build_slab.py cases/slab.vtk --nnodes 20 20 5 --spacing 0.05 0.05 0.05 \
  --field restitution_model 1 \
  --field fibers_orientation [1,0,0]
```



### Setting the default restitution model

In particular, and as mentioned above, you can set the restitution model for all tissue nodes using the `--field` option. This will set the base restitution model that can be later modified using regions (see next section).
For example, to set the default restitution model to 2, run:

```bash
python build_slab.py cases/slab.vtk --nnodes 20 20 5 \
    --spacing 0.05 0.05 0.05 --field restitution_model 2
```

## Modifying properties by region

It is possible to modify the value of any field (and, in particular, the restitution-model identifier) at the points in a particular region of the generated slab. Supported region shapes are `circle` (inner core + outer ring), `square`, and `diamond` (45°-rotated square). Each region is specified in world distance units (the same units as `--spacing`) and updates the `restitution_model` point-data field accordingly. Regions are applied in sequence and later regions overwrite earlier ones in case of overlap.

You can create such regions in the generated slab using the following CLI options.


- `--regions-file PATH`
  Path to a JSON file that contains a list of region objects. Each object must be a JSON object describing a region (see schema below). The file is loaded first.

- `--region JSON` (repeatable)
  Add a region by passing a JSON object string on the command line. This option can be used multiple times; each occurrence appends a region. CLI regions are processed after the regions file and therefore override file regions on overlap.

### Regions schema and behavior

Each region object is a JSON object with the following schema:

  - circle: `{ "shape":"circle", "cx":float, "cy":float, "r1":float, "r2":float, <field>: scalar|[...] }`
  - square: `{ "shape":"square", "cx":float, "cy":float, "r1":float, "r2":float, <field>: scalar|[...] }`
  - diamond: `{ "shape":"diamond", "cx":float, "cy":float, "r1":float, "r2":float, <field>: scalar|[...] }`
  - side: `{ "shape":"side", "side":str, <field>:value }`
  - node_ids: `{ "shape":"node_ids", "ids":[int,...], <field>:value }`

Here, `<field>` can be any point data field name (e.g., `"restitution_model"`, `"activation_region"`). For `circle/square/diamond`: field values can be scalar, or list (gradient with first element at `r1` and last at `r2`).

Precedence and behavior

- Precedence: regions are applied in this order: regions from `--regions-file` first, then regions supplied via `--region` in the order given, and finally the legacy single-region flags (`--add_square`, `--add_circle`, `--add_diamond`). Later regions overwrite earlier ones on overlap.
- Validation: input is strictly validated. If a region object is missing required fields or has invalid types, the script raises an explanatory exception and stops..
- Units: all coordinates and distances in region objects are interpreted in world distance units (the same units as `--spacing` and grid coordinates).

### Examples

```bash
# multiple regions via CLI (repeat --region)
python build_slab.py cases/out.vtk --nnodes 20 20 5 --spacing 0.05 0.05 0.05 \
   --region '{"shape":"square","cx":0.5,"cy":0.5,"r1":0.05,"r2":0.1,"restitution_model":7}' \
   --region '{"shape":"circle","cx":0.1,"cy":0.1,"r1":0.1,"r2":0.4,"restitution_model":[1,2,3]}'

# regions-file containing a list of regions
python build_slab.py cases/out.vtk --nnodes 20 20 5 --spacing 0.05 0.05 0.05 \
   --regions-file ./cases/regions_example.json

# file + CLI: CLI regions override file regions on overlap
python build_slab.py cases/out_mix.vtk --nnodes 40 40 5 --spacing 0.05 0.05 0.05 \
    --regions-file ./cases/regions_example.json \
    --region '{"shape":"diamond","cx":1.5,"cy":1.0,"r1":0.5,"r2":0.8,"restitution_model":[9,5,4]}'
```

An example regions JSON file (`regions_example.json`) could look like this:

```json
[
  { "shape":"circle", "cx":0.5, "cy":0.5, "r1":0.2, "r2":0.4, "restitution_model":[2, 3] },
  { "shape":"square", "cx":1.0, "cy":1.0, "r1":0.15, "r2":0.3, "restitution_model":[4, 5] }
]
```

## Definition of activation regions

Activation regions (stimulation or pacing sites) are defined using the same region system for restitution models and other fields, targeting the `activation_region` field. Any region shape (circle, square, diamond, side, node_ids) can be used to set `activation_region` values.

### Examples of activation regions

You can define activation sites by including the `activation_region` field in region objects:

```bash
# Define a square region with activation_region=1
python build_slab.py cases/slab.vtk --nnodes 50 50 5 --spacing 0.05 0.05 0.05 \
  --region '{"shape":"square","cx":0.5,"cy":0.5,"size":0.2,"activation_region":1}'

# Activate south side with activation_region=1
python build_slab.py cases/slab.vtk --nnodes 50 50 5 --spacing 0.05 0.05 0.05 \
  --region '{"shape":"side","side":"south","activation_region":1}'

# Activate specific nodes with activation_region=2
python build_slab.py cases/slab.vtk --nnodes 50 50 5 --spacing 0.05 0.05 0.05 \
  --region '{"shape":"node_ids","ids":[100,200,300],"activation_region":2}'
```

A regions file with mixed restitution models and stimulation sites could look like:

```json
[
  { "shape":"square", "cx":1.0, "cy":1.0, "size":0.5, "restitution_model":5 },
  { "shape":"side", "side":"south", "activation_region":1 },
  { "shape":"node_ids", "ids":[8553,9000], "activation_region":2 }
]
```

### Convenience options to define activation regions

For common activation scenarios, two convenience CLI options are provided:

- `--region-by-side SIDE REGION_ID`
  Defines an entire slab side as an activation region. `SIDE` must be one of: `north`, `south`, `east`, `west`. `REGION_ID` is the integer value to assign to `activation_region`. This option can be repeated to activate multiple sides with different region IDs.

- `--region-by-node-ids NODE_ID [NODE_ID ...] REGION_ID`
  Defines a region based on specific nodes by their IDs. All arguments except the last are node IDs; the last argument is the `REGION_ID` to assign. This option can be repeated to create multiple activation groups.

Examples:

```bash
# Activate south side with region_id=1
python build_slab.py cases/slab.vtk --nnodes 50 50 5 --spacing 0.05 0.05 0.05 \
  --region-by-side south 1

# Activate multiple sides with different region IDs
python build_slab.py cases/slab.vtk --nnodes 50 50 5 --spacing 0.05 0.05 0.05 \
  --region-by-side south 1 \
  --region-by-side north 2

# Activate specific nodes
python build_slab.py cases/slab.vtk --nnodes 50 50 5 --spacing 0.05 0.05 0.05 \
  --region-by-node-ids 2775 2776 2777 2778 2779 1

# Combine geometric regions with activations
python build_slab.py cases/slab.vtk --nnodes 50 50 5 --spacing 0.05 0.05 0.05 \
  --region '{"shape":"square","cx":1.225,"cy":1.225,"size":1.0,"restitution_model":5}' \
  --region-by-side south 1 \
  --region-by-node-ids 2775 2776 2777 2778 2779 2
```

**Note**: The convenience options `--region-by-side` and `--region-by-node-ids` are internally converted to region objects and applied in the same sequence as `--region` entries. Activations defined via these options are processed after `--regions-file` and `--region`, so they can overwrite previous values on overlap.



## Gradient support for geometric regions

For geometric regions (circle, square, diamond), a gradient can be set for a smooth transition of field values between the inner radius `r1` and outer radius `r2`. This allows for gradual changes in properties across the region.
The gradient is defined by providing a list of values for the field. The interpretation depends on the value provided:

- **Scalar**: Applied uniformly at `r2` (outer radius/size).
- **List**: Creates a smooth transition between `r1` and `r2`. The first value of the list applies at `r =< r1`, the last at `r2`, and intermediate values are set at uniform intervals between `r1` and `r2`.

To set vector fields, the list can contain vectors to define a gradient of vectors. If a uniform vector is desired, provide a list with a single vector. For instance, to set a uniform fiber orientation of `[1,0,0]`, use `"fiber_orientation" : [ [1,0,0] ]`.

For **existing vector fields**, if the type of the previous field is different from the provided value, then an error is raised.

Single values (scalar or single value lists) are applied at **`r2`**. Also, if a list is provided but `r1 == r2`, then the first element is applied to the entire region (where `r =< r1`).

Examples:

```bash
# Uniform scalar value at r =< r1
--region '{"shape":"circle","cx":1.0,"cy":1.0,"r1":0.2,"r2":0.2,"restitution_model":3}'

# Uniform vector field at r =< r1
--region '{"shape":"square","cx":1.0,"cy":1.0,"r1":0.15,"r2":0.15,"fibers_orientation":[[1,0,0]]}'

# Scalar gradient (3 layers interpolated between r1 and r2)
--region '{"shape":"circle","cx":1.0,"cy":1.0,"r1":0.2,"r2":0.6,"restitution_model":[5,4,3]}'

# Gradient of vectors
--region '{"shape":"square","cx":0.5,"cy":0.5,"r1":0.1,"r2":0.3,"fibers_orientation":[[1,0,0],[0,1,0]]}'

```

**Notes:**

- In the gradient `[5,4,3]`: model 5 at r1=0.2, model 4 at midpoint ($r\simeq 0.4$), model 3 at r2=0.6.
- For uniform application across the entire circular/square/diamond region, use `r1 == r2`.



# Direct execution of tissue slabs

The `arritmic3D.py` script includes a `--slab` option that allows you to directly build and run a slab simulation without needing to create the VTK file separately. This option generates a rectilinear grid slab based on the provided parameters and runs the simulation using default or specified configuration settings.

When using `--slab`, you can use in `arritmic3D.py` the same options available in `build_slab.py` to define the slab properties and regions. For instance:

```bash
python arritmic3D.py case_dir --slab --nnodes 20 20 5 --spacing 0.05 0.05 0.05 \
  --region '{"shape":"square","cx":0.5,"cy":0.5,"r1":0.05,"r2":0.1,"restitution_model":7}' \
  --region-by-side 'south' 1 \
  -p ACTIVATE_NODES='[{"ACTIVATION_REGION" : 1,"ACTIVATION_TIMES":[[500,1],[1000,1]]}]'
```

In this example, the lower margin of the slab (south side) is defined as an activation with region ID 1, and specific activation times are defined for nodes with that region ID. Further details on defining regions and activation sites can be found in the respective sections of this README.

When using `--slab`, the generated VTK will be used. In this case, you are not allowed to use the `--input-file` option and any `VTK_INPUT_FILE` present in the JSON will be ignored.



# Publications

Please, cite as:

* Serra, D., Romero, P., Garcia-Fernandez, I., Lozano, M., Liberos, A., Rodrigo, M., ... & Sebastian, R. (2022). An automata-based cardiac electrophysiology simulator to assess arrhythmia inducibility. Mathematics, 10(8), 1293. https://doi.org/10.3390/math10081293

Related research:

* Serra, D., Romero, P., Franco, P., Bernat, I., Lozano, M., Garcia-Fernandez, I., ... & Sebastian, R. (2025). Unsupervised stratification of patients with myocardial infarction based on imaging and in-silico biomarkers. IEEE Transactions on Medical Imaging. https://doi.org/10.1109/tmi.2025.3582383
* Romitti, G. S., Liberos, A., Termenón-Rivas, M., Barrios-Álvarez de Arcaya, J., Serra, D., Romero, P., ... & Rodrigo, M. (2025). Implementation of a Cellular Automaton for efficient simulations of atrial arrhythmias. Medical Image Analysis, 103484. https://doi.org/10.1016/j.media.2025.103484
* Serra, D., Romero, P., Lozano, M., Garcia-Fernandez, I., Penela, D., Berruezo, A., ... & Sebastian, R. (2023, October). Patient Stratification Based on Fast Simulation of Cardiac Electrophysiology on Digital Twins. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 35-43). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-52448-6_4
* Serra, D., Franco, P., Romero, P., Romitti, G., García-Fernández, I., Lozano, M., ... & Sebastian, R. (2023, July). Assessment of Risk for Ventricular Tachycardia based on Extensive Electrophysiology Simulations. In 2023 45th Annual International Conference of the IEEE Engineering in Medicine & Biology Society (EMBC) (pp. 1-4). IEEE. https://doi.org/10.1109/EMBC40787.2023.10340169
* Serra, D., Franco, P., Romero, P., García-Fernández, I., Lozano, M., Soto, D., ... & Sebastian, R. (2022, September). Personalized Fast Electrophysiology Simulations to Evaluate Arrhythmogenicity of Ventricular Slow Conduction Channels. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 56-64). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-23443-9_6