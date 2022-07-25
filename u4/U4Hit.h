#pragma once

#include "G4Types.hh"
#include "G4ThreeVector.hh"

/**1
U4Hit : formerly G4OpticksHit
--------------------------------

Type used in G4Opticks interface, NB all Geant4 types no Opticks ones.

Note that Opticks does not use the Geant4 system of units approach, it 
picks a standard set of units suited to optical physics and uses
these unless described otherwise. 

* distances (mm)
* wavelength (nm)
* energy (eV)
* time (ns) 

Reasons for this include that Opticks mostly uses float precision only 
resorting to double precision where that is unavoidable. This is 
contrary to Geant4 which uses double precision everywhere. 
Also Opticks compartmentalizes its dependency on Geant4 headers to 
only a few of its highest level sub-packages.

1**/

struct U4Hit 
{
    G4ThreeVector local_position ; 
    G4ThreeVector global_position ; 
    G4double      time ; 
    G4ThreeVector local_direction ; 
    G4ThreeVector global_direction ; 
    G4double      weight ; 
    G4ThreeVector local_polarization ; 
    G4ThreeVector global_polarization ; 
    G4double      wavelength ; 
    G4int         boundary ;
    G4int         sensorIndex ;
    G4int         nodeIndex ;
    G4int         photonIndex ;
    G4int         flag_mask ; 
    G4int         sensor_identifier ; 
    G4bool        is_cerenkov ; 
    G4bool        is_reemission ; 
};

/**2
U4HitExtra
-------------------

This extra hit information is only filled when ``Opticks::isWayEnabled`` 
is switched on with the ``--way`` option on the embedded Opticks commandline.

``boundary_pos`` ``boundary_time``
   global frame position and time of a photon when it crosses a boundary 
   selected by **geospecific** commandline options.

``origin_time`` 
   initial time of the first photon at generation from the genstep 
   obtained some other particle. This time is obtained immediately 
   after generation before starting the "bounce" loop 

``origin_trackID``
   non-optical parent G4Track::GetTrackID recorded into genstep at ``0*4+1``

2**/

struct U4HitExtra
{
    G4ThreeVector boundary_pos ; 
    G4double      boundary_time ;     

    G4double      origin_time ;     
    G4int         origin_trackID ; 
};

/**3
JUNO geometry example, showing  possible boundary_pos (1) or (2)::


                             |--------- 2230 ----------------|-- 120--|
                             20050                           17820    17700
                          / /                               /         /
                         / /                               /         /
                        / pInnerWater                     /         /
                       / /                               /         /
                      / /                  (0)          /         /
                     pTyvek                  \         pAcylic   /
                    / /                       \       /         /
                   / /                         \     /         pTarget:LS
                  / /                           \   /         /
                 / /                             \ /         /
                / /                              (1)        /
               / /                               / \       /
              / /                               /   \     /
             / /                               /     \   /         
            / /                               /       \ /
           / /                          Wa   /  Ac    (2)             
          / /                               /         / \
         / /                               /         /   \
        / /                               /         /     \        LS    


Geospecific options controlling which boundary_pos and time to record::

   --way --pvname pAcylic  --boundary Water///Acrylic --waymask 3    # (1): gives radius 17820

   --way --pvname pTarget  --boundary Acrylic///LS --waymask 3       # (2): gives radius 17700


Note that when running from GDML the pvname will often have address suffixes, eg::

   --boundary MineralOil///Acrylic --pvname /dd/Geometry/AD/lvSST#pvOIL0xc2415100x3f0b6a0 


Such options are used via GGeo::getSignedBoundary Opticks::getBoundary GGeo::getFirstNodeIndexForPVName Opticks::getPVName
in OGeo::initWayControl to set **way_control** in the OptiX GPU context.

For each boundary encountered in the GPU propagation in oxrap/cu/generate.cu a match with the 
boundary and/or pvname is checked and when found results in the setting of ``boundary_pos`` ``boundary_time``.


3**/



