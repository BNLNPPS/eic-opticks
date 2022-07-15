#pragma once

/**
sframe.h
===========

Persisted into (4,4,4) array.
Any extension should be in quad4 blocks 
for persisting, alignment and numpy convenience

Note that some variables like *frs* are
persisted in metadata, not in the array. 

Currently *frs* is usually the same as *moi* from MOI envvar
but are using *frs* to indicate intension for generalization 
to frame specification using global instance index rather than MOI
which uses the gas specific instance index. 

TODO: should be using Tran<double> for transforming 


**/

#include <cassert>
#include <vector>
#include "scuda.h"
#include "squad.h"
#include "sqat4.h"
#include "stran.h"
#include "NP.hh"
#include "SPath.hh"


struct sframe
{
    static constexpr const char* NAME = "sframe.npy" ; 
    static sframe Load( const char* dir, const char* name=NAME); 
    static sframe Load_(const char* path ); 
    static constexpr const unsigned NUM_4x4 = 4 ; 
    static constexpr const unsigned NUM_VALUES = NUM_4x4*4*4 ; 


    float4 ce = {} ;   // 0
    quad   q1 = {} ; 
    quad   q2 = {} ; 
    quad   q3 = {} ; 
   
    qat4   m2w ;       // 1
    qat4   w2m ;       // 2

    quad4  aux = {} ;  // 3


    // on the edge, the above are memcpy in/out by load/save
    const char* frs = nullptr ; 





    void set_grid(const std::vector<int>& cegs, float gridscale); 
    int ix0() const ; 
    int ix1() const ; 
    int iy0() const ; 
    int iy1() const ; 
    int iz0() const ; 
    int iz1() const ; 
    int num_photon() const ; 
    float gridscale() const ; 


    void set_midx_mord_iidx(int midx, int mord, int iidx); 
    int midx() const ; 
    int mord() const ; 
    int iidx() const ; 

    void set_inst(int inst); 
    int inst() const ; 

    void set_ins_gas_ias(int ins, int gas, int ias); 
    int ins() const ; 
    int gas() const ; 
    int ias() const ; 

    void set_propagate_epsilon(float eps); 
    float propagate_epsilon() const ; 

    float* data() ; 
    const float* cdata() const ; 

    void write( float* dst, unsigned num_values ) const ;
    NP* make_array() const ; 
    void save(const char* dir, const char* name=NAME) const ; 

    void read( const float* src, unsigned num_values ) ; 
    void load(const char* dir, const char* name=NAME) ; 
    void load_(const char* path ) ; 
    void load(const NP* a) ; 


    Tran<double> get_m2w() const ; 
    Tran<double> get_w2m() const ;  
    NP* transform_photon_m2w( const NP* ph, bool normalize ); 
    NP* transform_photon_w2m( const NP* ph, bool normalize ); 

}; 

inline sframe sframe::Load(const char* dir, const char* name)
{
    sframe fr ; 
    fr.load(dir, name); 
    return fr ; 
}
inline sframe sframe::Load_(const char* path)
{
    sframe fr ; 
    fr.load_(path); 
    return fr ; 
}



inline void sframe::set_grid(const std::vector<int>& cegs, float gridscale)
{
    assert( cegs.size() == 7 );   // use QEvent::StandardizeCEGS to convert 4 to 7  

    q1.i.x = cegs[0] ;  // ix0   these are after standardization
    q1.i.y = cegs[1] ;  // ix1
    q1.i.z = cegs[2] ;  // iy0 
    q1.i.w = cegs[3] ;  // iy1

    q2.i.x = cegs[4] ;  // iz0
    q2.i.y = cegs[5] ;  // iz1 
    q2.i.z = cegs[6] ;  // num_photon
    q2.f.w = gridscale ; 
}

inline int sframe::ix0() const { return q1.i.x ; }
inline int sframe::ix1() const { return q1.i.y ; }
inline int sframe::iy0() const { return q1.i.z ; }
inline int sframe::iy1() const { return q1.i.w ; }
inline int sframe::iz0() const { return q2.i.x ; }
inline int sframe::iz1() const { return q2.i.y ; }
inline int sframe::num_photon() const { return q2.i.z ; }
inline float sframe::gridscale() const { return q2.f.w ; }


inline void sframe::set_midx_mord_iidx(int midx, int mord, int iidx)
{
    q3.i.x = midx ; 
    q3.i.y = mord ; 
    q3.i.z = iidx ; 
}
inline int sframe::midx() const { return q3.i.x ; }
inline int sframe::mord() const { return q3.i.y ; }
inline int sframe::iidx() const { return q3.i.z ; }


inline void sframe::set_inst(int inst){ q3.i.w = inst ; }
inline int sframe::inst() const { return q3.i.w ; }

inline void sframe::set_ins_gas_ias(int ins, int gas, int ias)
{
    aux.q0.i.x = ins ; 
    aux.q0.i.y = gas ; 
    aux.q0.i.z = ias ; 
}

inline void sframe::set_propagate_epsilon(float eps)
{
    aux.q1.f.x = eps ; 
}
inline float sframe::propagate_epsilon() const 
{
    return aux.q1.f.x ; 
}

