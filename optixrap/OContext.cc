#include <iomanip>
#include <sstream>

#include <cuda.h>
#include <cuda_runtime_api.h>
// brap-
#include "BTimer.hh"

// npy-
#include "NGLM.hpp"
#include "NPY.hpp"
#include "GLMFormat.hpp"

// optixrap-
#include "OTimes.hh"
#include "OConfig.hh"
#include "OContext.hh"

#include "PLOG.hh"
using namespace optix ; 

const char* OContext::COMPUTE_ = "COMPUTE" ; 
const char* OContext::INTEROP_ = "INTEROP" ; 

const char* OContext::getModeName()
{
    switch(m_mode)
    {
       case COMPUTE:return COMPUTE_ ; break ; 
       case INTEROP:return INTEROP_ ; break ; 
    }
    assert(0);
}


OContext::OContext(optix::Context context, Mode_t mode, bool with_top) 
    : 
    m_context(context),
    m_mode(mode),
    m_debug_photon(-1),
    m_entry(0),
    m_closed(false),
    m_with_top(with_top)
{
    init();
}

optix::Context OContext::getContext()
{
     return m_context ; 
}

optix::Context& OContext::getContextRef()
{
     return m_context ; 
}


optix::Group OContext::getTop()
{
     return m_top ; 
}
unsigned int OContext::getNumRayType()
{
    return e_rayTypeCount ;
}


void OContext::setDebugPhoton(unsigned int debug_photon)
{
    m_debug_photon = debug_photon ; 
}
unsigned int OContext::getDebugPhoton()
{
    return m_debug_photon ; 
}


OContext::Mode_t OContext::getMode()
{
    return m_mode ; 
}


bool OContext::isCompute()
{
    return m_mode == COMPUTE ; 
}
bool OContext::isInterop()
{
    return m_mode == INTEROP ; 
}







void OContext::setStackSize(unsigned int stacksize)
{
    LOG(debug) << "OContext::setStackSize " << stacksize ;  
    m_context->setStackSize(stacksize);
}

void OContext::setPrintIndex(const std::string& pindex)
{
    LOG(debug) << "OContext::setPrintIndex " << pindex ;  
    if(!pindex.empty())
    {
        glm::ivec3 idx = givec3(pindex);
        LOG(debug) << "OContext::setPrintIndex " 
                  << pindex
                  << " idx " << gformat(idx) 
                   ;  
        m_context->setPrintLaunchIndex(idx.x, idx.y, idx.z);
    }
}


void OContext::init()
{
    m_cfg = new OConfig(m_context);

    m_context->setPrintEnabled(true);
    m_context->setPrintBufferSize(8192);
    //m_context->setPrintLaunchIndex(0,0,0);

    unsigned int num_ray_type = getNumRayType() ;

    m_context->setRayTypeCount( num_ray_type );   // more static than entry type count


    if(m_with_top)
    {
        m_top = m_context->createGroup();
        m_context[ "top_object" ]->set( m_top );
    }

    LOG(info) << "OContext::init " 
              << " mode " << getModeName()
              << " num_ray_type " << num_ray_type 
              ; 
}


void OContext::cleanUp()
{
    m_context->destroy();
    m_context = 0;
}




optix::Program OContext::createProgram(const char* filename, const char* progname )
{
    return m_cfg->createProgram(filename, progname);
}


unsigned int OContext::addEntry(const char* filename, const char* raygen, const char* exception, bool defer)
{
    return m_cfg->addEntry(filename, raygen, exception, defer ); 
}
unsigned int OContext::addRayGenerationProgram( const char* filename, const char* progname, bool defer)
{
    return m_cfg->addRayGenerationProgram(filename, progname, defer);
}
unsigned int OContext::addExceptionProgram( const char* filename, const char* progname, bool defer)
{
    return m_cfg->addExceptionProgram(filename, progname, defer);
}

void OContext::setMissProgram( unsigned int index, const char* filename, const char* progname, bool defer )
{
    m_cfg->setMissProgram(index, filename, progname, defer);
}



void OContext::close()
{
    if(m_closed) return ; 

    m_closed = true ; 

    unsigned int num = m_cfg->getNumEntryPoint() ;

    LOG(info) << "OContext::close numEntryPoint " << num ; 

    m_context->setEntryPointCount( num );
  
    m_cfg->dump("OContext::close");

    m_cfg->apply();
}


void OContext::dump(const char* msg)
{
    m_cfg->dump(msg);
}
unsigned int OContext::getNumEntryPoint()
{
    return m_cfg->getNumEntryPoint();
}




void OContext::launch(unsigned int lmode, unsigned int entry, unsigned int width, unsigned int height, OTimes* times )
{
    if(!m_closed) close();

    LOG(info)<< "OContext::launch" 
              << " entry " << entry 
              << " width " << width 
              << " height " << height 
              ;


    if(times) times->count     += 1 ; 


    if(lmode & VALIDATE)
    {
        double t0, t1 ; 
        t0 = BTimer::RealTime();

        m_context->validate();

        t1 = BTimer::RealTime();
        if(times) times->validate  += t1 - t0 ;
    }

    if(lmode & COMPILE)
    {
        double t0, t1 ; 
        t0 = BTimer::RealTime();

        m_context->compile();

        t1 = BTimer::RealTime();
        if(times) times->compile  += t1 - t0 ;
    }


    if(lmode & PRELAUNCH)
    {
        double t0, t1 ; 
        t0 = BTimer::RealTime();

        m_context->launch( entry, 0, 0); 

        t1 = BTimer::RealTime();
        if(times) times->prelaunch  += t1 - t0 ;
    }


    if(lmode & LAUNCH)
    {
        double t0, t1 ; 
        t0 = BTimer::RealTime();

        m_context->launch( entry, width, height ); 

        t1 = BTimer::RealTime();
        if(times) times->launch  += t1 - t0 ;
    }

}





