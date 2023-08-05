#pragma once

#include <vector>
#include <iostream>

#include "sstr.h"
#include "U4SensorIdentifier.h"
#include "G4PVPlacement.hh"

struct U4SensorIdentifierDefault : public U4SensorIdentifier 
{
    int getGlobalIdentity(const G4VPhysicalVolume* pv ) const ; 
    int getInstanceIdentity(const G4VPhysicalVolume* instance_outer_pv ) const ; 
    static void FindSD_r( std::vector<const G4VPhysicalVolume*>& sdpv , const G4VPhysicalVolume* pv, int depth );  
    static bool IsInterestingCopyNo( int copyno ); 
}; 

/**
U4SensorIdentifierDefault::getGlobalIdentity
---------------------------------------------

Canonically invoked from U4Tree::identifySensitiveGlobals

Currently a kludge using hardcoded pvn prefix.
This is because sensors within the global remainder 
is only relevant to test geometries for JUNO. 

Would be better to construct a boundary name and match that 
against a list of sensor boundaries (see GBndLib__SENSOR_BOUNDARY_LIST)

HMM: that is not easy without having the parent pv also 

**/

inline int U4SensorIdentifierDefault::getGlobalIdentity( const G4VPhysicalVolume* pv) const 
{
    const char* pvn = pv->GetName().c_str() ;
    int id = sstr::StartsWith(pvn, "nnvt_") ? 0 : -1 ;  

    std::cout 
        << "U4SensorIdentifierDefault::getGlobalIdentity " 
        << " id " << id 
        << " pvn " << pvn 
        << std::endl
        ;   

    return id ;  
}


/**
U4SensorIdentifierDefault::getInstanceIdentity
---------------------------------------------------

Canonically used from U4Tree::identifySensitiveInstances

The argument *instance_outer_pv* is recursively traversed


**/


inline int U4SensorIdentifierDefault::getInstanceIdentity( const G4VPhysicalVolume* instance_outer_pv ) const 
{
    const char* pvn = instance_outer_pv ? instance_outer_pv->GetName().c_str() : "-" ; 
    bool has_PMT_pvn = strstr(pvn, "PMT") != nullptr  ;  

    const G4PVPlacement* pvp = dynamic_cast<const G4PVPlacement*>(instance_outer_pv) ;
    int copyno = pvp ? pvp->GetCopyNo() : -1 ;

    std::vector<const G4VPhysicalVolume*> sdpv ; 
    FindSD_r(sdpv, instance_outer_pv, 0 );  

    unsigned num_sd = sdpv.size() ; 
    bool is_sensor = num_sd > 0 && has_PMT_pvn  ; 

    //bool is_interesting_copyno = IsInterestingCopyNo(copyno) ; 
    //bool dump = is_sensor && is_interesting_copyno ; 
    bool dump = false ; 
    //bool dump = true ;
    //bool dump = num_sd > 0 ; 
 
    if(dump) std::cout 
        << "U4SensorIdentifierDefault::getIdentity" 
        << " copyno " << copyno
        << " num_sd " << num_sd
        << " is_sensor " << is_sensor
        << " pvn " << ( pvn ? pvn : "-" )
        << " has_PMT_pvn " << ( has_PMT_pvn ? "YES" : "NO " ) 
        << std::endl 
        ;      


    return is_sensor ? copyno : -1  ; 
}

/**
U4SensorIdentifierDefault::FindSD_r
-------------------------------------

Recursive traverse collecting pv pointers for pv with associated SensitiveDetector. 

**/

inline void U4SensorIdentifierDefault::FindSD_r( 
    std::vector<const G4VPhysicalVolume*>& sdpv , 
    const G4VPhysicalVolume* pv, 
    int depth )
{
    const G4LogicalVolume* lv = pv->GetLogicalVolume() ;
    G4VSensitiveDetector* sd = lv->GetSensitiveDetector() ;
    if(sd) sdpv.push_back(pv); 
    for (size_t i=0 ; i < size_t(lv->GetNoDaughters()) ; i++ ) FindSD_r( sdpv, lv->GetDaughter(i), depth+1 );
}


inline bool U4SensorIdentifierDefault::IsInterestingCopyNo( int copyno )
{
    return 
        copyno > -1 && 
           (
            (std::abs( copyno -      0 ) < 100) || 
            (std::abs( copyno -  17612 ) < 100) ||
            (std::abs( copyno -  30000 ) < 100) ||
            (std::abs( copyno -  32400 ) < 100) ||
            (std::abs( copyno - 300000 ) < 100) || 
            (std::abs( copyno - 325600 ) < 100)  
           )
        ;   
}


