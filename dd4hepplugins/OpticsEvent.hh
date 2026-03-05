#pragma once

#include <string>
#include <DDG4/Geant4Action.h>
#include <DDG4/Geant4EventAction.h>

namespace ddeicopticks
{
//---------------------------------------------------------------------------//
/*!
 * DDG4 action plugin for eic-opticks event-level GPU simulation.
 *
 * At begin-of-event: prepares GPU event buffer (SEvt).
 * At end-of-event: triggers GPU optical photon simulation via
 * G4CXOpticks::simulate(), retrieves hits, and resets for next event.
 *
 * Gensteps are collected automatically during the event by the
 * OpticsSteppingAction plugin, which intercepts standard G4Cerenkov
 * and G4Scintillation processes.
 *
 * Properties:
 *   - Verbose (default: 0) -- verbosity level for hit reporting
 */
class OpticsEvent final : public dd4hep::sim::Geant4EventAction
{
  public:
    OpticsEvent(dd4hep::sim::Geant4Context* ctxt, std::string const& name);

    void begin(G4Event const* event) final;
    void end(G4Event const* event) final;

  protected:
    DDG4_DEFINE_ACTION_CONSTRUCTORS(OpticsEvent);
    ~OpticsEvent() final;

  private:
    int verbose_{0};
};

//---------------------------------------------------------------------------//
}  // namespace ddeicopticks
