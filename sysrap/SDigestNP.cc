#include <cassert>
#include "NP.hh"
#include "SDigest.hh"
#include "SDigestNP.hh" 
   
std::string SDigestNP::Item( const NP* a, int i, int j, int k, int l, int m, int o ) // static  
{
    const char* start = nullptr ; 
    NP::INT num_bytes = 0 ; 
    a->itembytes_(&start, num_bytes, i, j, k, l, m, o ); 
    assert( start && num_bytes > 0 ); 
    return SDigest::Buffer( start, num_bytes ); 
}

