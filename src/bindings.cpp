#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
//#include <pybind11/eigen.h>
#include "../src/cell_event_queue.h"
#include "../src/tissue.h"
#include "../src/action_potential_rs.h"
#include "../src/conduction_velocity.h"

#ifndef MODULE_NAME
#define MODULE_NAME _core
#endif

namespace py = pybind11;

PYBIND11_MODULE(MODULE_NAME, m) {
    // Define aliases for the template parameters
    using T_AP = ActionPotentialRestSurface;
    using T_CV = ConductionVelocity;

    // Expose the SystemEventType enum
    py::enum_<SystemEventType>(m, "SystemEventType")
        .value("NODE_EVENT", SystemEventType::NODE_EVENT)
        .value("EXT_ACTIVATION", SystemEventType::EXT_ACTIVATION)
        .value("FILE_WRITE", SystemEventType::FILE_WRITE)
        .value("OTHER", SystemEventType::OTHER)
        .value("NO_EVENT", SystemEventType::NO_EVENT)
        .export_values();

    py::class_<CardiacTissue<T_AP, T_CV>>(m, "CardiacTissue")
        .def(py::init<int, int, int, double, double, double>())
        .def("InitModels", &CardiacTissue<T_AP, T_CV>::InitModels,
             py::arg("fileAP"), py::arg("fileCV"))
        .def("InitPy", &CardiacTissue<T_AP, T_CV>::InitPy,
             py::arg("cell_types"), py::arg("parameters"), py::arg("fiber_orientation") = std::vector<std::vector<float>>({{0.0, 0.0, 0.0}}))
        .def("ChangeParameters", &CardiacTissue<T_AP, T_CV>::ChangeParameters)
        //.def("Reset", &CardiacTissue<T_AP, T_CV>::Reset)
        .def("GetStates", &CardiacTissue<T_AP, T_CV>::GetStates)
        .def("GetAPD", &CardiacTissue<T_AP, T_CV>::GetAPD)
        .def("GetAP", &CardiacTissue<T_AP, T_CV>::GetAP)
        .def("GetCV", &CardiacTissue<T_AP, T_CV>::GetCV)
        .def("GetDI", &CardiacTissue<T_AP, T_CV>::GetDI)
        .def("GetLastDI", &CardiacTissue<T_AP, T_CV>::GetLastDI)
        .def("GetLAT", &CardiacTissue<T_AP, T_CV>::GetLAT)
        .def("GetLife", &CardiacTissue<T_AP, T_CV>::GetLife)
        .def("GetBeat", &CardiacTissue<T_AP, T_CV>::GetBeat)
        .def("GetAPDVariation", &CardiacTissue<T_AP, T_CV>::GetAPDVariation)
        .def("GetIndex", &CardiacTissue<T_AP, T_CV>::GetIndex)
        .def("ExternalActivation", &CardiacTissue<T_AP, T_CV>::ExternalActivation)
        .def("SaveVTK", &CardiacTissue<T_AP, T_CV>::SaveVTK)
        .def("GetTime", &CardiacTissue<T_AP, T_CV>::GetTime)
        .def("update", &CardiacTissue<T_AP, T_CV>::update,
             py::arg("debug") = 0,
             "Update the tissue state by processing the next event in the queue. Returns the type of event that was processed.")
        .def("SetTimer", &CardiacTissue<T_AP, T_CV>::SetTimer)
        .def("SetSystemEvent", &CardiacTissue<T_AP, T_CV>::SetSystemEvent)
        .def("size", &CardiacTissue<T_AP, T_CV>::size)
        .def("GetNumLiveNodes", &CardiacTissue<T_AP, T_CV>::GetNumLiveNodes)
        .def("GetSizeX", &CardiacTissue<T_AP, T_CV>::GetSizeX)
        .def("GetSizeY", &CardiacTissue<T_AP, T_CV>::GetSizeY)
        .def("GetSizeZ", &CardiacTissue<T_AP, T_CV>::GetSizeZ)
        .def("GetSensorInfo", &CardiacTissue<T_AP, T_CV>::GetSensorInfo,
             "Get sensor data collected during the simulation")
        .def("GetSensorDataNames", &CardiacTissue<T_AP, T_CV>::GetSensorDataNames,
             "Get the names of the sensor data collected during the simulation")
        .def("GetDefaultParameters", &CardiacTissue<T_AP, T_CV>::GetDefaultParameters,
             "Get the default parameters for the tissue nodes")
        .def("GetAPDMeanVariation", &CardiacTissue<T_AP, T_CV>::GetAPDMeanVariation,
             "Get the mean APD variation due to restitution curves (without electrotonic effect) since the last call to ResetVariations")
        .def("ResetVariations", &CardiacTissue<T_AP, T_CV>::ResetVariations,
             "Reset the accumulated APD and CV variations to zero")
        .def("SaveState", &CardiacTissue<T_AP, T_CV>::SaveState,
             "Save the current state of the tissue to a binary file")
        .def("LoadState", &CardiacTissue<T_AP, T_CV>::LoadState,
             "Load the state of the tissue from a binary file")
        .def("SetInitialAPD", &CardiacTissue<T_AP, T_CV>::SetInitialAPD);

}

