/* --------------------------------------------------
   Copyright (C): OpenGATE Collaboration
   This software is distributed under the terms
   of the GNU Lesser General  Public Licence (LGPL)
   See LICENSE.md for further details
   -------------------------------------------------- */

#include "GateOpticksActor.h"
#include "GateHelpersDict.h"
#include "G4RunManager.hh"
#include "G4Threading.hh"
#include "G4OpticalPhoton.hh"
#include "G4EventManager.hh"
#include "G4TransportationManager.hh"
// START Only needed for CPU vs GPU validation
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
// END Only needed for CPU vs GPU validation
#include <iostream>
#include <chrono>

#ifdef GATE_USE_OPTICKS
#include "G4Cerenkov.hh"
#include "G4Scintillation.hh"
#include "G4AutoLock.hh"
#include "G4CXOpticks.hh"
#include "SEvt.hh"
#include "SEventConfig.hh"
#include "U4.hh"
#include "QSim.hh"
#include "sphoton.h"
#include "OpticksPhoton.h"
#include "NP.hh"
#include "SComp.h"
namespace { G4Mutex genstep_mutex = G4MUTEX_INITIALIZER; }
#endif

GateOpticksActor::GateOpticksActor(py::dict &user_info)
    : GateVActor(user_info, true) {
  fActions.insert("SteppingAction");
  fActions.insert("BeginOfRunAction");
  fActions.insert("EndOfRunAction");
  fBatchSize = 0;
  fSaveToFile = true;
  // START Only needed for CPU vs GPU validation
  fCpuMode = false;
  fDetectorVolumeName = "detector";
  // END Only needed for CPU vs GPU validation
}

GateOpticksActor::~GateOpticksActor() = default;

void GateOpticksActor::InitializeUserInfo(py::dict &user_info) {
  GateVActor::InitializeUserInfo(user_info);
  fBatchSize = DictGetInt(user_info, "batch_size");
  fOutputPath = DictGetStr(user_info, "output_path");
  fSaveToFile = DictGetBool(user_info, "save_to_file");
  // START Only needed for CPU vs GPU validation
  fCpuMode = DictGetBool(user_info, "cpu_mode");
  fDetectorVolumeName = DictGetStr(user_info, "detector_volume_name");
  // END Only needed for CPU vs GPU validation
}

void GateOpticksActor::InitializeCpp() {
  GateVActor::InitializeCpp();
#ifdef GATE_USE_OPTICKS
  G4CXOpticks *gx = G4CXOpticks::Get();
  if (!gx) {
    G4VPhysicalVolume *world = G4TransportationManager::GetTransportationManager()
        ->GetNavigatorForTracking()->GetWorldVolume();
    if (world) {
      SEvt::Create_EGPU();

      // Set up geometry and GPU context
      gx = G4CXOpticks::SetGeometry(world);

      if (gx && gx->cx) {
        // Verify QSim is available (required for simulation)
        QSim* qs = QSim::Get();
        if (qs) {
          std::cout << "GateOpticksActor: GPU simulation initialized for world: "
                    << world->GetName() << std::endl;
        } else {
          std::cerr << "GateOpticksActor: ERROR - QSim not available!" << std::endl;
        }
      } else {
        std::cerr << "GateOpticksActor: ERROR - GPU context failed to initialize!" << std::endl;
      }
    }
  }
#endif
}

void GateOpticksActor::SetProcessBatchFunction(ProcessBatchFunction &f) {
  fProcessBatch = f;
}

void GateOpticksActor::BeginOfRunAction(const G4Run *run) {
  auto &l = fThreadLocalData.Get();
  l.fCurrentRunId = run->GetRunID();
  l.fNumGenstepsInBatch = 0;
  l.fTotalNumGensteps = 0;
  l.fTotalNumHits = 0;
  l.fTotalNumPhotons = 0;
  l.fNumBatchesProcessed = 0;
  l.fCurrentBatchNumHits = 0;
  l.fCurrentBatchNumPhotons = 0;
  // START Only needed for CPU vs GPU validation
  l.fTotalNumCpuHits = 0;
  l.fTotalNumCpuPhotons = 0;
  // END Only needed for CPU vs GPU validation
}

