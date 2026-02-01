/* --------------------------------------------------
   Copyright (C): OpenGATE Collaboration
   This software is distributed under the terms
   of the GNU Lesser General  Public Licence (LGPL)
   See LICENSE.md for further details
   -------------------------------------------------- */

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include "GateOpticksActor.h"

class PyGateOpticksActor : public GateOpticksActor {
public:
  // Inherit the constructors
  using GateOpticksActor::GateOpticksActor;

  void BeginOfRunActionMasterThread(int run_id) override {
    PYBIND11_OVERLOAD(void, GateOpticksActor, BeginOfRunActionMasterThread, run_id);
  }

  int EndOfRunActionMasterThread(int run_id) override {
    PYBIND11_OVERLOAD(int, GateOpticksActor, EndOfRunActionMasterThread, run_id);
  }
};

void init_GateOpticksActor(py::module &m) {
  py::class_<GateOpticksActor, PyGateOpticksActor,
             std::unique_ptr<GateOpticksActor, py::nodelete>, GateVActor>(
      m, "GateOpticksActor")
      .def(py::init<py::dict &>())
      .def("BeginOfRunActionMasterThread",
           &GateOpticksActor::BeginOfRunActionMasterThread)
      .def("EndOfRunActionMasterThread",
           &GateOpticksActor::EndOfRunActionMasterThread)
      .def("SetProcessBatchFunction", &GateOpticksActor::SetProcessBatchFunction)
      // Current batch getters
      .def("GetCurrentBatchNumHits", &GateOpticksActor::GetCurrentBatchNumHits)
      .def("GetCurrentBatchNumPhotons", &GateOpticksActor::GetCurrentBatchNumPhotons)
      // Cumulative getters
      .def("GetTotalNumHits", &GateOpticksActor::GetTotalNumHits)
      .def("GetTotalNumPhotons", &GateOpticksActor::GetTotalNumPhotons)
      .def("GetTotalNumGensteps", &GateOpticksActor::GetTotalNumGensteps)
      .def("GetNumBatchesProcessed", &GateOpticksActor::GetNumBatchesProcessed)
      .def("GetCurrentRunId", &GateOpticksActor::GetCurrentRunId);
}
