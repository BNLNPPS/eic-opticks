#pragma once

#include <string>
#include <DDG4/Geant4Action.h>
#include <DDG4/Geant4PhysicsList.h>

class Local_G4Cerenkov_modified;
class Local_DsG4Scintillation;
class G4OpAbsorption;
class G4OpRayleigh;
class G4OpBoundaryProcess;

namespace ddeicopticks
{
//---------------------------------------------------------------------------//
/*!
 * DDG4 physics list plugin for eic-opticks genstep-collecting processes.
 *
 * Replaces DD4hep's standard Geant4CerenkovPhysics, Geant4ScintillationPhysics,
 * and Geant4OpticalPhotonPhysics with eic-opticks modified versions that
 * collect gensteps for GPU optical photon simulation instead of producing
 * optical photons on CPU.
 *
 * Properties:
 *   - MaxNumPhotonsPerStep (default: 10000)
 *   - MaxBetaChangePerStep (default: 10.0)
 *   - TrackSecondariesFirst (default: true)
 *   - OpticksMode (default: 0)
 */
class OpticsPhysics final : public dd4hep::sim::Geant4PhysicsList
{
  public:
    OpticsPhysics(dd4hep::sim::Geant4Context* ctxt, std::string const& name);

    void constructProcesses(G4VUserPhysicsList* physics) final;

  protected:
    DDG4_DEFINE_ACTION_CONSTRUCTORS(OpticsPhysics);
    ~OpticsPhysics() final;

  private:
    int max_num_photons_per_step_{10000};
    double max_beta_change_per_step_{10.0};
    bool track_secondaries_first_{true};
    int opticks_mode_{0};
};

//---------------------------------------------------------------------------//
}  // namespace ddeicopticks
