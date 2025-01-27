# Arritmic3D
A fast Eikonal computational model for electrophysiology simulation (Ventricle version).

The simulator has two versions in separate branches: branch `ventricle`, with the ventricle version, and branch `atria`, with the atria version. Checkout the desired branch before proceeding.

This README covers
* [Installation](#installation)
* [Usage](#usage)
* [Publications](#publications)

<p align="center">
    <img alt="Arritmic3D view" src="https://disco.uv.es/cgi-bin/in-public?fileman:es:imgdump:pub:/disco/arritmic3D/doc/ac_img.png:commlab:" width="90%" />
</p>



## Installation
* Install Processing (https://processing.org/) (at least, version 3.4)
* Run processing and install the required libraries. To do so, open the _Tools_ menu, select _Manage tools_ and, there, go to _Libraries_ tab.
    * Install `PeasyCam` and `ControlP5`.
    * Instal the `Toxi` library, following these steps:
        1.	Download `
    toxiclibs-complete-0021.zip` from: https://github.com/postspectacular/toxiclibs/releases.
        2.	Open Processing.
        3.	Go to File->Preferences.
        4.	Check the value of the field labeled "Sketchbook location:" and open this path in a file browser.
        6.	Inside that folder you should find another folder called "libraries". Extract the toxiclib library downloaded in STEP 1 and copy it into this location.
        7.	Restart Processing.
* Clone or download this repository and open the file `arrithmic3D/arritmic3D.pde` from within Processing. Note that Processing requires the directory and the file have the same name.


## Preparing a case
To run a case, you will need several subdirectories inside `arritmic3D`.
* Two directories called `RestitutionCurves` and `RestitutionSurfaces`.
* A directory called `casos` with a case per subdirectory in it.

You can download sample data from Zenodo [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.14277909.svg)](https://doi.org/10.5281/zenodo.14277909).
Optionally, you can build your own case from a VTK file using the scripts in the `Readers/` directory (no documentation for this yet, sorry...). Block cases (rectangular slabs of tissue) can also be built.



### Restitution curves directory
This directory contains the parameters of the restituion curve activation models.
* `restitutionCurves` directory. Inside this folder are files in the format `RestitutionCurve_A_B_C.csv` where A={BZ, Sanas} (respectively: Border Zone, Helthy); B={APD, CV} and C={Endo, Epi, Mid}


### Cases directory
A directory called `casos/` is necessary. There, each case will be hosted inside a subdirectory, containing the geometric and biophysical information for the case. You can download the Krueger et al. (2013) simulation case from the URL indicated above.
Inside each case directory the files required are:

* `Endo.stl`
* params.dat (The parameters to be entered in this file are described below)
* A folder `Reader_VTK` that contains:
    * `cell_conex_nodos.txt` : M x N matrix describing for each of the M elements the N nodes that make it up
    * `centerMass-BLP.txt` : 3 x 1 vector describing the (x, y, z)’ coordinate of the center of mass of the atria
    * `centroidCellX.txt` : R x 1 vector describing for each of the R nodes of the geometry its x-coordinate
    * `centroidCellY.txt` : R x 1 vector describing for each of the R nodes of the geometry its y-coordinate
    * `centroidCellZ.txt` : R x 1 vector describing for each of the R nodes of the geometry its z-coordinate
    * `EndoToEpi.txt` : R x 1 vector describing for each of the R nodes the Endo (0) or Epi (1) belonging
    * `fibreOrientationX.txt` : R x 1 vector describing for each of the R nodes of the geometry its fibre orientation vector x-component
    * `fibreOrientationY.txt` : R x 1 vector describing for each of the R nodes of the geometry its fibre orientation vector y-component
    * `fibreOrientationZ.txt` : R x 1 vector describing for each of the R nodes of the geometry its fibre orientation vector z-component
    * `nodes_CasoBloqueVent.txt` : S x 1 vector containing the nodes IDs activated for the re-entrance case (! nodes counting starts from 0)
    * `pacing_34.txt` : NOT USED but must be present in the folder (it was prepared for the Ventricle case)
    * `scalarValues.txt` : NOT USED but must be present in the folder (it was prepared for the Ventricle case)
    * `scarTissue`.txt` : NOT USED but must be present in the folder (it was prepared for the Ventricle case)
    * `vecinos.txt` : R x P matrix describing for each of the R nodes of the geometry its P neighbors
    * `vertex_DlongDtransratio.txt` : R x 1 vector describing for each of the R nodes of the geometry its CV tissue-specific (CVL/CVL,LA)  factor
    * `vertex_reductCV.txt` : R x 1 vector describing for each of the R nodes of the geometry CV transversal-to-longitudinal (CVL/CVT) factor

## Usage

### Defining the simulation parameters
Open `paramsInit.dat` file located in `arritmic3D/arritmic3D` with an editor, and modify the number case option and the case folder name  you want to simulate:
````
case           :  1                         ### Case options: VENTRICLE -> 0 / BLOCK_VTK -> 1
folderCase     :  Bloque_vtk_Z06_100x200    ### FolderCase: The case folder name located in arritmic3D/arritmic3D/casos
````
Next, open `params.dat` file located in `arritmic3D/arritmic3D/casos/<folderCase>` with an editor, and modify the case parameters as you need. The definition of each parameter is detailed below in the comments:
````
model                   :   Bloque.stl                  #  Polydata mesh name to display
hay_mesh                :   true                        #  If there is Polydata mesh to display
grid_enable             :   false                       #  Do not modify, currently disabled
pinta_cada_n            :   1                           #  Draw every 'n' nodes. Value 1 draws all
id_extraI               :   27891                       #  Initial pacing node id
active_MultiIDs_extraI  :   true                        #  For Block case, initial pacing nodes id's, defined in txt. This option disables the activation of vertical/horizontal front or single node
fileName_MultiIDs_extraI:   initNodesMulti.txt          #  Name of txt file, which contains in each line: node id, activation time delay and XYZ vector of propagation direction (All separated by spaces)
voxel_size              :   5                           #  Draw size of each node in pixels
tam_frente              :   5.0                         #  For the active/inactive wavefront display mode, it defines the time before inactive and after active of the nodes to be displayed
multi_view              :   false                       #  For Ventricle case, shows front and back view at the same time
show_cmap               :   true                        #  Show color map
dt                      :   1.0                         #  Time step to update simulation events
vmode                   :   1                           #  Defines the initial display mode (this can be changed later during the simulation). By default, value 1 displays life time
tmode                   :   2                           #  Defines the type of cell to display, by its conductivity (0 - healthy, 1 - border zone, 2 - healthy+border zone)
fps                     :   1000                        #  Maximum framerate. Do not modify if it is not necessary to avoid increasing the computation time in simulations without visualization, such as MultiSim
show_path               :   false                       #  Do not modify, currently disabled
show_frente             :   true                        #  Activate simulation visualization
show_graph              :   false                       #  Do not modify, currently disabled
rec_frame               :   false                       #  Record sequence of simulation images to build video (show_frente must be activated)
show_gui                :   true                        #  Enable GUI to modify some parameters in individual simulations
rango_visual            :   250                         #  Visual range for color map
rango_min               :   0                           #  Minimum visual range value for color map
radio_cateter           :   0                           #  Radius distance in millimeters simulating catheter pacing
min_pot_act             :   0.0                         #  Do not modify, currently disabled
di_Sana_Init            :   230                         #  Diastolic interval of healthy cells to define initial state
di_BZ_Init              :   185                         #  Diastolic interval of border zone cells to define initial state
stimFrecS1              :   500                         #  Stimulus frequency for S1
stimFrecS2              :   300                         #  Stimulus frequency for S2
nStimsS1                :   3                           #  Number of S1 stimuli (The cellular automaton only needs 3 S1 stimuli to stabilize)
nStimsS2                :   2                           #  Number of S2 stimuli
apd_memory              :   0.4                         #  % APD memory applied to new activation (supported values from 0-1, 0%-100%)
cv_memory               :   0.05                        #  % Conduction velocity memory applied to new activation (supported values from 0-1, 0%-100%)
reductCVtr              :   0.70                        #  % Reduction of the conduction velocity in the transverse direction to the fiber orientation (supported values from 0-1, 0%-100%) Ex: 0.70, applies in the transverse direction 70% of the longitudinal conduction velocity
apd_isot_ef             :   0.4                         #  % Electrotonic effect of neighboring APDs (supported values from 0-1, 0%-100%)
dir_fibras              :   0.0,0.0,0.0                 #  Fiber orientation for the block case (the ventricle case reads from the .txt file the data for each node)
multiSim                :   false                       #  Activate the execution of multiple simulations of the same case, modifying the parameters detailed below in each one. This option by default disables the visualization to reduce computation time. At the end, a .txt file is automatically created with the results of the simulations in the arritmic3D/arritmic3D/multiSims_Res folder
id_extraIMulti          :                               #  Define all initial pacing node id's options to simulate (comma separated, if several. If empty data, get 34 pacing nodes from the 17 AHA segments, 17 endo and 17 Epi)
stimFrecS1Multi         :   600                         #  Define all stimulus frequency options for S1 to simulate (comma separated, if several)
stimFrecS2Multi         :   290,295,300,305,310,315,320 #  Define all stimulus frequency options for S2 to simulate (comma separated, if several)
nStimsS1Multi           :   3                           #  Define all S1 stimulus number options to simulate (comma separated, if several)
nStimsS2Multi           :   2                           #  Define all S2 stimulus number options to simulate (comma separated, if several)
cv_memoryMulti          :   0.05                        #  Define all % Conduction velocity memory options (comma separated, if several)
apd_isot_efMulti        :   0.4                         #  Define all % Electrotonic effect of neighboring APDs options (comma separated, if several)
````

### Running simulation
* Open `Arritmic3D.pde` file in Processing.
* Press the Run button to start the simulation
* The application has different functionalities that are activated by pressing some keys while the 'arritmic3D' display window is active:
    * 'f' key: Simulation visualization on/off
    * 'w' key: Mesh polydata visualization on/off
    * 'Arrow left / right' key: Rotate viewpoint
    * 'm' key: Visualization modes. (Life Time (default), APD, Diastolic interval, Conduction velocity, Wave front active-inactive, Activation period map)
    * 't' key: Visualize type of node by its conductivities (healthy, border zone, healthy+border zone)
    * 'i' key: Enable display of inactive nodes on/off (By default, only active nodes are displayed)
    * 'l' key: Number of active nodes
    * 'g' key: Record sequence of simulation images to build video start/stop ('f' key must be on). Files saved in arritmic3D/arritmic3D/video/videoFrames**
    * 'c' key: Generate Ensight files to save Life time animation start/stop. Saved in folder arritmic3D/arritmic3D/casos/<folderCase>/Case_***
    * 'e' key: Activate saving only the values ​​of BZ nodes in the Lifetime Ensight recording, setting the value of healthy nodes to -2, so that they can be deleted later with threshold
    * 'x' key: Generate Ensight files to save Activation maps on each new stimulus start/stop. Saved in folder arritmic3D/arritmic3D/casos/<folderCase>/A_Map_***
    * 'y' key: Activate saving only the values ​​of Mid nodes in the Activation maps Ensight recording, setting the value of the remaining nodes to -2, so that they can be deleted later with threshold
    * 'z' key: Activate saving only the values ​​of BZ nodes in the Activation maps Ensight recording, setting the value of healthy nodes to -2, so that they can be deleted later with threshold

### Troubleshooting

If there is an error after pressing the Run button to start the simulation, it can be related to the frame rate definition. You can try by changing the line

<code>frameRate(caseParams.fps);</code>

by

<code>frameRate(1000);</code> (or whatever the maximum frame rate you want to be)

in the last line of the <code>setup()</code> function.

## Publications
Please, cite as:
* Serra, D., Romero, P., Garcia-Fernandez, I., Lozano, M., Liberos, A., Rodrigo, M., ... & Sebastian, R. (2022). An automata-based cardiac electrophysiology simulator to assess arrhythmia inducibility. Mathematics, 10(8), 1293. https://doi.org/10.3390/math10081293

Related research:
* Serra, D., Franco, P., Romero, P., García-Fernández, I., Lozano, M., Soto, D., ... & Sebastian, R. (2022, September). Personalized Fast Electrophysiology Simulations to Evaluate Arrhythmogenicity of Ventricular Slow Conduction Channels. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 56-64). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-23443-9_6
* Serra, D., Franco, P., Romero, P., Romitti, G., García-Fernández, I., Lozano, M., ... & Sebastian, R. (2023, July). Assessment of Risk for Ventricular Tachycardia based on Extensive Electrophysiology Simulations. In 2023 45th Annual International Conference of the IEEE Engineering in Medicine & Biology Society (EMBC) (pp. 1-4). IEEE. https://doi.org/10.1109/EMBC40787.2023.10340169
* Serra, D., Romero, P., Lozano, M., Garcia-Fernandez, I., Penela, D., Berruezo, A., ... & Sebastian, R. (2023, October). Patient Stratification Based on Fast Simulation of Cardiac Electrophysiology on Digital Twins. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 35-43). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-52448-6_4
