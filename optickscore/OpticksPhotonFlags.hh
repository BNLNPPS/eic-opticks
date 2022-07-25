#pragma once
#include <string>
#include <glm/fwd.hpp>
#include "plog/Severity.h"
#include "OKCORE_API_EXPORT.hh"

/**
OpticksPhotonFlags 
===================

Encapsulate the quad of photon flags written into GPU photon buffer 
by optixrap/cu/generate.cu

The whole flags Quad is passed to the static functions to 
ease future rearrangments of flags 

See the "FLAGS Macro" documentation of ocu/generate.cu 
for where the flags come from. 


**/

struct OKCORE_API OpticksPhotonFlags 
{
    static const plog::Severity LEVEL ; 
    union uif_t {
        unsigned u ; 
        int      i ; 
        float    f ; 
    };  
     
    static int      Boundary(   const float& x , const float&   , const float&   , const float&   );
    static unsigned SensorIndex(const float& x , const float&   , const float&   , const float&   );
    static unsigned NodeIndex(  const float&,    const float& y , const float&   , const float&   );
    static unsigned PhotonIndex(const float&,    const float&   , const float& z , const float&   );
    static unsigned FlagMask(   const float&,    const float&   , const float&   , const float& w );

    static int      Boundary(   const glm::vec4& f ); 
    static unsigned SensorIndex(const glm::vec4& f );
    static unsigned NodeIndex(  const glm::vec4& f );
    static unsigned PhotonIndex(const glm::vec4& f );
    static unsigned FlagMask(   const glm::vec4& f );

    int      boundary ; 
    unsigned sensorIndex ; 
    unsigned nodeIndex ; 
    unsigned photonIndex ; 
    unsigned flagMask ; 

    OpticksPhotonFlags( const glm::vec4& f );
    OpticksPhotonFlags( int boundary, unsigned sensorIndex, unsigned nodeIndex, unsigned photonIndex, unsigned flagMask ); 
    std::string desc() const ;
    std::string brief() const ;

    bool operator==(const OpticksPhotonFlags& rhs) const ;
};


