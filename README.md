# Arritmic3D
A fast Eikonal computational model for electrophysiology simulation (Ventricle version).

The simulator has three versions in separate branches: branch `main` (this branch), with a development version, branch `ventricle`, with the ventricle version, and branch `atria`, with the atria version. Checkout the desired branch before proceeding.

The original version of the solver was developed in the Java environment Processing (https://processing.org/). Now it is being migrated to C++ and provided with a Python interface.

**Warning**: This development branch is not fully functional and has some differences in the diffusion model with respect to the other two branches. Thus, **this branch is not validated and should not be used for research purposes yet**.

This README covers
* [Installation](#installation)
* [Publications](#publications)



## Installation
There is an istall script. Right now, it has two versions, one for users with Python 3.11 and the other for users with Python 3.12. Review the install script before executing it and use it at your own discretion.

The install script will install the prerequisites for compilation, namely the C++ and Python build infrastructure.

For now, we recommend using the solver in a Python virtual environment. Once the build prerequisites are installed, you can run the install_venv.sh script, which will create a virtual env (based on `venv` module) and install the requirements. If you prefer building your own, the required packages are listed in the corresponding requiremente.txt file.


## Publications
Please, cite as:
* Serra, D., Romero, P., Garcia-Fernandez, I., Lozano, M., Liberos, A., Rodrigo, M., ... & Sebastian, R. (2022). An automata-based cardiac electrophysiology simulator to assess arrhythmia inducibility. Mathematics, 10(8), 1293. https://doi.org/10.3390/math10081293

Related research:
* Serra, D., Franco, P., Romero, P., García-Fernández, I., Lozano, M., Soto, D., ... & Sebastian, R. (2022, September). Personalized Fast Electrophysiology Simulations to Evaluate Arrhythmogenicity of Ventricular Slow Conduction Channels. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 56-64). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-23443-9_6
* Serra, D., Franco, P., Romero, P., Romitti, G., García-Fernández, I., Lozano, M., ... & Sebastian, R. (2023, July). Assessment of Risk for Ventricular Tachycardia based on Extensive Electrophysiology Simulations. In 2023 45th Annual International Conference of the IEEE Engineering in Medicine & Biology Society (EMBC) (pp. 1-4). IEEE. https://doi.org/10.1109/EMBC40787.2023.10340169
* Serra, D., Romero, P., Lozano, M., Garcia-Fernandez, I., Penela, D., Berruezo, A., ... & Sebastian, R. (2023, October). Patient Stratification Based on Fast Simulation of Cardiac Electrophysiology on Digital Twins. In International Workshop on Statistical Atlases and Computational Models of the Heart (pp. 35-43). Cham: Springer Nature Switzerland. https://doi.org/10.1007/978-3-031-52448-6_4
* Romitti, G. S., Liberos, A., Termenón-Rivas, M., de Arcaya, J. B. Á., Serra, D., Romero, P., ... & Rodrigo, M. (2025). Implementation of a Cellular Automaton for efficient simulations of atrial arrhythmias. Medical Image Analysis, 103484. https://doi.org/10.1016/j.media.2025.103484