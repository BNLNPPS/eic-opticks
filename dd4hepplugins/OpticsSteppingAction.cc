#include "OpticsSteppingAction.hh"

#include <DD4hep/InstanceCount.h>
#include <DDG4/Factories.h>

#include <G4AutoLock.hh>
#include <G4Cerenkov.hh>
#include <G4Scintillation.hh>
#include <G4OpticalPhoton.hh>
#include <G4SteppingManager.hh>
#include <G4ProcessVector.hh>
#include <G4Track.hh>
#include <G4Step.hh>
#include <G4Material.hh>
#include <G4MaterialPropertiesTable.hh>
#include <G4MaterialPropertyVector.hh>
#include <G4MaterialPropertiesIndex.hh>

#include <U4.hh>

namespace
{
G4Mutex genstep_mutex = G4MUTEX_INITIALIZER;
}

namespace ddeicopticks
{
//---------------------------------------------------------------------------//
OpticsSteppingAction::OpticsSteppingAction(
    dd4hep::sim::Geant4Context* ctxt,
    std::string const& name)
    : dd4hep::sim::Geant4SteppingAction(ctxt, name)
{
    dd4hep::InstanceCount::increment(this);
    declareProperty("Verbose", verbose_);
}

//---------------------------------------------------------------------------//
OpticsSteppingAction::~OpticsSteppingAction()
{
    dd4hep::InstanceCount::decrement(this);
}

//---------------------------------------------------------------------------//
void OpticsSteppingAction::operator()(const G4Step* step,
                                       G4SteppingManager* mgr)
{
    // Skip optical photons -- only collect gensteps from charged particles
    if (step->GetTrack()->GetDefinition()
        == G4OpticalPhoton::OpticalPhotonDefinition())
    {
        return;
    }

    // Only process PostStepDoIt steps
    G4StepStatus stepStatus = mgr->GetfStepStatus();
    if (stepStatus == fAtRestDoItProc)
    {
        return;
    }

    // Iterate over all post-step processes to find Cerenkov/Scintillation
    G4ProcessVector* procPost = mgr->GetfPostStepDoItVector();
    size_t maxLoops = mgr->GetMAXofPostStepLoops();

    for (size_t i = 0; i < maxLoops; ++i)
    {
        G4VProcess* proc = (*procPost)[i];
        if (!proc)
        {
            continue;
        }

        G4String const& procName = proc->GetProcessName();

        // --- Cerenkov genstep collection ---
        // DD4hep Geant4CerenkovPhysics names the process after the instance
        // (e.g. "CerenkovPhys"), so use dynamic_cast instead of name matching
        if (auto* cerenkov_check = dynamic_cast<G4Cerenkov*>(proc))
        {
            G4int numPhotons = cerenkov_check->GetNumPhotons();
            if (numPhotons <= 0)
            {
                continue;
            }

            G4Track const* track = step->GetTrack();
            G4DynamicParticle const* particle = track->GetDynamicParticle();
            G4double charge = particle->GetDefinition()->GetPDGCharge();
            G4Material const* material = track->GetMaterial();
            G4MaterialPropertiesTable* mpt =
                material->GetMaterialPropertiesTable();

            if (!mpt)
            {
                continue;
            }

            G4MaterialPropertyVector* Rindex =
                mpt->GetProperty(kRINDEX);
            if (!Rindex || Rindex->GetVectorLength() == 0)
            {
                if (verbose_ > 1)
                {
                    warning("Material '%s' has no RINDEX -- skipping "
                            "Cerenkov genstep",
                            material->GetName().c_str());
                }
                continue;
            }

            G4double Pmin = Rindex->Energy(0);
            G4double Pmax = Rindex->GetMaxEnergy();
            G4double nMax = Rindex->GetMaxValue();
            G4double beta1 = step->GetPreStepPoint()->GetBeta();
            G4double beta2 = step->GetPostStepPoint()->GetBeta();
            G4double beta = (beta1 + beta2) * 0.5;
            G4double BetaInverse = 1.0 / beta;
            G4double maxCos = BetaInverse / nMax;
            G4double maxSin2 = (1.0 - maxCos) * (1.0 + maxCos);
            G4double meanPhotons1 =
                cerenkov_check->GetAverageNumberOfPhotons(
                    charge, beta1, material, Rindex);
            G4double meanPhotons2 =
                cerenkov_check->GetAverageNumberOfPhotons(
                    charge, beta2, material, Rindex);

            G4AutoLock lock(&genstep_mutex);
            U4::CollectGenstep_G4Cerenkov_modified(
                track, step, numPhotons, BetaInverse,
                Pmin, Pmax, maxCos, maxSin2,
                meanPhotons1, meanPhotons2);

            if (verbose_ > 0)
            {
                info("Cerenkov genstep: %d photons, beta=%.6f, "
                     "material=%s",
                     numPhotons, beta,
                     material->GetName().c_str());
            }
        }

        // --- Scintillation genstep collection ---
        if (auto* scintillation = dynamic_cast<G4Scintillation*>(proc))
        {
            G4int numPhotons = scintillation->GetNumPhotons();
            if (numPhotons <= 0)
            {
                continue;
            }

            G4Track const* track = step->GetTrack();
            G4Material const* material = track->GetMaterial();
            G4MaterialPropertiesTable* mpt =
                material->GetMaterialPropertiesTable();

            if (!mpt
                || !mpt->ConstPropertyExists(
                       kSCINTILLATIONTIMECONSTANT1))
            {
                if (verbose_ > 1)
                {
                    warning("Material '%s' has no "
                            "SCINTILLATIONTIMECONSTANT1 -- skipping "
                            "Scintillation genstep",
                            material->GetName().c_str());
                }
                continue;
            }

            G4double scintTime =
                mpt->GetConstProperty(kSCINTILLATIONTIMECONSTANT1);

            G4AutoLock lock(&genstep_mutex);
            U4::CollectGenstep_DsG4Scintillation_r4695(
                track, step, numPhotons,
                /*scnt=*/1, scintTime);

            if (verbose_ > 0)
            {
                info("Scintillation genstep: %d photons, material=%s",
                     numPhotons,
                     material->GetName().c_str());
            }
        }
    }
}

//---------------------------------------------------------------------------//
}  // namespace ddeicopticks

DECLARE_GEANT4ACTION_NS(ddeicopticks, OpticsSteppingAction)
