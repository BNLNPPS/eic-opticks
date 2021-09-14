#pragma once

#include "QUDARAP_API_EXPORT.hh"

/**
QCK : carrier struct holding Cerenkov lookup arrays, created by QCerenkov  
===========================================================================

Instances of QCK are created by *QCerenkov::makeICDF*

Contains NP arrays:

rindex
    (ni, 2)  refractive index values together with energy domain values (eV) 
    for a single material 

bis
    (ny,) : BetaInverse domain values in range from from 1. to RINDEX_max

s2c
    (ny,nx,2) : *ny* cumulative Cerenkov sin^2 theta "s2" energy integrals up to *nx* energy cuts. 
    Last "payload" dimension is 2 as the energy domain values are kept together with the 
    cumulative integral values. This is necessary as the energies are not common, 
    with each BetaInverse having different energy ranges over which Cerenkov photons 
    can be produced. 

s2cn
    (ny,nx,2) : normalized version of *s2c* with all values divided by the maximum energy 
    integral values giving the inverse-CDF ICDF. This is usable via NP::pdomain
    to perform domain lookups to generate Cerenkov wavelengths given input BetaInverse
    and a random number.   

**/

struct NP ; 

template <typename T>
struct QUDARAP_API QCK
{
    NP* rindex ; 
    NP* bis ;  // BetaInverse 
    NP* s2c ;  // cumulative s2 integral 
    NP* s2cn ; // normalized *s2c*, aka *icdf* 

    T emn ; 
    T emx ; 
    T rmn ; 
    T rmx ; 

    void init() ;  

    void save(const char* base, const char* reldir=nullptr) const ; 
    static QCK* Load(const char* base, const char* reldir=nullptr); 

    // lookup from sets of ICDF, normalized s2 energy integrals  
    bool is_permissable( const T BetaInverse) const ; 
    T   energy_lookup_( const T BetaInverse, const T u) const ;  
    NP* energy_lookup(  const T BetaInverse, const NP* uu) const ; 

    // traditional s2 rejection sampling using rindex as function of energy 
    T   energy_sample_( const T BetaInverse,  const std::function<T()>& rng ) const ; 
    NP* energy_sample(  const T BetaInverse,  const std::function<T()>& rng, unsigned ni ) const ; 

}; 

