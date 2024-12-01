#include <sstream>
#include <cstring>
#include "SLOG.hh"

#include "QRng.hh"


#ifdef OLD_MONOLITHIC_CURANDSTATE
#include "SCurandState.hh"
#else
#include "SEventConfig.hh"
#include "SCurandState.h"
#endif

#include "sdirectory.h"
#include "ssys.h"

#include "qrng.h"
#include "QU.hh"

#include "QUDA_CHECK.h"

const plog::Severity QRng::LEVEL = SLOG::EnvLevel("QRng", "DEBUG"); 
const QRng* QRng::INSTANCE = nullptr ; 
const QRng* QRng::Get(){ return INSTANCE ;  }

std::string QRng::Desc() // static
{
    std::stringstream ss ; 
    ss << "QRng::Desc"
       << " IMPL " << IMPL 
       ;
    std::string str = ss.str() ;
    return str ;  
}


/**
QRng::QRng
------------

QRng instanciation is invoked from QSim::UploadComponents

**/

QRng::QRng(unsigned skipahead_event_offset)
    :
#ifdef OLD_MONOLITHIC_CURANDSTATE
    path(SCurandState::Path()),        // null path will assert in Load
    rngmax(0),
    d_rng_states(LoadAndUpload(rngmax, path)),   // rngmax set based on file_size/item_size of path 
#else
    cs(nullptr),
    path(cs.getDir()),                        // informational 
    rngmax(SEventConfig::MaxCurandState()),    // max of : OPTICKS_MAX_PHOTON OPTICKS_MAX_SIMTRACE 
    d_rng_states(LoadAndUpload(rngmax, cs)),   // 
#endif
    qr(new qrng(d_rng_states, skipahead_event_offset)),
    d_qr(nullptr)
{
    init(); 
}


void QRng::init()
{
    INSTANCE = this ; 

    initMeta(); 

    bool VERBOSE = ssys::getenvbool(init_VERBOSE); 
    LOG_IF(info, VERBOSE)
         << "[" << init_VERBOSE << "] " << ( VERBOSE ? "YES" : "NO " )
         << "\n"
         << desc()
         ;  
}

/**
QRng::initMeta
------------------

1. record device pointer qr->rng_startes

2. upload qrng.h *qr* instance within single element array, setting d_qr

**/

void QRng::initMeta()
{
    assert( qr->rng_states == d_rng_states ) ; 

    const char* label_1 = "QRng::initMeta/d_qr" ; 
    d_qr = QU::UploadArray<qrng>(qr, 1, label_1 ); 

    bool uploaded = d_qr != nullptr && d_rng_states != nullptr ; 
    LOG_IF(fatal, !uploaded) << " FAILED to upload curandState and/or metadata " ;  
    assert(uploaded); 
}


void QRng::cleanup()
{
    QUDA_CHECK(cudaFree(qr->rng_states)); 
}

QRng::~QRng()
{
}



#ifdef OLD_MONOLITHIC_CURANDSTATE

const char* QRng::Load_FAIL_NOTES = R"(
QRng::Load_FAIL_NOTES
=================================

QRng::Load failed to load the curandState files. 
These files should have been created during the *opticks-full* installation 
by the bash function *opticks-prepare-installation* 
which runs *qudarap-prepare-installation*. 

Investigate by looking at the contents of the curandState directory, 
as shown below::

    epsilon:~ blyth$ ls -l  ~/.opticks/rngcache/RNG/
    total 892336
    -rw-r--r--  1 blyth  staff   44000000 Oct  6 19:43 QCurandState_1000000_0_0.bin
    -rw-r--r--  1 blyth  staff  132000000 Oct  6 19:53 QCurandState_3000000_0_0.bin
    epsilon:~ blyth$ 


)" ;

#else
const char* QRng::Load_FAIL_NOTES = R"(
QRng::Load_FAIL_NOTES
===============================

TODO : for new chunked impl

)" ;

#endif




#ifdef OLD_MONOLITHIC_CURANDSTATE

/**
QRng::LoadAndUpload
--------------------

rngmax is an output argument obtained from file_size/item_size 

**/

