#pragma once

#if defined(__CUDACC__) || defined(__CUDABE__)
   #define SSTATE_METHOD __device__
#else
   #define SSTATE_METHOD 
#endif 

/**
sstate.h
=========

This was formerly qstate.h but as no CUDA specifics it 
belongs down in sysrap not up in QUDARap. 

Populated by qsim::fill_state from texture and buffer lookups 
using photon wavelength and the boundary obtained from geometry 
intersect.  

Old version of this in OptiXRap/cu also copied things from "PRD" into here ... 
BUT seems no point doing that, can just directly use them from PRD. 

**/

struct sstate
{
    float4 material1 ;    // refractive_index/absorption_length/scattering_length/reemission_prob
    float4 m1group2 ;     // group_velocity/spare1/spare2/spare3
    float4 material2 ;   
    float4 surface ;      // detect/absorb/reflect_specular/reflect_diffuse

    uint4  optical ;      // x/y/z/w index/type/finish/value  
    uint4  index ;        // indices of m1/m2/surf/sensor

#ifdef WAY_ENABLED
    float4 way0 ;   
    float4 way1 ;   
#endif

#if defined(__CUDACC__) || defined(__CUDABE__)
#else
    void save(const char* dir) const ; 
#endif


};

#if defined(__CUDACC__) || defined(__CUDABE__)
#else
#include "NP.hh"
inline void sstate::save(const char* dir) const 
{
    NP* st = NP::Make<float>(1, 6, 4 ); 
    st->read2( (float*)&material1.x  ); 
    st->save(dir, "s.npy"); 
    // see QState::Save for a more fussy alternative
}
#endif





#if defined(__CUDACC__) || defined(__CUDABE__)
#else
inline std::ostream& operator<<(std::ostream& os, const sstate& s )   
{
    os << "sstate"
       << std::endl 
       << " material1 " << s.material1 
       << " (refractive_index/absorption_length/scattering_length/reemission_prob) " 
       << std::endl 
       << " m1group2 " << s.m1group2
       << " (group_velocity/spare1/spare2/spare3) "
       << std::endl 
       << " material2 " << s.material2 
       << " (refractive_index/absorption_length/scattering_length/reemission_prob) " 
       << std::endl 
       << " surface   " << s.surface
       << " (detect/absorb/reflect_specular/reflect_diffuse) " 
       << std::endl 
       << " optical   " << s.optical
       << " (x/y/z/w index/type/finish/value) "
       << std::endl 
       << " index     " << s.index
       << " (indices of m1/m2/surf/sensor) "
       << std::endl 
       ;
    return os; 
}
#endif









/**
https://stackoverflow.com/questions/252552/why-do-we-need-c-unions

https://forums.developer.nvidia.com/t/handling-structures-with-bitfields/60628

   njuffa April 24, 2018, 7:02pm #3 : My advice, based on experience gathered over 25 years: Avoid bitfields. 


See sysrap/tests/squadUnionTest.cc for idea to have cake and eat it too, ie easy access and easy persisting::

    union qstate2
    {
        struct 
        {   
            float m1_refractive_index ; 
            float m1_absorption_length ;
            float m1_scattering_length ; 
            float m1_reemission_prob ; 

            float m2_refractive_index ; 
            float m2_absorption_length ;
            float m2_scattering_length ; 
            float m2_reemission_prob ; 

        } field ;   
                
        quad2 q ;   
    };

Actually persisting is not much of a reason as can just cast the 
entire qstate to a quad6 for example. 

But if decide to pursure streamlined qstate with mixed up fields 
the named field access would be helpful to isolate user code from changes to the struct. 

**/



