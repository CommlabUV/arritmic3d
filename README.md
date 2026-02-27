# Arritmic3D: A fast Eikonal computational model for electrophysiology
simulation
CoMMLab
2026-01-01

- [Installation](#installation)
- [Compilation from source](#compilation-from-source)
- [Usage](#usage)
- [Citing Arritmic3D](#citing-arritmic3d)
- [Related research](#related-research)

Arritmic3D is a fast Eikonal computational model for electrophysiology
simulation.

The simulator has three versions in separate branches: branch `main`
(this branch), with a development version, branch `ventricle`, with the
ventricle version, and branch `atria`, with the atria version. Checkout
the desired branch before proceeding.

> [!WARNING]
>
> This development branch is not fully functional and has some
> differences in the diffusion model with respect to the other two
> branches. Thus, **this branch is not validated and should not be used
> for research purposes yet**.

The original version of the solver was developed in the Java environment
[Processing](https://processing.org/). Now it is being migrated to C++
and provided with a Python interface.

# Installation

Arritmic3D is available as a Python package in PyPI. From a python environment, you can install the package using pip:

``` bash
pip install arritmic3d
```

This will also install some convenience scripts that can be used from outside the python environment, as command line tools (see the [usage](#usage) section).

# Compilation from source

You can also compile the wheel yourself. To compile it, you
need a standard C++17 compiler, Eigen and Pybind11 installed in your
system.

As a reference, in an ubuntu machine it should be enough to run:

``` bash
sudo apt-get install \
    build-essential \
    pkg-config \
    cmake \
    python3-dev \
    gcc \
    g++ \
    libeigen3-dev \
    python3-pybind11
```

Then, clone the repository from [GitHub](https://github.com/CoMMLabuv/Arritmic3D) and compile the wheel by running:

``` bash
python -m pip install --upgrade pip setuptools wheel
python -m pip install .
```


# Usage

The solver is presented as a C++ library with is wrapped in a Python module.
In addition, a Python script to run simulations is provided as an executable called `arritmic3d`. This script is meant to provide a reather flexible way to run simulations in many common scenarios.

However, for advanced usage, you can build your own Python programs that use the Arritmic3D module.
You can find the Arritmic3D API documentation
[here](docs/Arritmic3D_API.md).

A guide on the usage of the `arritmic3d` script and other related tools can be found in the
[web page](https://commlabuv.github.io/arritmic3d/).


# Citing Arritmic3D

If you find Arritmic3D useful for your research, please cite the
following paper:


<div id="refs" class="references csl-bib-body hanging-indent"
entry-spacing="0">

<div id="ref-serraAutomataBasedCardiacElectrophysiology2022"
class="csl-entry">

Serra, Dolors, Pau Romero, Ignacio Garcia-Fernandez, Miguel Lozano,
Alejandro Liberos, Miguel Rodrigo, Alfonso Bueno-Orovio, Antonio
Berruezo, and Rafael Sebastian. 2022. “An Automata-Based Cardiac
Electrophysiology Simulator to Assess Arrhythmia Inducibility.”
*Mathematics* 10 (8): 1293. <https://doi.org/10.3390/math10081293>.
</div>

</div>

# Related research

<div id="refs" class="references csl-bib-body hanging-indent"
entry-spacing="0">

<div id="ref-romittiImplementationCellularAutomaton2025"
class="csl-entry">

Romitti, Giada S., Alejandro Liberos, María Termenón-Rivas, Javier
Barrios-Álvarez De Arcaya, Dolors Serra, Pau Romero, David Calvo, et al.
2025. “Implementation of a Cellular Automaton for Efficient Simulations
of Atrial Arrhythmias.” *Medical Image Analysis* 101 (April): 103484.
<https://doi.org/10.1016/j.media.2025.103484>.

</div>

<div id="ref-serraAssessmentRiskVentricular2023" class="csl-entry">

Serra, D., P. Franco, P. Romero, G. Romitti, I. García-Fernández, M.
Lozano, A. Liberos, et al. 2023. “Assessment of Risk for Ventricular
Tachycardia Based on Extensive Electrophysiology Simulations.” In *2023
45th Annual International Conference of the IEEE Engineering in Medicine
& Biology Society (EMBC)*, 1–4. Sydney, Australia: IEEE.
<https://doi.org/10.1109/EMBC40787.2023.10340169>.

</div>

<div id="ref-serraPersonalizedFastElectrophysiology2022"
class="csl-entry">

Serra, Dolors, Paula Franco, Pau Romero, Ignacio García-Fernández,
Miguel Lozano, David Soto, Diego Penela, Antonio Berruezo, Oscar Camara,
and Rafael Sebastian. 2022. “Personalized Fast Electrophysiology
Simulations to Evaluate Arrhythmogenicity of Ventricular Slow Conduction
Channels.” In *Statistical Atlases and Computational Models of the
Heart. Regular and CMRxMotion Challenge Papers*, edited by Oscar Camara,
Esther Puyol-Antón, Chen Qin, Maxime Sermesant, Avan Suinesiaputra, Shuo
Wang, and Alistair Young, 13593:56–64. Cham: Springer Nature
Switzerland. <https://doi.org/10.1007/978-3-031-23443-9_6>.

</div>

<div id="ref-serraUnsupervisedStratificationPatients2025"
class="csl-entry">

Serra, Dolors, Pau Romero, Paula Franco, Ignacio Bernat, Miguel Lozano,
Ignacio Garcia-Fernandez, David Soto, Antonio Berruezo, Oscar Camara,
and Rafael Sebastian. 2025. “Unsupervised Stratification of Patients
With Myocardial Infarction Based on Imaging and In-Silico Biomarkers.”
*IEEE Transactions on Medical Imaging* 44 (12): 4762–74.
<https://doi.org/10.1109/TMI.2025.3582383>.

</div>

<div id="ref-serraPatientStratificationBased2024" class="csl-entry">

Serra, Dolors, Pau Romero, Miguel Lozano, Ignacio Garcia-Fernandez,
Diego Penela, Antonio Berruezo, Oscar Camara, Miguel Rodrigo, Miriam
Gil, and Rafael Sebastian. 2024. “Patient Stratification Based on Fast
Simulation of Cardiac Electrophysiology on Digital Twins.” In
*Statistical Atlases and Computational Models of the Heart. Regular and
CMRxRecon Challenge Papers*, edited by Oscar Camara, Esther Puyol-Antón,
Maxime Sermesant, Avan Suinesiaputra, Qian Tao, Chengyan Wang, and
Alistair Young, 14507:35–43. Cham: Springer Nature Switzerland.
<https://doi.org/10.1007/978-3-031-52448-6_4>.

</div>

</div>
