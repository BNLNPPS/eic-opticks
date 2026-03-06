#include "OpticsPhysics.hh"

#include <DD4hep/InstanceCount.h>
#include <DDG4/Factories.h>

#include <G4ParticleTable.hh>
#include <G4ProcessManager.hh>
#include <G4OpticalPhoton.hh>
#include <G4OpAbsorption.hh>
#include <G4OpRayleigh.hh>
#include <G4OpBoundaryProcess.hh>

#include <Local_G4Cerenkov_modified.hh>
#include <Local_DsG4Scintillation.hh>

namespace ddeicopticks
{
//---------------------------------------------------------------------------//
OpticsPhysics::OpticsPhysics(dd4hep::sim::Geant4Context* ctxt,
                             std::string const& name)
    : dd4hep::sim::Geant4PhysicsList(ctxt, name)
{
    dd4hep::InstanceCount::increment(this);
    declareProperty("MaxNumPhotonsPerStep", max_num_photons_per_step_);
    declareProperty("MaxBetaChangePerStep", max_beta_change_per_step_);
    declareProperty("TrackSecondariesFirst", track_secondaries_first_);
    declareProperty("OpticksMode", opticks_mode_);
}

//---------------------------------------------------------------------------//
OpticsPhysics::~OpticsPhysics()
{
    dd4hep::InstanceCount::decrement(this);
}

//---------------------------------------------------------------------------//
void OpticsPhysics::constructProcesses(G4VUserPhysicsList* /* physics */)
{
    info("Constructing eic-opticks genstep-collecting optical processes");

    // Create modified Cerenkov process (collects gensteps instead of photons)
    auto* cerenkov = new Local_G4Cerenkov_modified;
    cerenkov->SetMaxNumPhotonsPerStep(max_num_photons_per_step_);
    cerenkov->SetMaxBetaChangePerStep(max_beta_change_per_step_);
    cerenkov->SetTrackSecondariesFirst(track_secondaries_first_);

    // Create modified Scintillation process (collects gensteps instead of photons)
    auto* scintillation = new Local_DsG4Scintillation(opticks_mode_);
    scintillation->SetTrackSecondariesFirst(track_secondaries_first_);

    // Create standard optical photon transport processes
    auto* absorption = new G4OpAbsorption;
    auto* rayleigh = new G4OpRayleigh;
    auto* boundary = new G4OpBoundaryProcess;

    info("  Cerenkov: MaxPhotons=%d MaxBeta=%.1f TrackFirst=%s",
         max_num_photons_per_step_,
         max_beta_change_per_step_,
         track_secondaries_first_ ? "yes" : "no");
    info("  Scintillation: OpticksMode=%d", opticks_mode_);

    // Ensure G4OpticalPhoton is constructed
    G4OpticalPhoton::OpticalPhotonDefinition();

    // Iterate all particles and register processes
    auto* particleTable = G4ParticleTable::GetParticleTable();
    auto* particleIterator = particleTable->GetIterator();
    particleIterator->reset();

    while ((*particleIterator)())
    {
        G4ParticleDefinition* particle = particleIterator->value();
        G4ProcessManager* pmanager = particle->GetProcessManager();
        G4String particleName = particle->GetParticleName();

        // Cerenkov: applicable to charged particles
        if (cerenkov->IsApplicable(*particle))
        {
            pmanager->AddProcess(cerenkov);
            pmanager->SetProcessOrdering(cerenkov, idxPostStep);
        }

        // Scintillation: applicable to all except opticalphoton
        if (scintillation->IsApplicable(*particle)
            && particleName != "opticalphoton")
        {
            pmanager->AddProcess(scintillation);
            pmanager->SetProcessOrderingToLast(scintillation, idxAtRest);
            pmanager->SetProcessOrderingToLast(scintillation, idxPostStep);
        }

        // Optical photon: transport processes
        if (particleName == "opticalphoton")
        {
            // Scintillation for reemission (must be before absorption)
            pmanager->AddProcess(scintillation);
            pmanager->SetProcessOrderingToLast(scintillation, idxAtRest);
            pmanager->SetProcessOrderingToLast(scintillation, idxPostStep);

            pmanager->AddDiscreteProcess(absorption);
            pmanager->AddDiscreteProcess(rayleigh);
            pmanager->AddDiscreteProcess(boundary);
        }
    }

    info("Optical processes registered for all particles");
}

//---------------------------------------------------------------------------//
}  // namespace ddeicopticks

DECLARE_GEANT4ACTION_NS(ddeicopticks, OpticsPhysics)
