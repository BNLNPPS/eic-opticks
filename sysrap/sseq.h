#pragma once
/**
sseq.h : photon level step-by-step history and material recording seqhis/seqmat using 64 bit uint
==================================================================================================

For persisting srec arrays use::

   NP* seq = NP<unsigned long long>::Make(num_photon, 2)

**/

#include "scuda.h"

#if defined(__CUDACC__) || defined(__CUDABE__)
#    define SSEQ_METHOD __device__ __forceinline__
#else
#    define SSEQ_METHOD inline 
#endif

#if defined(__CUDACC__) || defined(__CUDABE__)
#define FFS(x)   (__ffs(x))
#define FFSLL(x) (__ffsll(x))
#else
#define FFS(x)   (ffs(x))
#define FFSLL(x) (ffsll(x))
#endif

#define rFFS(x)   ( x == 0 ? 0 : 0x1 << (x - 1) ) 
#define rFFSLL(x) ( x == 0 ? 0 : 0x1ull << (x - 1) )   
// use rFFSLL when values of x exceed about 31


#if defined(__CUDACC__) || defined(__CUDABE__)
#else
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "smath.h"
#include "OpticksPhoton.hh"
#include "sstr.h"
#endif

struct sseq
{
    typedef unsigned long long ULL ; 
    static SSEQ_METHOD unsigned GetNibble(const ULL& seq, unsigned slot) ;  
    static SSEQ_METHOD void     ClearNibble(    ULL& seq, unsigned slot) ;  
    static SSEQ_METHOD void     SetNibble(      ULL& seq, unsigned slot, unsigned value ) ;  

    ULL seqhis ; 
    ULL seqbnd ; 

    SSEQ_METHOD void zero() { seqhis = 0ull ; seqbnd = 0ull ; }
    SSEQ_METHOD void add_nibble( unsigned slot, unsigned flag, unsigned boundary ); 

    SSEQ_METHOD unsigned get_flag(unsigned slot) const ;
    SSEQ_METHOD void     set_flag(unsigned slot, unsigned flag) ;
    SSEQ_METHOD int      seqhis_nibbles() const ;
    SSEQ_METHOD int      seqbnd_nibbles() const ;

#if defined(__CUDACC__) || defined(__CUDABE__)
#else
    SSEQ_METHOD std::string desc() const ; 
    SSEQ_METHOD std::string desc_seqhis() const ; 
#endif
};


SSEQ_METHOD unsigned sseq::GetNibble(const unsigned long long& seq, unsigned slot)
{ 
    return ( seq >> 4*slot ) & 0xfull ; 
}

SSEQ_METHOD void sseq::ClearNibble(unsigned long long& seq, unsigned slot)
{
    seq &= ~( 0xfull << 4*slot ) ; 
}

SSEQ_METHOD void sseq::SetNibble(unsigned long long& seq, unsigned slot, unsigned value)
{ 
    seq =  ( seq & ~( 0xfull << 4*slot )) | ( (value & 0xfull) << 4*slot ) ;   
}

SSEQ_METHOD unsigned sseq::get_flag(unsigned slot) const 
{
    unsigned f = GetNibble(seqhis, slot) ; 
    return  f == 0 ? 0 : 0x1ull << (f - 1) ; 
}
SSEQ_METHOD void sseq::set_flag(unsigned slot, unsigned flag)
{
    SetNibble(seqhis, slot, FFS(flag)) ; 
}

SSEQ_METHOD int sseq::seqhis_nibbles() const { return smath::count_nibbles(seqhis) ; }
SSEQ_METHOD int sseq::seqbnd_nibbles() const { return smath::count_nibbles(seqbnd) ; }


/**
sseq::add_nibble
------------------

Populates one nibble of the seqhis+seqbnd bitfields 

Hmm signing the boundary for each step would eat into bits too much, perhaps 
just collect material, as done in old workflow ?

Have observed (see test_desc_seqhis) that when adding more than 16 nibbles 
into the 64 bit ULL (which will not fit), get unexpected "mixed" wraparound 
not just simply overwriting.  

**/

SSEQ_METHOD void sseq::add_nibble(unsigned slot, unsigned flag, unsigned boundary )
{
    seqhis |=  (( FFS(flag) & 0xfull ) << 4*slot ); 
    seqbnd |=  (( boundary  & 0xfull ) << 4*slot ); 
    // 0xfull is needed to avoid all bits above 32 getting set
    // NB: nibble restriction of each "slot" means there is absolute no need for FFSLL
}

#if defined(__CUDACC__) || defined(__CUDABE__)
#else



SSEQ_METHOD std::string sseq::desc() const 
{
    std::stringstream ss ; 
    ss 
         << " seqhis " << std::setw(16) << std::hex << seqhis << std::dec 
         << " seqbnd " << std::setw(16) << std::hex << seqbnd << std::dec 
         ;
    std::string s = ss.str(); 
    return s ; 
}

SSEQ_METHOD std::string sseq::desc_seqhis() const 
{
    std::string fseq = OpticksPhoton::FlagSequence(seqhis) ; 

    std::stringstream ss ; 
    ss 
        << " " << std::setw(16) << std::hex << seqhis << std::dec 
        << " nib " << std::setw(2) << seqhis_nibbles() 
        << " " << sstr::TrimTrailing(fseq.c_str()) 
        ;
    std::string s = ss.str(); 
    return s ; 
}

#endif


