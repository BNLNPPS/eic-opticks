
#include <cuda_runtime.h>
#include <sstream>

#include "SStr.hh"
#include "scuda.h"
#include "squad.h"

#include "QUDA_CHECK.h"
#include "NP.hh"

#include "QU.hh"
#include "QBuf.hh"
#include "QOptical.hh"

#include "SLOG.hh"

const plog::Severity QOptical::LEVEL = SLOG::EnvLevel("QOptical", "DEBUG");

const QOptical* QOptical::INSTANCE = nullptr ; 
const QOptical* QOptical::Get(){ return INSTANCE ; }

QOptical::QOptical(const NP* optical_)
    :
    optical(optical_),
    buf(QBuf<unsigned>::Upload(optical)),
    d_optical((quad*)buf->d)
{
    INSTANCE = this ; 
} 

std::string QOptical::desc() const
{
    std::stringstream ss ; 
    ss << "QOptical"
       << " optical " << ( optical ? optical->desc() : "-" )
       ;   
    std::string s = ss.str(); 
    return s ; 
}

// from QOptical.cu
extern "C" void QOptical_check(dim3 numBlocks, dim3 threadsPerBlock, quad* optical, unsigned width, unsigned height ); 

void QOptical::check() const 
{
    unsigned height = optical->shape[0] ; 
    unsigned width = 1 ; 

    LOG(LEVEL) 
         << "[" << " height " << height << " width " << width 
         ; 

    dim3 numBlocks ; 
    dim3 threadsPerBlock ; 
    QU::ConfigureLaunch( numBlocks, threadsPerBlock, width, height ); 

    QOptical_check(numBlocks, threadsPerBlock, d_optical, width, height );  

    LOG(LEVEL) << "]" ; 
}


