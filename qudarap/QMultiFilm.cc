#include "SLOG.hh"
#include "SSys.hh"
#include "scuda.h"
#include "squad.h"

#include <sstream>

#include "NP.hh"

#include "QUDA_CHECK.h"
#include "QRng.hh"
#include "QU.hh"
#include "QMultiFilm.hh"
#include "QTex.hh"
#include "qmultifilm.h"

const plog::Severity QMultiFilm::LEVEL = SLOG::EnvLevel("QMultiFilm", "DEBUG"); 
const QMultiFilm* QMultiFilm::INSTANCE = nullptr ; 
const QMultiFilm* QMultiFilm::Get(){ return INSTANCE ;  }


QMultiFilm::QMultiFilm(const NP* lut )
    :
    dsrc(lut->ebyte == 8 ? lut : nullptr),
    src( lut->ebyte == 4 ? lut : NP::MakeNarrow(dsrc)),
    multifilm(new qmultifilm),
    d_multifilm(nullptr) 
{

    makeMultiFilmAllTex();
    INSTANCE = this ; 
    init();
}

qmultifilm* QMultiFilm::getDevicePtr() const
{   
    return d_multifilm ;
}

void QMultiFilm::init(){

    uploadMultifilmlut();

}

void QMultiFilm::uploadMultifilmlut()
{
    int num = 2 ;
    for(int i = 0 ; i < num ; i++)
    {
        multifilm->nnvt_normal_tex[i] = tex_nnvt_normal[i]->texObj ;
        multifilm->nnvt_highqe_tex[i] = tex_nnvt_highqe[i]->texObj ;
        multifilm->hama_tex[i]        = tex_hama[i]       ->texObj ;

        multifilm->nnvt_normal_meta[i]= tex_nnvt_normal[i]->d_meta ;
        multifilm->nnvt_highqe_meta[i]= tex_nnvt_highqe[i]->d_meta ;
        multifilm->hama_meta[i]       = tex_hama[i]       ->d_meta ; 
    }
    d_multifilm = QU::UploadArray<qmultifilm>(multifilm, 1,"QMultiFilm::uploadMultifilmlut" );
}


void QMultiFilm::makeMultiFilmAllTex(){

    assert( src->has_shape(3,2,128,256,4));
    std::vector<std::string> pmtTypeList;
    src -> get_names( pmtTypeList );
    assert( pmtTypeList.size() == 3);
    for(unsigned i = 0 ; i < pmtTypeList.size() ; i++){

        std::string pmtName = pmtTypeList[i];
        //NP* pmt_src = src -> spawn_item(i);
        QTex<float4>  ** tex_arr = nullptr;
        if(pmtName == "kPMT_NNVT"){
            tex_arr = tex_nnvt_normal;
        }
        else if( pmtName == "kPMT_NNVT_HighQE"){
            tex_arr = tex_nnvt_highqe; 
        }
        else if( pmtName == "kPMT_Hamamatsu"){
            tex_arr = tex_hama;
        }
        else{ 
            assert(0);
        }
        makeMultiFilmOnePMTTex( i , tex_arr );          

    }
}

void QMultiFilm::makeMultiFilmOnePMTTex(  int pmtcatIdx , QTex<float4> ** tex_pmt  ){

    //   int bndDimIdx = src->get_meta<int>("boundary");
    //   int resDimIdx = src->get_meta<int>("resolution");

    int resolution_dim = src->shape[1];

    //assert(bnd_dim == 2) ;
    assert(resolution_dim == 2);

    for(int i = 0; i < resolution_dim ; i++){
        int offset = i;
        tex_pmt[offset] = makeMultiFilmOneTex( pmtcatIdx , i );               
    }
}

