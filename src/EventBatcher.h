#pragma once
/**
EventBatcher — CPU-side bookkeeping for multi-event GPU batching
=================================================================

Maps GPU hits back to their originating G4 event by tracking which
gensteps belong to which event. No GPU struct changes, no bandwidth
impact beyond a single seed buffer download (~4 bytes per photon).

Thread-safe: uses a mutex to protect genstep registration, compatible
with G4TaskRunManager multithreaded event processing.

Usage:
  1. Call recordGenstep(eventID) from SteppingAction under mutex
     each time a genstep is collected
  2. After GPU simulate(), use unpackHits() to sort hits by event ID

The mapping chain:
  hit.index -> photon_idx -> seed[photon_idx] -> genstep_idx -> eventID
**/

#include <vector>
#include <map>
#include <mutex>
#include <cassert>
#include <string>

#include "sysrap/sphoton.h"
#include "sysrap/SEvt.hh"
#include "sysrap/NP.hh"

struct EventBatcher
{
    std::vector<int> genstep_to_eventid;
    std::mutex mtx;

    // Call from SteppingAction each time a genstep is collected.
    // Must be called under the same mutex as genstep collection.
    void recordGenstep(int eventID)
    {
        std::lock_guard<std::mutex> lock(mtx);
        genstep_to_eventid.push_back(eventID);
    }

    int getEventID(int genstep_idx) const
    {
        assert(genstep_idx >= 0 && genstep_idx < static_cast<int>(genstep_to_eventid.size()));
        return genstep_to_eventid[genstep_idx];
    }

    size_t numGensteps() const
    {
        return genstep_to_eventid.size();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mtx);
        genstep_to_eventid.clear();
    }

    // Unpack hits into per-event groups using the seed buffer
    // seed: int array of size num_photon, seed[photon_idx] = genstep_idx
    // Returns map of eventID -> vector of sphoton hits
    std::map<int, std::vector<sphoton>> unpackHits(SEvt* gpu_sev, const int* seed) const
    {
        std::map<int, std::vector<sphoton>> hits_by_event;

        unsigned num_hits = gpu_sev->GetNumHit(0);

        for (unsigned idx = 0; idx < num_hits; idx++)
        {
            sphoton hit;
            gpu_sev->getHit(hit, idx);

            unsigned photon_idx = hit.index;
            int genstep_idx = seed[photon_idx];
            int evtid = getEventID(genstep_idx);

            hits_by_event[evtid].push_back(hit);
        }

        return hits_by_event;
    }
};
