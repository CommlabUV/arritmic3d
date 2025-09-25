#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
//#include <pybind11/eigen.h>
#include "../src/cell_event_queue.h"
#include "../src/tissue.h"
#include "../src/action_potential_rc.h"
#include "../src/conduction_velocity.h"

namespace py = pybind11;

PYBIND11_MODULE(tissue_module, m) {
    // Expose the CellType enum
    py::enum_<CellType>(m, "CellType")
        .value("HEALTHY", CellType::HEALTHY)
        .value("CORE", CellType::CORE)
        .value("BORDER_ZONE", CellType::BORDER_ZONE)
        .export_values();

    // Expose the TissueRegion enum
    py::enum_<TissueRegion>(m, "TissueRegion")
        .value("ENDO", TissueRegion::ENDO)
        .value("MID", TissueRegion::MID)
        .value("EPI", TissueRegion::EPI)
        .export_values();

     // Expose the SystemEventType enum
     py::enum_<SystemEventType>(m, "SystemEventType")
          .value("NODE_EVENT", SystemEventType::NODE_EVENT)
          .value("EXT_ACTIVATION", SystemEventType::EXT_ACTIVATION)
          .value("FILE_WRITE", SystemEventType::FILE_WRITE)
          .value("OTHER", SystemEventType::OTHER)
          .value("NO_EVENT", SystemEventType::NO_EVENT)
          .export_values();

    py::class_<CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>>(m, "CardiacTissue")
        .def(py::init<int, int, int, double, double, double>())
        .def("InitPy", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::InitPy,
             py::arg("cell_types"), py::arg("tissue_region"), py::arg("parameters"), py::arg("fiber_orientation") = std::vector<std::vector<float>>({{0.0, 0.0, 0.0}}))
        .def("Reset", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::Reset)
        .def("GetStates", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetStates)
        .def("GetAPD", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetAPD)
        .def("GetCV", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetCV)
        .def("GetDI", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetDI)
        .def("GetLastDI", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetLastDI)
        .def("GetLAT", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetLAT)
        .def("GetLT", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetLT)
        .def("GetBeat", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetBeat)
        .def("GetIndex", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetIndex)
        .def("ExternalActivation", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::ExternalActivation)
        .def("SaveVTK", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::SaveVTK)
        .def("GetTime", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetTime)
        .def("update", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::update,
             py::arg("debug") = 0,
             "Update the tissue state by processing the next event in the queue. Returns the type of event that was processed.")
        .def("SetTimer", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::SetTimer)
        .def("SetSystemEvent", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::SetSystemEvent)
        .def("size", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::size)
        .def("GetSizeX", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetSizeX)
        .def("GetSizeY", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetSizeY)
        .def("GetSizeZ", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetSizeZ)
        .def("GetSensorInfo", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetSensorInfo,
             "Get sensor data collected during the simulation")
        .def("GetSensorDataNames", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetSensorDataNames,
             "Get the names of the sensor data collected during the simulation")
        .def("GetDefaultParameters", &CardiacTissue<ActionPotentialRestCurve, ConductionVelocity>::GetDefaultParameters,
             "Get the default parameters for the tissue nodes");
}