void GateOpticksActor::SteppingAction(G4Step *step) {
  if (!step) return;
  G4Track* track = step->GetTrack();
  if (!track) return;

  auto &l = fThreadLocalData.Get();

  // Handle optical photons
  if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
    // START Only needed for CPU vs GPU validation
    if (fCpuMode) {
      // CPU mode: track photons and count hits at detector boundaries
      l.fTotalNumCpuPhotons++;

      // Check if photon was absorbed at a boundary (Detection)
      G4StepPoint* postPoint = step->GetPostStepPoint();
      if (postPoint->GetStepStatus() == fGeomBoundary) {
        // Get the boundary process to check what happened
        G4ProcessManager* pm = track->GetDefinition()->GetProcessManager();
        G4int nProcesses = pm->GetPostStepProcessVector()->entries();
        G4ProcessVector* procVec = pm->GetPostStepProcessVector(typeDoIt);

        for (G4int i = 0; i < nProcesses; i++) {
          G4VProcess* proc = (*procVec)[i];
          if (proc && proc->GetProcessName() == "OpBoundary") {
            G4OpBoundaryProcess* boundary = dynamic_cast<G4OpBoundaryProcess*>(proc);
            if (boundary) {
              G4OpBoundaryProcessStatus status = boundary->GetStatus();
              // Detection means photon was absorbed based on EFFICIENCY
              if (status == Detection) {
                l.fTotalNumCpuHits++;
              }
            }
            break;
          }
        }
      }
      // In CPU mode, let Geant4 continue tracking the photon
      return;
    }
    // END Only needed for CPU vs GPU validation
    {
#ifdef GATE_USE_OPTICKS
      // GPU mode: kill optical photons - they will be simulated on GPU
      track->SetTrackStatus(fStopAndKill);
      return;
#endif
    }
  }

#ifdef GATE_USE_OPTICKS
  // START Only needed for CPU vs GPU validation
  if (fCpuMode) return;  // Skip genstep collection in CPU mode
  // END Only needed for CPU vs GPU validation

  G4SteppingManager *fpSteppingManager =
      G4EventManager::GetEventManager()->GetTrackingManager()->GetSteppingManager();
  if (!fpSteppingManager) return;

  if (fpSteppingManager->GetfStepStatus() == fAtRestDoItProc) return;

  // Check for Cerenkov/Scintillation in process vector
  G4ProcessVector *procPost = fpSteppingManager->GetfPostStepDoItVector();
  size_t MAXofPostStepLoops = fpSteppingManager->GetMAXofPostStepLoops();

  for (size_t i = 0; i < MAXofPostStepLoops; i++) {
    if (!(*procPost)[i]) continue;
    G4String procName = (*procPost)[i]->GetProcessName();

    if (procName == "Cerenkov") {
      G4Cerenkov* cer = dynamic_cast<G4Cerenkov*>((*procPost)[i]);
      if (cer && cer->GetNumPhotons() > 0) {
        CollectCerenkovGenstep(step);
      }
    } else if (procName == "Scintillation") {
      G4Scintillation* scint = dynamic_cast<G4Scintillation*>((*procPost)[i]);
      if (scint && scint->GetNumPhotons() > 0) {
        CollectScintillationGenstep(step);
      }
    }
  }

  // Check batch size
  if (fBatchSize > 0 && l.fNumGenstepsInBatch >= fBatchSize) {
    RunGPUSimulationAndProcessBatch();
  }
#else
  (void)step;
#endif
}