QTex<float4>* QMultiFilm::makeMultiFilmOneTex( int pmtcatIdx , int resIdx ){


    assert( src->uifc == 'f' ); 
    assert( src->ebyte == 4 );    // expecting float src array, possible narrowed from double dsrc array  

    /*
       int bndDimIdx = src->get_meta<int>("boundary");
       int resDimIdx = src->get_meta<int>("resolution");
       int wvDimIdx = src->get_meta<int>("wavelength");
       int aoiDimIdx = src->get_meta<int>("aoi");
       int payDimIdx = src->get_meta<int>("payload");
       */
    int resolution_dim = src->shape[1];
    assert(resolution_dim == 2);

    unsigned ni = src->shape[2]; 
    unsigned nj = src->shape[3]; 
    unsigned nk = src->shape[4]; 

    assert( ni == 128); 
    assert( nj == 256); 
    assert( nk == 4 ); 

    unsigned ny = ni ; // height  
    unsigned nx = nj ; // width 

    int offset = pmtcatIdx*resolution_dim*ni*nj*nk + resIdx * ni*nj*nk;    

    bool qmultifilmlut_disable_interpolation = SSys::getenvbool("QMULTIFILMLUT_DISABLE_INTERP"); 
    char filterMode = qmultifilmlut_disable_interpolation ? 'P' : 'L' ; 

    LOG_IF(fatal, qmultifilmlut_disable_interpolation) << "QMULTIFILMLUT_DISABLE_INTERP active using filterMode " << filterMode ; 


    QTex<float4>* tx = new QTex<float4>(nx, ny, src->cvalues<float>()+offset , filterMode , 1, src ) ; 

    //tx->setHDFactor(hd_factor); 
    float wv_low  = src->get_meta<float>("wv_low");
    float wv_high = src->get_meta<float>("wv_high");

    float aoi_low = src->get_meta<float>("aoi_low");
    float aoi_high = src->get_meta<float>("aoi_high");
    float aoi_sublow = src->get_meta<float>("aoi_sublow");
    float aoi_subhigh = src->get_meta<float>("aoi_subhigh");

    quad domainX;
    domainX.f.x = aoi_low;
    domainX.f.y = aoi_high;
    domainX.f.z = aoi_sublow ;
    domainX.f.w = aoi_subhigh;
    tx->setMetaDomainX(&domainX);

    quad domainY;
    domainY.f.x = wv_low;
    domainY.f.y = wv_high;
    tx->setMetaDomainY(&domainY);

    tx->uploadMeta(); 

    LOG(LEVEL)
        << " src " << src->desc()
        << " nx (width) " << nx
        << " ny (height) " << ny
        //<< " tx.HDFactor " << tx->getHDFactor() 
        << " tx.filterMode " << tx->getFilterMode()
        << " LOG(LEVEL) = INFO "
        ;

    return tx ; 

}

std::string QMultiFilm::desc() const
{
    std::stringstream ss ; 
    ss << "QMultiFilm"
        << " dsrc " << ( dsrc ? dsrc->desc() : "-" )
        << " src " << ( src ? src->desc() : "-" )
        ; 
    int num = 2;
    for(int i = 0 ; i < num ;i++){
        ss<<" tex_hama["<<i<<"]" << ( tex_hama[i] ? tex_hama[i] ->desc(): "-") << std::endl;
    }

    for(int i = 0 ; i < num ;i++){
        ss<<" tex_nnvt_normal["<<i<<"]" << ( tex_nnvt_normal[i] ? tex_nnvt_normal[i] ->desc(): "-")<<std::endl;
    }

    for(int i = 0 ; i < num ;i++){
        ss<<" tex_nnvt_highqe["<<i<<"]" << ( tex_nnvt_highqe[i] ? tex_nnvt_highqe[i] ->desc(): "-")<<std::endl;
    }

    std::string s = ss.str(); 
    return s ; 
}



extern "C" void QMultiFilm_check(dim3 numBlocks, dim3 threadsPerBlock, unsigned width, unsigned height  ); 
extern "C" void QMultiFilm_lookup(dim3 numBlocks, dim3 threadsPerBlock, cudaTextureObject_t texObj, quad4* meta, float4* lookup, unsigned num_lookup, unsigned width, unsigned height); 

void QMultiFilm::configureLaunch( dim3& numBlocks, dim3& threadsPerBlock, unsigned width, unsigned height )
{
    threadsPerBlock.x = 32 ; 
    threadsPerBlock.y = 32 ; 
    threadsPerBlock.z = 1 ; 

    numBlocks.x = (width + threadsPerBlock.x - 1) / threadsPerBlock.x ; 
    numBlocks.y = (height + threadsPerBlock.y - 1) / threadsPerBlock.y ;
    numBlocks.z = 1 ; 

    LOG(LEVEL) 
        << " width " << std::setw(7) << width 
        << " height " << std::setw(7) << height 
        << " width*height " << std::setw(7) << width*height 
        << " threadsPerBlock"
        << "(" 
        << std::setw(3) << threadsPerBlock.x << " " 
        << std::setw(3) << threadsPerBlock.y << " " 
        << std::setw(3) << threadsPerBlock.z << " "
        << ")" 
        << " numBlocks "
        << "(" 
        << std::setw(3) << numBlocks.x << " " 
        << std::setw(3) << numBlocks.y << " " 
        << std::setw(3) << numBlocks.z << " "
        << ")" 
        ;
}

void QMultiFilm::check(){

    check( tex_hama[0] );

}

void QMultiFilm::check( QTex<float4> *tex )
{
    unsigned width = tex->width ; 
    unsigned height = tex->height ; 

    LOG(LEVEL)
        << " width " << width
        << " height " << height
        ;

    dim3 numBlocks ; 
    dim3 threadsPerBlock ; 
    configureLaunch( numBlocks, threadsPerBlock, width, height ); 
    QMultiFilm_check(numBlocks, threadsPerBlock, width, height );  

    cudaDeviceSynchronize();
}

NP* QMultiFilm::lookup(int pmtcatIdx , int resIdx ){

    QTex<float4> **tex = choose_tex(pmtcatIdx);
    int offset = resIdx;
    NP* out =  lookup(tex[offset]);
    return out;
}

