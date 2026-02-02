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

/*
 * No trampoline class needed here.
 *
 * PyGateVActor (in pyGateVActor.cpp) already provides PYBIND11_OVERLOAD
 * trampolines for BeginOfRunActionMasterThread and EndOfRunActionMasterThread.
 *
 * Since GateOpticksActor inherits from GateVActor, and we specify GateVActor
 * as the base class in py::class_, pybind11 uses PyGateVActor's trampolines.
 *
 * This pattern matches other actors like GateKillActor, GateDoseActor, etc.
 */

void init_GateOpticksActor(py::module &m) {
  py::class_<GateOpticksActor, std::unique_ptr<GateOpticksActor, py::nodelete>,
             GateVActor>(m, "GateOpticksActor")
      .def(py::init<py::dict &>())
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
