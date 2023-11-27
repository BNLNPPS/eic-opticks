#include "scuda.h"
#include "squad.h"
#include "stran.h"

#include "SSys.hh"
#include "SPath.hh"
#include "SEvent.hh"

#include "SGenstep.hh"
#include "SFrameGenstep.hh"
#include "SCenterExtentGenstep.hh"

#include "NP.hh"

#include "SLOG.hh"

const plog::Severity SCenterExtentGenstep::LEVEL = SLOG::EnvLevel("SCenterExtentGenstep", "DEBUG" ); 

const char* SCenterExtentGenstep::BASE = "$TMP/SCenterExtentGenstep"  ; 

void SCenterExtentGenstep::save() const
{
    save(BASE); 
}
void SCenterExtentGenstep::save(const char* dir) const
{
    const char* fold = SPath::Resolve(dir, DIRPATH);  
    
    LOG(LEVEL) << " saving to " << fold ; 

    gs->save(fold, "gs.npy" ); 
    save_vec(fold, "isect.npy", ii ); 
    save_vec(fold, "photons.npy", pp ); 
    NP::Write(fold, "peta.npy", (float*)(&peta->q0.f.x), 1, 4, 4 );
}

void SCenterExtentGenstep::save_vec(const char* dir, const char* name, const std::vector<quad4>& vv ) const 
{
    if(vv.size() == 0) 
    {
        LOG(LEVEL) << " skip as no vec entries for " << name  ;
        return ; 
    }
    LOG(LEVEL) << "[ " << name << " size " << vv.size() ;
    NP* arr = NP::Make<float>(vv.size(), 4, 4);
    LOG(LEVEL) << arr->sstr() ;
    arr->read<float>((float*)vv.data());
    arr->save(dir, name);
    LOG(LEVEL) << "]" ; 
}

template<typename T> void SCenterExtentGenstep::set_meta(const char* key, T value ) 
{
    assert(gs); 
    gs->set_meta<T>(key, value) ; 
}


SCenterExtentGenstep::SCenterExtentGenstep(const float4* ce_)
    :
    gs(nullptr),
    gridscale(SSys::getenvfloat("GRIDSCALE", 1.0 )),
    peta(new quad4),
    ce( ce_ ? *ce_ : make_float4(0.f, 0.f, 0.f, 100.f ))
{
    init(); 
} 

/**
SCenterExtentGenstep::init
-----------------------------

Interprets the CEGS envvar defining a 2D plane or 3D grid
of integer genstep positions. The integral coordinates get scaled 
by GRIDSCALE to give the float positions.  

**/

void SCenterExtentGenstep::init()
{
    peta->zero(); 

    LOG(LEVEL) << "[ gridscale " << gridscale  ;

    SSys::getenvintvec("CEGS", cegs, ':', "16:0:9:10" );
    // input CEGS are 4 or 7 ints delimited by colon nx:ny:nz:num_pho OR nx:px:ny:py:nz:py:num_pho 

 
    SFrameGenstep::StandardizeCEGS(cegs);
    assert( cegs.size() == 8 );

    int ix0 = cegs[0] ;
    int ix1 = cegs[1] ;
    int iy0 = cegs[2] ;
    int iy1 = cegs[3] ;
    int iz0 = cegs[4] ;
    int iz1 = cegs[5] ;
    int photons_per_genstep = cegs[6] ;
    int high = cegs[7] ; 


    nx = (ix1 - ix0)/2 ;
    ny = (iy1 - iy0)/2 ;
    nz = (iz1 - iz0)/2 ;
    int gridaxes = SGenstep::GridAxes(nx, ny, nz);

    LOG(LEVEL)
        << " nx " << nx
        << " ny " << ny
        << " nz " << nz
        << " GridAxes " << gridaxes
        << " GridAxesName " << SGenstep::GridAxesName(gridaxes)
        << " high " << high
        ;

    peta->q0.i.x = ix0 ;
    peta->q0.i.y = ix1 ;
    peta->q0.i.z = iy0 ;
    peta->q0.i.w = iy1 ;

    peta->q1.i.x = iz0 ;
    peta->q1.i.y = iz1 ;
    peta->q1.i.z = photons_per_genstep ;
    peta->q1.f.w = gridscale ;


    //const Tran<double>* geotran = Tran<double>::make_identity();
    const Tran<double>* geotran = Tran<double>::make_translate(ce.x, ce.y, ce.z);


    LOG(LEVEL) << "ce (" << ce.x << " " << ce.y << " " << ce.z << " " << ce.w << ")" ;

    peta->q2.f.x = ce.x ;   // moved from q1
    peta->q2.f.y = ce.y ;
    peta->q2.f.z = ce.z ;
    peta->q2.f.w = ce.w ;

    bool ce_scale = true ;

    float3 offset = make_float3(0.f, 0.f, 0.f ) ;
    std::vector<float3> ce_offset ; 
    ce_offset.push_back(offset);   

    gs = SFrameGenstep::MakeCenterExtentGenstep(ce, cegs, gridscale, geotran, ce_offset, ce_scale );

    const char* topline = SSys::getenvvar("TOPLINE", "SCenterExtentGenstep.topline") ; 
    const char* botline = SSys::getenvvar("BOTLINE", "SCenterExtentGenstep.botline" ) ; 
    set_meta<std::string>("TOPLINE", topline );
    set_meta<std::string>("BOTLINE", botline );

    SFrameGenstep::GenerateCenterExtentGenstepPhotons( pp, gs, gridscale );

    LOG(LEVEL) << "]" ;
}

const char* SCenterExtentGenstep::desc() const 
{
    std::stringstream ss ; 
    ss << " CEGS (" ; 
    for(unsigned i=0 ; i < cegs.size() ; i++ ) ss << cegs[i] << " " ; 
    ss << ")" ; 
    ss << " nx " << nx ; 
    ss << " ny " << ny ; 
    ss << " nz " << nz ; 
    ss << " GRIDSCALE " << gridscale ; 
    ss << " CE (" 
       << ce.x << " " 
       << ce.y << " " 
       << ce.z << " " 
       << ce.w 
       << ") " 
       ;   

    ss << " gs " << gs->sstr() ; 
    ss << " pp " << pp.size() ; 
    ss << " ii " << ii.size() ; 

    std::string s = ss.str(); 
    return strdup(s.c_str()); 
}


template void     SCenterExtentGenstep::set_meta<int>(const char*, int ); 
template void     SCenterExtentGenstep::set_meta<unsigned>(const char*, unsigned ); 
template void     SCenterExtentGenstep::set_meta<float>(const char*, float ); 
template void     SCenterExtentGenstep::set_meta<double>(const char*, double ); 
template void     SCenterExtentGenstep::set_meta<std::string>(const char*, std::string ); 


