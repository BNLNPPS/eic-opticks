#pragma once
/**
QProp : setup to allow direct (no texture) interpolated property access on device 
=====================================================================================

See NP::Combine for the construction of compound prop array 
from many indiviual prop arrays with various domain lengths 
and differing domain values 

See ~/np/tests/NPInterp.py for prototyping the linear interpolation 

**/

#include <vector>
#include <string>
#include "plog/Severity.h"
#include "QUDARAP_API_EXPORT.hh"

union quad ; 
struct float4 ; 
struct dim3 ; 
template <typename T> struct qprop ; 
struct NP ; 

template <typename T>
struct QUDARAP_API QProp
{
    static const plog::Severity LEVEL ;
    static const QProp<T>*  INSTANCE ; 
    static const QProp<T>*  Get(); 

    const NP* a  ;  
    const T* pp ; 
    unsigned nv ; 

    unsigned ni ; 
    unsigned nj ; 
    unsigned nk ; 

    qprop<T>* prop ; 
    qprop<T>* d_prop ; 

    static QProp<T>* Make3D( const NP* a ); 
    QProp(const NP* a); 

    virtual ~QProp(); 
    void init(); 
    void uploadProps(); 
    void cleanup(); 

    void dump() const ; 
    std::string desc() const ;
    qprop<T>* getDevicePtr() const ;
    void lookup( T* lookup, const T* domain,  unsigned lookup_prop, unsigned domain_width ) const ; 
    void lookup_scan(T x0, T x1, unsigned nx, const char* fold, const char* reldir=nullptr ) const ; 

    void configureLaunch( dim3& numBlocks, dim3& threadsPerBlock, unsigned width, unsigned height ) const ;
};


