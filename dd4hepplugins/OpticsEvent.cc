#include "OpticsEvent.hh"

#include <DD4hep/InstanceCount.h>
#include <DDG4/Factories.h>

#include <G4Event.hh>

#include <G4CXOpticks.hh>
#include <SEvt.hh>
#include <SComp.h>
#include <sphoton.h>
#include <NP.hh>
#include <NPFold.h>
#include <map>

namespace ddeicopticks
{
//---------------------------------------------------------------------------//
OpticsEvent::OpticsEvent(dd4hep::sim::Geant4Context* ctxt,
                         std::string const& name)
    : dd4hep::sim::Geant4EventAction(ctxt, name)
{
    dd4hep::InstanceCount::increment(this);
    declareProperty("Verbose", verbose_);
}

//---------------------------------------------------------------------------//
OpticsEvent::~OpticsEvent()
{
    dd4hep::InstanceCount::decrement(this);
}

//---------------------------------------------------------------------------//
void OpticsEvent::begin(G4Event const* event)
{
    int eventID = event->GetEventID();

    if (verbose_ > 0)
    {
        info("OpticsEvent::begin -- event #%d", eventID);
    }

    SEvt::CreateOrReuse_EGPU();
    SEvt* sev = SEvt::Get_EGPU();
    if (sev)
    {
        sev->beginOfEvent(eventID);
    }
}

//---------------------------------------------------------------------------//
void OpticsEvent::end(G4Event const* event)
{
    int eventID = event->GetEventID();

    G4CXOpticks* gx = G4CXOpticks::Get();
    if (!gx)
    {
        error("OpticsEvent::end -- G4CXOpticks not initialized");
        return;
    }

    SEvt* sev = SEvt::Get_EGPU();
    if (!sev)
    {
        error("OpticsEvent::end -- no EGPU SEvt instance");
        return;
    }

    int64_t num_genstep = sev->getNumGenstepFromGenstep();
    int64_t num_photon = sev->getNumPhotonFromGenstep();

    if (verbose_ > 0 || num_genstep > 0)
    {
        info("Event #%d: %lld gensteps, %lld photons to simulate",
             eventID,
             static_cast<long long>(num_genstep),
             static_cast<long long>(num_photon));
    }

    if (num_genstep > 0)
    {
        info("Event #%d: sev=%p sev->topfold=%p", eventID, (void*)sev, (void*)sev->topfold);
        info("Event #%d: calling gx->simulate, gx=%p", eventID, (void*)gx);
        gx->simulate(eventID, /*reset=*/false);
        SEvt* sev2 = SEvt::Get_EGPU();
        info("Event #%d: simulate returned, sev2=%p sev2->topfold=%p (same=%d)",
             eventID, (void*)sev2, (void*)(sev2?sev2->topfold:nullptr),
             (sev == sev2));

        unsigned num_hit = sev->getNumHit();
        info("Event #%d: %u hits from GPU", eventID, num_hit);

        // Debug: dump topfold contents
        {
            NPFold* tf = sev->topfold ;
            int nsub = tf ? tf->get_num_subfold() : 0;
            int naa = tf ? (int)tf->aa.size() : 0;
            int nkk = tf ? (int)tf->kk.size() : 0;
            info("Event #%d: topfold nsub=%d naa=%d nkk=%d",
                 eventID, nsub, naa, nkk);
            for (int ai = 0; ai < nkk; ai++)
            {
                const char* k = tf->kk[ai].c_str();
                const NP* a = tf->aa[ai];
                info("  [%d] key='%s' shape[0]=%d", ai,
                     k ? k : "?", a ? a->shape[0] : -1);
            }

            // Look for photon array
            const NP* photon = tf->get("photon");
            if (photon)
            {
                int np = photon->shape[0];
                info("Event #%d: found %d photons", eventID, np);
                const sphoton* pp = (const sphoton*)photon->cvalues<float>();
                std::map<unsigned, int> flag_counts;
                std::map<unsigned, int> boundary_counts;
                for (int i = 0; i < np; i++)
                {
                    flag_counts[pp[i].flagmask]++;
                    boundary_counts[pp[i].boundary()]++;
                }
                for (auto& [f, c] : flag_counts)
                    info("  flagmask 0x%08x : %d photons", f, c);
                for (auto& [b, c] : boundary_counts)
                    info("  boundary %u : %d photons", b, c);
            }
            else
            {
                info("Event #%d: no 'photon' key in topfold", eventID);
            }
        }

        // TODO: Convert GPU hits to DD4hep sensitive detector hits.
        // Hits accessible via:
        //   sphoton hit;
        //   sev->getHit(hit, idx);

        sev->endOfEvent(eventID);
        gx->reset(eventID);
    }
    else
    {
        sev->endOfEvent(eventID);
    }
}

//---------------------------------------------------------------------------//
}  // namespace ddeicopticks

DECLARE_GEANT4ACTION_NS(ddeicopticks, OpticsEvent)