QTex<float4> ** QMultiFilm::choose_tex(int pmtcatIdx){

    QTex<float4> **tex = nullptr;
    switch(pmtcatIdx){
        case 0: tex = tex_nnvt_normal ; break ; 
        case 1: tex = tex_hama        ; break ;
        case 2: tex = tex_nnvt_highqe ; break ;
    }      

    return tex;
}


NP* QMultiFilm::lookup( QTex<float4> *tex  )
{
    unsigned width = tex->width ; 
    unsigned height = tex->height; 
    unsigned num_lookup = width*height ; 
    //   unsigned payload = 4 ;

    LOG(LEVEL)
        << " width " << width
        << " height " << height
        << " lookup " << num_lookup
        ;


    NP* out = NP::Make<float>(height, width, 4 ); 
    float4* out_v = out->values<float4>(); 
    lookup( tex,out_v , num_lookup, width, height); 

    return out ; 
}

void QMultiFilm::lookup( QTex<float4> *tex, float4* lookup, unsigned num_lookup, unsigned width, unsigned height)
{
    LOG(LEVEL) << "[" ; 
    dim3 numBlocks ; 
    dim3 threadsPerBlock ; 
    configureLaunch( numBlocks, threadsPerBlock, width, height ); 

    size_t size = width * height * sizeof(float4) ; 

    LOG(LEVEL) 
        << " num_lookup " << num_lookup
        << " width " << width 
        << " height " << height

        << " size " << size 
        << " tex->texObj " << tex->texObj
        << " tex->meta " << tex->meta
        << " tex->d_meta " << tex->d_meta
        ; 

    float4* d_lookup = nullptr ;  
    QUDA_CHECK( cudaMalloc(reinterpret_cast<void**>( &d_lookup ), size )); 

    LOG(LEVEL)
        <<" QMultiFilm_lookup (";
    QMultiFilm_lookup(numBlocks, threadsPerBlock, tex->texObj, tex->d_meta, d_lookup, num_lookup, width, height);  

    LOG(LEVEL)
        <<" QMultiFilm_lookup )";
    QUDA_CHECK( cudaMemcpy(reinterpret_cast<void*>( lookup ), d_lookup, size, cudaMemcpyDeviceToHost )); 
    QUDA_CHECK( cudaFree(d_lookup) ); 

    cudaDeviceSynchronize();

    dump(lookup , num_lookup);

    LOG(LEVEL) << "]" ; 
}


void QMultiFilm::dump( float4* lookup, unsigned num_lookup, unsigned edgeitems  )
{
    LOG(LEVEL); 
    for(unsigned i=0 ; i < num_lookup ; i++)
    {
        if( i < edgeitems || i > num_lookup - edgeitems )
            std::cout 
                << std::setw(6) << i 
                << std::setw(10) << std::fixed << std::setprecision(3) << lookup[i].x
                << std::setw(10) << std::fixed << std::setprecision(3) << lookup[i].y
                << std::setw(10) << std::fixed << std::setprecision(3) << lookup[i].z
                << std::setw(10) << std::fixed << std::setprecision(3) << lookup[i].w 
                << std::endl 
                ; 
    }
}

/* */
extern "C" void QMultiFilm_mock_lookup(dim3 numBlocks, dim3 threadsPerBlock, qmultifilm* d_multifilm, quad2* d_input, float4* d_out, unsigned num_lookup, unsigned width, unsigned height); 

NP* QMultiFilm::mock_lookup( NP * input_arr )
{
    assert(input_arr->has_shape(128,256,2,4));

    unsigned height = input_arr->shape[0]; 
    unsigned width = input_arr->shape[1] ; 
    unsigned num_lookup = width*height ; 

    LOG(LEVEL)
        << " width " << width
        << " height " << height
        << " lookup " << num_lookup
        ;

    //upload input_array
    quad2* qd2 = (quad2*)input_arr->values<float>();
    quad2* d_input = QU::UploadArray<quad2>(qd2, num_lookup,"multifilm_mock_lookup");

    //malloc for output array 
    NP* out = NP::Make<float>(height, width, 4 ); 
    float4* h_out = out->values<float4>();
    float4* d_out = nullptr; 

    size_t size = num_lookup*sizeof(float4);
    QUDA_CHECK( cudaMalloc(reinterpret_cast<void**>( &d_out ), size )); 

    mock_lookup( d_input, d_out , num_lookup , width, height); 

    QUDA_CHECK( cudaMemcpy(reinterpret_cast<void*>(h_out), d_out, size, cudaMemcpyDeviceToHost )); 
    QUDA_CHECK( cudaFree(d_out) ); 
    QUDA_CHECK( cudaFree(d_input) ); 
    cudaDeviceSynchronize();

    dump(h_out, num_lookup);
    return out ; 
}


void* QMultiFilm::mock_lookup( quad2* d_input, float4* d_out, unsigned num_lookup, unsigned width, unsigned height ){

    dim3 numBlocks ; 
    dim3 threadsPerBlock ; 
    configureLaunch( numBlocks, threadsPerBlock, width, height ); 

    QMultiFilm_mock_lookup(numBlocks, threadsPerBlock, d_multifilm, d_input, d_out, num_lookup, width, height);  
}
