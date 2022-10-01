#include "QTexMaker.hh"

#include "scuda.h"
#include "NP.hh"
#include "QTex.hh"
#include "SLOG.hh"

const plog::Severity QTexMaker::LEVEL = SLOG::EnvLevel("QTexMaker", "DEBUG"); 


QTex<float4>* QTexMaker::Make2d_f4( const NP* icdf, char filterMode, bool normalizedCoords )  // static 
{
    unsigned ndim = icdf->shape.size(); 
    LOG(info) << " ndim " << ndim ; 
    unsigned hd_factor = icdf->get_meta<unsigned>("hd_factor", 0) ; 
    LOG(info) << " hd_factor " << hd_factor ; 

    if( filterMode == 'P' ) LOG(fatal) << " filtermode 'P' without interpolation is in use : appropriate for basic tex machinery tests only " ; 

    LOG(info)
        << "["  
        << " icdf " << icdf->sstr()
        << " ndim " << ndim 
        << " hd_factor " << hd_factor 
        << " filterMode " << filterMode 
        ;

    assert( ndim == 3 && icdf->shape[ndim-1] == 4 ); 

    QTex<float4>* tex = QTexMaker::Make2d_f4_(icdf, filterMode, normalizedCoords ); 
    tex->setHDFactor(hd_factor); 
    tex->uploadMeta(); 

    LOG(LEVEL) << "]" ; 

    return tex ; 
}




QTex<float4>* QTexMaker::Make2d_f4_( const NP* a, char filterMode, bool normalizedCoords )  // static 
{
    assert( a->ebyte == 4 && "need to narrow double precision arrays first ");  
    unsigned ndim = a->shape.size(); 
    assert( ndim == 3 );      

    unsigned ni = a->shape[0] ; 
    unsigned nj = a->shape[1] ; 
    unsigned nk = a->shape[2] ; assert( nk == 4 ); 

    size_t height = ni ; 
    size_t  width = nj ; 
    const void* src = (const void*)a->bytes(); 

    // note that from the point of view of array content, saying (height, width) 
    // is a more appropriate ordering than the usual contrary convention  
    
    QTex<float4>* tex = new QTex<float4>( width, height, src, filterMode, normalizedCoords  );
    tex->setOrigin(a); 

    return tex ; 
}

