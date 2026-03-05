#include "OpticsRun.hh"

#include <DD4hep/InstanceCount.h>
#include <DDG4/Factories.h>
#include <DDG4/Geant4Kernel.h>

#include <G4Run.hh>

#include <G4CXOpticks.hh>
#include <SEvt.hh>
#include <SEventConfig.hh>

namespace ddeicopticks
{
//---------------------------------------------------------------------------//
OpticsRun::OpticsRun(dd4hep::sim::Geant4Context* ctxt,
                     std::string const& name)
    : dd4hep::sim::Geant4RunAction(ctxt, name)
{
    dd4hep::InstanceCount::increment(this);
    declareProperty("SaveGeometry", save_geometry_);
}

//---------------------------------------------------------------------------//
OpticsRun::~OpticsRun()
{
    dd4hep::InstanceCount::decrement(this);
}

//---------------------------------------------------------------------------//
void OpticsRun::begin(G4Run const* run)
{
    G4VPhysicalVolume* world = context()->world();
    if (!world)
    {
        except("OpticsRun: world volume is null at begin-of-run");
        return;
    }

    info("Initializing G4CXOpticks geometry (run #%d)", run->GetRunID());
    SEvt::CreateOrReuse(SEvt::EGPU);

    bool hasDevice = SEventConfig::HasDevice();
    info("HasDevice=%s, IntegrationMode=%d", hasDevice ? "YES" : "NO",
         SEventConfig::IntegrationMode());
    G4CXOpticks::SetGeometry(world);

    if (save_geometry_)
    {
        info("Saving Opticks geometry to disk");
        G4CXOpticks::SaveGeometry();
    }

    info("G4CXOpticks geometry initialized successfully");
}

//---------------------------------------------------------------------------//
void OpticsRun::end(G4Run const* run)
{
    info("Finalizing G4CXOpticks (run #%d)", run->GetRunID());
    G4CXOpticks::Finalize();
}

//---------------------------------------------------------------------------//
}  // namespace ddeicopticks

DECLARE_GEANT4ACTION_NS(ddeicopticks, OpticsRun)
