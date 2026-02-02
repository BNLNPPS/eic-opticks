/* --------------------------------------------------
   Copyright (C): OpenGATE Collaboration
   This software is distributed under the terms
   of the GNU Lesser General  Public Licence (LGPL)
   See LICENSE.md for further details
   -------------------------------------------------- */

#ifndef GateOpticksActor_h
#define GateOpticksActor_h

#include "GateHelpers.h"
#include "GateVActor.h"
#include <pybind11/stl.h>

namespace py = pybind11;

class GateOpticksActor : public GateVActor {

public:
  // Callback function type for Python to process batch results
  using ProcessBatchFunction = std::function<void(GateOpticksActor *)>;

  // Constructor
  explicit GateOpticksActor(py::dict &user_info);
  ~GateOpticksActor() override;

  void InitializeUserInfo(py::dict &user_info) override;
  void InitializeCpp() override;

  // Geant4 action callbacks
  void BeginOfRunAction(const G4Run *run) override;
  void EndOfRunAction(const G4Run *run) override;
  void SteppingAction(G4Step *step) override;
  int EndOfRunActionMasterThread(int run_id) override;

  // Set callback from Python (called after each batch)
  void SetProcessBatchFunction(ProcessBatchFunction &f);

  // Data getters for Python (current batch results)
  unsigned int GetCurrentBatchNumHits() const;
  unsigned int GetCurrentBatchNumPhotons() const;

  // Cumulative getters
  unsigned int GetTotalNumHits() const;
  unsigned int GetTotalNumPhotons() const;
  unsigned int GetTotalNumGensteps() const;
  unsigned int GetNumBatchesProcessed() const;
  // START Only needed for CPU vs GPU validation
  unsigned int GetTotalNumCpuHits() const;
  bool IsCpuMode() const;
  // END Only needed for CPU vs GPU validation

  int GetCurrentRunId() const;

protected:
  void CollectCerenkovGenstep(G4Step *step);
  void CollectScintillationGenstep(G4Step *step);
  void RunGPUSimulationAndProcessBatch();
  void ResetBatch();

  // Configuration
  int fBatchSize;
  std::string fOutputPath;
  bool fSaveToFile;
  // START Only needed for CPU vs GPU validation
  bool fCpuMode;  // If true, track photons on CPU and count hits at boundaries
  std::string fDetectorVolumeName;  // Volume name pattern for detector (for CPU hit counting)
  // END Only needed for CPU vs GPU validation

  // Callback to Python
  ProcessBatchFunction fProcessBatch;

  // For MT, all threads local variables are gathered here
  struct threadLocalT {
    int fCurrentRunId;
    int fNumGenstepsInBatch;      // gensteps in current batch
    int fTotalNumGensteps;        // cumulative across all batches
    int fTotalNumHits;            // cumulative hits (GPU)
    int fTotalNumPhotons;         // cumulative photons
    int fNumBatchesProcessed;
    // Current batch results (set after GPU sim)
    int fCurrentBatchNumHits;
    int fCurrentBatchNumPhotons;
    // START Only needed for CPU vs GPU validation
    int fTotalNumCpuHits;         // optical photons absorbed at detector (CPU mode)
    int fTotalNumCpuPhotons;      // optical photons tracked (CPU mode)
    // END Only needed for CPU vs GPU validation
  };
  G4Cache<threadLocalT> fThreadLocalData;
};

#endif // GateOpticksActor_h