inline int sframe::ins() const { return aux.q0.i.x ; }
inline int sframe::gas() const { return aux.q0.i.y ; }
inline int sframe::ias() const { return aux.q0.i.z ; }

inline const float* sframe::cdata() const 
{
    return (const float*)&ce.x ;  
}
inline float* sframe::data()  
{
    return (float*)&ce.x ;  
}
inline void sframe::write( float* dst, unsigned num_values ) const 
{
    assert( num_values == NUM_VALUES ); 
    char* dst_bytes = (char*)dst ; 
    char* src_bytes = (char*)cdata(); 
    unsigned num_bytes = sizeof(float)*num_values ; 
    memcpy( dst_bytes, src_bytes, num_bytes );
}    

inline void sframe::read( const float* src, unsigned num_values ) 
{
    assert( num_values == NUM_VALUES ); 
    char* src_bytes = (char*)src ; 
    char* dst_bytes = (char*)data(); 
    unsigned num_bytes = sizeof(float)*num_values ; 
    memcpy( dst_bytes, src_bytes, num_bytes );
}    

inline NP* sframe::make_array() const 
{
    NP* a = NP::Make<float>(NUM_4x4, 4, 4) ; 
    write( a->values<float>(), NUM_4x4*4*4 ) ; 
    return a ; 
}
inline void sframe::save(const char* dir, const char* name) const
{
    NP* a = make_array(); 
    a->set_meta<std::string>("creator", "sframe::save"); 
    if(frs) a->set_meta<std::string>("frs", frs); 
    a->save(dir, name); 
}
inline void sframe::load(const char* dir, const char* name) 
{
    const NP* a = NP::Load(dir, name); 
    load(a); 
}
inline void sframe::load_(const char* path_)   // eg $A_FOLD/sframe.npy
{
    const char* path = SPath::Resolve(path_, NOOP);
    bool exists = SPath::Exists(path) ;
    if(!exists) 
        std::cerr 
            << "sframe::load_ ERROR : non-existing" 
            << " path_ " << path_ 
            << " path " << path 
            << std::endl   
            ;

    assert(exists); 

    if( exists )
    {
        const NP* a = NP::Load(path);
        load(a); 
    }
}
inline void sframe::load(const NP* a) 
{
    read( a->cvalues<float>() , NUM_VALUES );   
    std::string _frs = a->get_meta<std::string>("frs", ""); 
    if(!_frs.empty()) frs = strdup(_frs.c_str()); 
}





inline Tran<double> sframe::get_m2w() const { return Tran<double>::ConvertFromQat(&m2w); }
inline Tran<double> sframe::get_w2m() const { return Tran<double>::ConvertFromQat(&w2m); }

/**
sframe::transform_photon_m2w
-------------------------------

Canonical call from SEvt::setFrame for transforming input photons into frame 
When normalize is true the mom and pol are normalized after the transformation. 

Note that the transformed photon array is always in double precision. 
That will be narrowed down to float prior to upload by QEvent::setInputPhoton

**/

inline NP* sframe::transform_photon_m2w( const NP* ph, bool normalize )
{
    if( ph == nullptr ) return nullptr ; 
    Tran<double> t_m2w = get_m2w() ; 
    NP* pht = Tran<double>::PhotonTransform(ph, normalize,  &t_m2w );
    assert( pht->ebyte == 8 ); 
    return pht ; 
}

inline NP* sframe::transform_photon_w2m( const NP* ph, bool normalize  )
{
    if( ph == nullptr ) return nullptr ; 
    Tran<double> t_w2m = get_w2m() ; 
    NP* pht = Tran<double>::PhotonTransform(ph, normalize, &t_w2m );
    assert( pht->ebyte == 8 ); 
    return pht ; 
}






inline std::ostream& operator<<(std::ostream& os, const sframe& fr)
{
    os 
       << " frs " << ( fr.frs ? fr.frs : "-" ) << std::endl 
       << " ce  " << fr.ce 
       << std::endl 
       << " m2w " << fr.m2w 
       << std::endl 
       << " w2m " << fr.w2m 
       << std::endl 
       << " midx " << std::setw(4) << fr.midx()
       << " mord " << std::setw(4) << fr.mord()
       << " iidx " << std::setw(4) << fr.iidx()
       << std::endl 
       << " inst " << std::setw(4) << fr.inst()
       << std::endl 
       << " ix0  " << std::setw(4) << fr.ix0()
       << " ix1  " << std::setw(4) << fr.ix1()
       << " iy0  " << std::setw(4) << fr.iy0()
       << " iy1  " << std::setw(4) << fr.iy1()
       << " iz0  " << std::setw(4) << fr.iz0()
       << " iz1  " << std::setw(4) << fr.iz1()
       << " num_photon " << std::setw(4) << fr.num_photon()
       << std::endl 
       << " ins  " << std::setw(4) << fr.ins()
       << " gas  " << std::setw(4) << fr.gas()
       << " ias  " << std::setw(4) << fr.ias()
       << std::endl 
       ;

    return os; 
}



