#pragma once

#include <string>
#include <DDG4/Geant4Action.h>
#include <DDG4/Geant4SteppingAction.h>

namespace ddeicopticks
{
//---------------------------------------------------------------------------//
/*!
 * DDG4 stepping action plugin for eic-opticks genstep collection.
 *
 * Intercepts each Geant4 step and checks whether the standard G4Cerenkov
 * or G4Scintillation processes produced photons. If so, extracts the
 * genstep parameters and feeds them to U4::CollectGenstep_*() for
 * subsequent GPU optical photon simulation.
 *
 * This replaces OpticsPhysics by working with standard, unmodified
 * Cerenkov/Scintillation processes instead of Local_G4Cerenkov_modified.
 *
 * Properties:
 *   - Verbose (default: 0) -- verbosity level for genstep collection logging
 */
class OpticsSteppingAction final : public dd4hep::sim::Geant4SteppingAction
{
  public:
    OpticsSteppingAction(dd4hep::sim::Geant4Context* ctxt,
                         std::string const& name);

    void operator()(const G4Step* step, G4SteppingManager* mgr) final;

  protected:
    DDG4_DEFINE_ACTION_CONSTRUCTORS(OpticsSteppingAction);
    ~OpticsSteppingAction() final;

  private:
    int verbose_{0};
};

//---------------------------------------------------------------------------//
}  // namespace ddeicopticks