void GateOpticksActor::CollectCerenkovGenstep(G4Step *step) {
#ifdef GATE_USE_OPTICKS
  G4Track *aTrack = step->GetTrack();
  const G4DynamicParticle *aParticle = aTrack->GetDynamicParticle();
  G4double charge = aParticle->GetDefinition()->GetPDGCharge();
  const G4Material *aMaterial = aTrack->GetMaterial();
  G4MaterialPropertiesTable *MPT = aMaterial->GetMaterialPropertiesTable();
  if (!MPT) return;

  G4MaterialPropertyVector *Rindex = MPT->GetProperty(kRINDEX);
  if (!Rindex || Rindex->GetVectorLength() == 0) return;

  // Get Cerenkov process
  G4SteppingManager *fpSteppingManager =
      G4EventManager::GetEventManager()->GetTrackingManager()->GetSteppingManager();
  G4ProcessVector *procPost = fpSteppingManager->GetfPostStepDoItVector();
  G4Cerenkov *proc = nullptr;
  for (size_t i = 0; i < fpSteppingManager->GetMAXofPostStepLoops(); i++) {
    if ((*procPost)[i] && (*procPost)[i]->GetProcessName() == "Cerenkov") {
      proc = (G4Cerenkov *)(*procPost)[i];
      break;
    }
  }
  if (!proc) return;

  int fNumPhotons = proc->GetNumPhotons();
  if (fNumPhotons <= 0) return;

  G4double Pmin = Rindex->Energy(0);
  G4double Pmax = Rindex->GetMaxEnergy();
  G4double nMax = Rindex->GetMaxValue();
  G4double beta1 = step->GetPreStepPoint()->GetBeta();
  G4double beta2 = step->GetPostStepPoint()->GetBeta();
  G4double beta = (beta1 + beta2) * 0.5;
  G4double BetaInverse = 1. / beta;
  G4double maxCos = BetaInverse / nMax;
  G4double maxSin2 = (1.0 - maxCos) * (1.0 + maxCos);
  G4double MeanNumberOfPhotons1 = proc->GetAverageNumberOfPhotons(charge, beta1, aMaterial, Rindex);
  G4double MeanNumberOfPhotons2 = proc->GetAverageNumberOfPhotons(charge, beta2, aMaterial, Rindex);

  // Thread-safe genstep collection
  G4AutoLock lock(&genstep_mutex);
  U4::CollectGenstep_G4Cerenkov_modified(aTrack, step, fNumPhotons, BetaInverse,
      Pmin, Pmax, maxCos, maxSin2, MeanNumberOfPhotons1, MeanNumberOfPhotons2);

  auto &l = fThreadLocalData.Get();
  l.fNumGenstepsInBatch++;
  l.fTotalNumGensteps++;
#else
  (void)step;
#endif
}

void GateOpticksActor::CollectScintillationGenstep(G4Step *step) {
#ifdef GATE_USE_OPTICKS
  G4Track *aTrack = step->GetTrack();
  const G4Material *aMaterial = aTrack->GetMaterial();
  G4MaterialPropertiesTable *MPT = aMaterial->GetMaterialPropertiesTable();
  if (!MPT) return;

  G4SteppingManager *fpSteppingManager =
      G4EventManager::GetEventManager()->GetTrackingManager()->GetSteppingManager();
  G4ProcessVector *procPost = fpSteppingManager->GetfPostStepDoItVector();
  G4Scintillation *proc = nullptr;
  for (size_t i = 0; i < fpSteppingManager->GetMAXofPostStepLoops(); i++) {
    if ((*procPost)[i] && (*procPost)[i]->GetProcessName() == "Scintillation") {
      proc = (G4Scintillation *)(*procPost)[i];
      break;
    }
  }
  if (!proc) return;

  int fNumPhotons = proc->GetNumPhotons();
  if (fNumPhotons <= 0) return;

  G4double ScintillationTime = 0;

  // Thread-safe genstep collection
  G4AutoLock lock(&genstep_mutex);
  U4::CollectGenstep_DsG4Scintillation_r4695(aTrack, step, fNumPhotons, 0, ScintillationTime);

  auto &l = fThreadLocalData.Get();
  l.fNumGenstepsInBatch++;
  l.fTotalNumGensteps++;
#else
  (void)step;
#endif
}