curandState* QRng::LoadAndUpload(ULL& rngmax, const char* path)  // static 
{
    curandState* h_states = Load(rngmax, path); 
    curandState* d_states = UploadAndFree(h_states, rngmax ); 
    return d_states ; 
}

curandState* QRng::Load(ULL& rngmax, const char* path)  // static 
{
    bool null_path = path == nullptr ; 
    LOG_IF(fatal, null_path ) << " QRng::Load null path " ; 
    assert( !null_path );  

    FILE *fp = fopen(path,"rb");
    bool failed = fp == nullptr ; 
    LOG_IF(fatal, failed ) << " unabled to open file [" << path << "]" ; 
    LOG_IF(error, failed ) << Load_FAIL_NOTES  ; 
    assert(!failed); 


    fseek(fp, 0L, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    long type_size = sizeof(curandState) ;  
    long item_size = 44 ; 

    rngmax = file_size/item_size ; 


    LOG(LEVEL) 
        << " path " << path 
        << " file_size " << file_size 
        << " item_size " << item_size 
        << " type_size " << type_size 
        << " rngmax " << rngmax
        ; 

    assert( file_size % item_size == 0 );  

    curandState* rng_states = (curandState*)malloc(sizeof(curandState)*rngmax);

    for(ULL i = 0 ; i < rngmax ; ++i )
    {   
        curandState& rng = rng_states[i] ;
        fread(&rng.d,                     sizeof(unsigned int),1,fp);   //  1
        fread(&rng.v,                     sizeof(unsigned int),5,fp);   //  5 
        fread(&rng.boxmuller_flag,        sizeof(int)         ,1,fp);   //  1 
        fread(&rng.boxmuller_flag_double, sizeof(int)         ,1,fp);   //  1
        fread(&rng.boxmuller_extra,       sizeof(float)       ,1,fp);   //  1
        fread(&rng.boxmuller_extra_double,sizeof(double)      ,1,fp);   //  2    11*4 = 44 
    }   
    fclose(fp);

    return rng_states ; 
}

curandState* QRng::UploadAndFree(curandState* h_states, ULL num_states )  // static 
{
    const char* label_0 = "QRng::UploadAndFree/rng_states" ; 
    curandState* d_states = QU::UploadArray<curandState>(h_states, num_states, label_0 ) ;   
    free(h_states); 
    return d_states ;  
}




#else

/**
QRng::LoadAndUpload
----------------------

rngmax
    input argument that determines how many chunks of curandState to load and upload

(_SCurandState)cs
    vector of SCurandChunk metadata on the chunk files 


For example with chunks of 10M each and rngmax of 25M::

     10M     10M      10M
   +------+--------+-------+
   

Read full chunks until doing so would go over rngmax, then 


**/

curandState* QRng::LoadAndUpload(ULL rngmax, const _SCurandState& cs)  // static 
{
    LOG(info) << cs.desc() ; 

    curandState* d0 = QU::device_alloc<curandState>( rngmax, "QRng::LoadAndUpload/rngmax" ); 
    curandState* d = d0 ; 

    ULL available_chunk = cs.chunk.size(); 
    ULL count = 0 ; 

    std::cout 
        << "QRng::LoadAndUpload"
        << " rngmax " << rngmax
        << " rngmax/M " << rngmax/M
        << " available_chunk " << available_chunk 
        << " cs.all.num/M " << cs.all.num/M 
        << " d0 " << d0 
        << "\n"
        ;

    for(ULL i=0 ; i < available_chunk ; i++)
    {
        ULL remaining = rngmax - count ;  

        const SCurandChunk& chunk = cs.chunk[i]; 
 
        bool partial_read = remaining < chunk.ref.num ;  

        ULL num = partial_read ? remaining : chunk.ref.num ;

        std::cout 
            << "QRng::LoadAndUpload"
            << " i " << std::setw(3) << i 
            << " chunk.ref.num/M " << std::setw(4) << chunk.ref.num/M
            << " count/M " << std::setw(4) << count/M
            << " remaining/M " << std::setw(4) << remaining/M
            << " partial_read " << ( partial_read ? "YES" : "NO " )
            << " num/M " << std::setw(4) << num/M
            << " d " << d 
            << "\n"
            ;

        scurandref cr = chunk.load(num) ;
  
        assert( cr.states != nullptr); 

        bool num_match = cr.num == num ; 
        if(!num_match) std::cerr
            << "QRng::LoadAndUpload"
            << " num_match " << ( num_match ? "YES" : "NO " )
            << " cr.num/M " << cr.num/M
            << " num/M " << num/M
            << "\n"
            ;

        assert(num_match); 

        QU::copy_host_to_device<curandState>( d , cr.states , num ); 

        free(cr.states); 

        d += num ;  
        count += num ;  

        if(count > rngmax) assert(0); 
        if(count == rngmax) break ;
    }

    bool complete = count == rngmax ; 
    assert( complete );
    return complete ? d0 : nullptr ; 
}

#endif


/**
QRng::Save
------------

Used from the old QCurandState::save

TODO: eliminate, functionality duplicates in SCurandChunk::Save

**/
void QRng::Save( curandState* states, unsigned num_states, const char* path ) // static
{
    sdirectory::MakeDirsForFile(path);
    FILE *fp = fopen(path,"wb");
    LOG_IF(fatal, fp == nullptr) << " error opening file " << path ; 
    assert(fp); 

    for(unsigned i = 0 ; i < num_states ; ++i )
    {   
        curandState& rng = states[i] ;
        fwrite(&rng.d,                     sizeof(unsigned int),1,fp);
        fwrite(&rng.v,                     sizeof(unsigned int),5,fp);
        fwrite(&rng.boxmuller_flag,        sizeof(int)         ,1,fp);
        fwrite(&rng.boxmuller_flag_double, sizeof(int)         ,1,fp);
        fwrite(&rng.boxmuller_extra,       sizeof(float)       ,1,fp);
        fwrite(&rng.boxmuller_extra_double,sizeof(double)      ,1,fp);
    }   
    fclose(fp);
    return ; 
}










std::string QRng::desc() const
{
    std::stringstream ss ; 
    ss << "QRng"
       << " path " << path 
       << " rngmax " << rngmax 
       << " qr " << qr
       << " qr.skipahead_event_offset " << qr->skipahead_event_offset
       << " d_qr " << d_qr
       << Desc() 
       ;

    std::string str = ss.str(); 
    return str ; 
}



template <typename T>
extern void QRng_generate(
    dim3, 
    dim3, 
    T*, 
    unsigned,
    unsigned,
    curandState*,
    unsigned long long 
);


template<typename T>
void QRng::generate( T* uu, unsigned ni, unsigned nv, unsigned long long skipahead_ )
{
    T* d_uu = QU::device_alloc<T>(ni*nv, "QRng::generate:ni*nv");

    QU::ConfigureLaunch(numBlocks, threadsPerBlock, ni, 1 );  

    QRng_generate<T>(numBlocks, threadsPerBlock, d_uu, ni, nv, qr->rng_states, skipahead_ ); 

    const char* label = "QRng::generate" ; 
    QU::copy_device_to_host_and_free<T>( uu, d_uu, ni*nv, label );
}


template void QRng::generate<float>( float*,   unsigned, unsigned, unsigned long long ); 
template void QRng::generate<double>( double*, unsigned, unsigned, unsigned long long ); 








template <typename T>
extern void QRng_generate_2(dim3, dim3, qrng*, unsigned, T*, unsigned, unsigned );


template<typename T>
void QRng::generate_2( T* uu, unsigned ni, unsigned nv, unsigned event_idx )
{
    const char* label = "QRng::generate_2:ni*nv" ; 

    T* d_uu = QU::device_alloc<T>(ni*nv, label );

    QU::ConfigureLaunch(numBlocks, threadsPerBlock, ni, 1 );  

    QRng_generate_2<T>(numBlocks, threadsPerBlock, d_qr, event_idx, d_uu, ni, nv ); 

    QU::copy_device_to_host_and_free<T>( uu, d_uu, ni*nv, label );
}


template void QRng::generate_2<float>( float*,   unsigned, unsigned, unsigned ); 
template void QRng::generate_2<double>( double*, unsigned, unsigned, unsigned ); 