template <typename T>
void OContext::upload(optix::Buffer& buffer, NPY<T>* npy)
{
    unsigned int numBytes = npy->getNumBytes(0) ;

    LOG(info)<<"OContext::upload" 
             << " numBytes " << numBytes 
             << npy->description("upload")
             ;

    // memcpy( buffer->map(), npy->getBytes(), numBytes );
    // buffer->unmap(); 
    void* d_ptr = NULL;
    rtBufferGetDevicePointer(buffer->get(), 0, &d_ptr);
    cudaMemcpy(d_ptr, npy->getBytes(), numBytes, cudaMemcpyHostToDevice);
    buffer->markDirty();
}


template <typename T>
void OContext::download(optix::Buffer& buffer, NPY<T>* npy)
{
    unsigned int numBytes = npy->getNumBytes(0) ;
    LOG(info)<<"OContext::download" 
             << " numBytes " << numBytes 
             << npy->description("download")
             ;

    void* ptr = buffer->map() ; 
    npy->read( ptr );
    buffer->unmap(); 
}




template <typename T>
optix::Buffer OContext::createIOBuffer(NPY<T>* npy, const char* name, bool set_size)
{
    assert(npy);
    unsigned int ni = std::max(1u,npy->getShape(0));
    unsigned int nj = std::max(1u,npy->getShape(1));  
    unsigned int nk = std::max(1u,npy->getShape(2));  
    unsigned int nl = std::max(1u,npy->getShape(3));  

    bool compute = isCompute();
    bool interop = !compute ; 
    int buffer_id = npy ? npy->getBufferId() : -1 ;
    RTformat format = getFormat(npy->getType());

    std::stringstream ss ; 
    ss 
       << std::setw(10) << name
       << std::setw(20) << npy->getShapeString()
       << ( interop ? " INTEROP " : " COMPUTE " ) 
       << " id " << std::setw(4) << buffer_id
       << ( format == RT_FORMAT_USER ? " USER" : " QUAD"  )   
       ;

    std::string hdr = ss.str();

    if(interop)
    {
        if(!(buffer_id > -1))
            LOG(fatal) << "OContext::createIOBuffer CANNOT createBufferFromGLBO as not uploaded  "
                         << " name " << std::setw(20) << name
                         << " buffer_id " << buffer_id 
                         ; 
        assert(buffer_id > -1 );
    }


    Buffer buffer;
    if(interop)
        buffer = m_context->createBufferFromGLBO(RT_BUFFER_INPUT_OUTPUT, buffer_id);
    else
        buffer = m_context->createBuffer(RT_BUFFER_INPUT_OUTPUT | RT_BUFFER_COPY_ON_DIRTY);


    buffer->setFormat(format);  // must set format, before can set ElementSize

    unsigned int size ; 
    if(format == RT_FORMAT_USER)
    {
        buffer->setElementSize(sizeof(T));
        size = ni*nj*nk ; 
        LOG(info) << hdr
                  << " size (ijk) " << std::setw(10) << size 
                  << " elementsize " << sizeof(T)
                  ;
    }
    else
    {
        //size = ni*nj ;
        size = npy->getNumQuads() ;  
        LOG(info) << hdr 
                  << " size (gnq) " << std::setw(10) << size 
                  ;

    }


    if(set_size)
    {
        buffer->setSize(size); 
    }


    return buffer ; 
}

/*

2016-07-21 20:23:13.813 INFO  [1868636] [OContext::launch@212] OContext::launch entry 1 width 100000 height 1
libc++abi.dylib: terminating with uncaught exception of type optix::Exception: Invalid context (Details: Function "RTresult _rtContextValidate(RTcontext)" caught exception: Validation error: Buffer validation failed for 'genstep_buffer':
Validation error: Buffer dimensionality is not set, [4915247], [4915291])


*/





RTformat OContext::getFormat(NPYBase::Type_t type)
{
    RTformat format ; 
    switch(type)
    {
        case NPYBase::FLOAT:     format = RT_FORMAT_FLOAT4         ; break ; 
        case NPYBase::SHORT:     format = RT_FORMAT_SHORT4         ; break ; 
        case NPYBase::INT:       format = RT_FORMAT_INT4           ; break ; 
        case NPYBase::UINT:      format = RT_FORMAT_UNSIGNED_INT4  ; break ; 
        case NPYBase::CHAR:      format = RT_FORMAT_BYTE4          ; break ; 
        case NPYBase::UCHAR:     format = RT_FORMAT_UNSIGNED_BYTE4 ; break ; 
        case NPYBase::ULONGLONG: format = RT_FORMAT_USER           ; break ; 
        case NPYBase::DOUBLE:    format = RT_FORMAT_USER           ; break ; 
    }
    return format ; 
}




template OXRAP_API void OContext::upload<float>(optix::Buffer&, NPY<float>* );
template OXRAP_API void OContext::download<float>(optix::Buffer&, NPY<float>* );

template OXRAP_API void OContext::upload<short>(optix::Buffer&, NPY<short>* );
template OXRAP_API void OContext::download<short>(optix::Buffer&, NPY<short>* );

template OXRAP_API void OContext::upload<unsigned long long>(optix::Buffer&, NPY<unsigned long long>* );
template OXRAP_API void OContext::download<unsigned long long>(optix::Buffer&, NPY<unsigned long long>* );


template OXRAP_API optix::Buffer OContext::createIOBuffer(NPY<float>*, const char*, bool);
template OXRAP_API optix::Buffer OContext::createIOBuffer(NPY<short>*, const char*, bool);
template OXRAP_API optix::Buffer OContext::createIOBuffer(NPY<unsigned long long>*, const char*, bool);


