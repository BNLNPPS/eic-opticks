#include "NPY.hpp"
#include "OpticksColors.hh"
#include "OColors.hh"

#ifdef OLD_WAY
#else
#include "OConfig.hh"
#endif



#include "PLOG.hh"
// trace/debug/info/warning/error/fatal


OColors::OColors(optix::Context& ctx, OpticksColors* colors)
           : 
           m_context(ctx),
           m_colors(colors)
{
}


void OColors::convert()
{
    NPY<unsigned char>* npy = m_colors->getCompositeBuffer();

    if(npy == NULL)
    {
        LOG(warning) << "OColors::convert SKIP no composite color buffer " ; 
        return ;  
    }

    nuvec4 cd = m_colors->getCompositeDomain();


#ifdef OLD_WAY
    optix::TextureSampler tex = makeColorSampler(npy);
#else

    unsigned int n = npy->getNumItems();
    assert(npy->hasShape(n,4));

    unsigned int nx = n ;  
    unsigned int ny = 1 ;

    optix::Buffer colorBuffer = m_context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_BYTE4, nx, ny );
    memcpy( colorBuffer->map(), npy->getBytes(), npy->getNumBytes(0) );
    colorBuffer->unmap(); 

    optix::TextureSampler tex = m_context->createTextureSampler();
    OConfig::configureSampler(tex, colorBuffer);

    tex->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT);  

#endif
    m_context["color_texture"]->setTextureSampler(tex);
    m_context["color_domain"]->setUint(optix::make_uint4(cd.x, cd.y, cd.z, cd.w));

    // see cu/color_lookup.h
}








#ifdef OLD_WAY
optix::TextureSampler OColors::makeColorSampler(NPY<unsigned char>* buffer)
{
    unsigned int n = buffer->getNumItems();
    assert(buffer->hasShape(n,4));

    // the move from GBuffer (ncol, 1) to NPY<unsigned char> (ncol, 4)
    // just changes the "width" not the length, so should nx should stay = n (and not change to n*4)

    unsigned int nx = n ;  
    unsigned int ny = 1 ;

    LOG(debug) << "OColors::makeColorSampler "
              << " nx " << nx 
              << " ny " << ny  ;

    optix::TextureSampler sampler = makeSampler(buffer, RT_FORMAT_UNSIGNED_BYTE4, nx, ny);
    return sampler ; 
}

optix::TextureSampler OColors::makeSampler(NPY<unsigned char>* buffer, RTformat format, unsigned int nx, unsigned int ny)
{
    // TODO: avoid duplication between this and OPropertyLib

    optix::Buffer optixBuffer = m_context->createBuffer(RT_BUFFER_INPUT, format, nx, ny );
    memcpy( optixBuffer->map(), buffer->getPointer(), buffer->getNumBytes() );
    optixBuffer->unmap(); 

    optix::TextureSampler sampler = m_context->createTextureSampler();

    sampler->setWrapMode(0, RT_WRAP_CLAMP_TO_EDGE ); 
    sampler->setWrapMode(1, RT_WRAP_CLAMP_TO_EDGE );

    RTfiltermode minification = RT_FILTER_LINEAR ;
    RTfiltermode magnification = RT_FILTER_LINEAR ;
    RTfiltermode mipmapping = RT_FILTER_NONE ;
    sampler->setFilteringModes(minification, magnification, mipmapping);

    sampler->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT);  

    //RTtextureindexmode indexingmode = RT_TEXTURE_INDEX_ARRAY_INDEX ;  // by inspection : zero based array index offset by 0.5
    RTtextureindexmode indexingmode = RT_TEXTURE_INDEX_NORMALIZED_COORDINATES ; // needed by OptiX 400 ? see OptiX_400 pdf p17 
    sampler->setIndexingMode(indexingmode);  

    sampler->setMaxAnisotropy(1.0f);  
    sampler->setMipLevelCount(1u);     
    sampler->setArraySize(1u);        

    unsigned int texture_array_idx = 0u ;
    unsigned int mip_level = 0u ; 
    sampler->setBuffer(texture_array_idx, mip_level, optixBuffer);

    return sampler ; 
}

#endif