void GateOpticksActor::RunGPUSimulationAndProcessBatch() {
#ifdef GATE_USE_OPTICKS
  // Only run on master thread
  if (!G4Threading::IsMasterThread()) return;

  auto &l = fThreadLocalData.Get();
  if (l.fNumGenstepsInBatch == 0) return;

  G4CXOpticks *gx = G4CXOpticks::Get();
  if (!gx) return;

  // Time the GPU simulation
  auto gpu_start = std::chrono::high_resolution_clock::now();

  gx->simulate(0, false);
  cudaDeviceSynchronize();

  auto gpu_end = std::chrono::high_resolution_clock::now();
  auto gpu_duration = std::chrono::duration_cast<std::chrono::milliseconds>(gpu_end - gpu_start);

  std::cout << "GPU_SIMULATE_TIME_MS: " << gpu_duration.count()
            << " gensteps: " << l.fNumGenstepsInBatch << std::endl;

  // Get results
  SEvt *sev = SEvt::Get_EGPU();
  if (sev) {
    l.fCurrentBatchNumPhotons = sev->getNumPhotonCollected();
    l.fCurrentBatchNumHits = sev->getNumHit();

    l.fTotalNumHits += l.fCurrentBatchNumHits;
    l.fTotalNumPhotons += l.fCurrentBatchNumPhotons;
  }

  l.fNumBatchesProcessed++;

  if (fProcessBatch) {
    fProcessBatch(this);
  }

  if (fSaveToFile && sev) {
    // Gather photon data from GPU
    NP* photon_arr = sev->gatherComponent(SCOMP_PHOTON);
    if (photon_arr && photon_arr->shape.size() > 0 && photon_arr->shape[0] > 0) {
      std::string photonPath = fOutputPath + "/photon_batch_" + std::to_string(l.fNumBatchesProcessed) + ".npy";
      photon_arr->save(photonPath.c_str());
      std::cout << "SAVED_PHOTON_DATA: " << photonPath << " shape=(" << photon_arr->shape[0] << "," << photon_arr->shape[1] << "," << photon_arr->shape[2] << ")" << std::endl;
      delete photon_arr;
    } else {
      std::cout << "NO_PHOTON_DATA_TO_SAVE" << std::endl;
    }
  }

  ResetBatch();
#endif
}

void GateOpticksActor::ResetBatch() {
#ifdef GATE_USE_OPTICKS
  auto &l = fThreadLocalData.Get();
  l.fNumGenstepsInBatch = 0;
  l.fCurrentBatchNumHits = 0;
  l.fCurrentBatchNumPhotons = 0;

  SEvt *sev = SEvt::Get_EGPU();
  if (sev) {
    sev->clear_genstep();
    sev->clear_output();
  }
#endif
}

void GateOpticksActor::EndOfRunAction(const G4Run * /*run*/) {
#ifdef GATE_USE_OPTICKS
  auto &l = fThreadLocalData.Get();
  if (l.fNumGenstepsInBatch > 0) {
    RunGPUSimulationAndProcessBatch();
  }
#endif
}

int GateOpticksActor::EndOfRunActionMasterThread(int run_id) {
  (void)run_id;
  return 0;
}

unsigned int GateOpticksActor::GetCurrentBatchNumHits() const {
  return fThreadLocalData.Get().fCurrentBatchNumHits;
}

unsigned int GateOpticksActor::GetCurrentBatchNumPhotons() const {
  return fThreadLocalData.Get().fCurrentBatchNumPhotons;
}

unsigned int GateOpticksActor::GetTotalNumHits() const {
  return fThreadLocalData.Get().fTotalNumHits;
}

unsigned int GateOpticksActor::GetTotalNumPhotons() const {
  return fThreadLocalData.Get().fTotalNumPhotons;
}

unsigned int GateOpticksActor::GetTotalNumGensteps() const {
  return fThreadLocalData.Get().fTotalNumGensteps;
}

unsigned int GateOpticksActor::GetNumBatchesProcessed() const {
  return fThreadLocalData.Get().fNumBatchesProcessed;
}

int GateOpticksActor::GetCurrentRunId() const {
  return fThreadLocalData.Get().fCurrentRunId;
}

// START Only needed for CPU vs GPU validation
unsigned int GateOpticksActor::GetTotalNumCpuHits() const {
  return fThreadLocalData.Get().fTotalNumCpuHits;
}

bool GateOpticksActor::IsCpuMode() const {
  return fCpuMode;
}
// END Only needed for CPU vs GPU validation
