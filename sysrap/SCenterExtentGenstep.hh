#pragma once

struct NP ; 
struct quad6 ; 
template <typename T> struct Tran ;

#include <vector>
#include "plog/Severity.h"
#include "SYSRAP_API_EXPORT.hh"

/**


NB this was demoted from extg4/X4Intersect as its all generally applicable 
and hence belongs at a lower level 

**/

struct SYSRAP_API SCenterExtentGenstep
{
    static const plog::Severity LEVEL ; 

    SCenterExtentGenstep(); 
    void init(); 
    const char* desc() const ; 
    void save(const char* dir) const ; 
    template<typename T> void set_meta(const char* key, T value ) ; 


    NP*    gs ;         // not const as need to externally set the meta 
    float  gridscale ;   
    quad4* peta ; 
    bool   dump ; 
    float4 ce ;
 
    std::vector<int> cegs ; 
    int nx ; 
    int ny ; 
    int nz ; 

    std::vector<int> override_ce ;   
    std::vector<quad4> pp ;
    std::vector<quad4> ii ;

};



