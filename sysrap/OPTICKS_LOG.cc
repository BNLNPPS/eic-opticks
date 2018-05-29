
#include <plog/Log.h>

#include "OPTICKS_LOG.hh"
#include "PLOG_INIT.hh"
       

void OPTICKS_LOG::Initialize(PLOG* instance, void* app1, void* app2 )
{
    // initialize all linked loggers and hookup the main logger

#ifdef WITH_SYSRAP
    SYSRAP_LOG::Initialize(instance->prefixlevel_parse( info, "SYSRAP"), app1, NULL );
#endif
#ifdef WITH_BRAP
    BRAP_LOG::Initialize(instance->prefixlevel_parse( info, "BRAP"), app1, NULL );
#endif
#ifdef WITH_NPY
    NPY_LOG::Initialize(instance->prefixlevel_parse( info, "NPY"), app1, NULL );
#endif
#ifdef WITH_OKCORE
    OKCORE_LOG::Initialize(instance->prefixlevel_parse( info, "OKCORE"), app1, NULL );
#endif
#ifdef WITH_GGEO
    GGEO_LOG::Initialize(instance->prefixlevel_parse( info, "GGEO"), app1, NULL );
#endif
#ifdef WITH_ASIRAP
    ASIRAP_LOG::Initialize(instance->prefixlevel_parse( info, "ASIRAP"), app1, NULL );
#endif
#ifdef WITH_MESHRAP
    MESHRAP_LOG::Initialize(instance->prefixlevel_parse( info, "MESHRAP"), app1, NULL );
#endif
#ifdef WITH_OKGEO
    OKGEO_LOG::Initialize(instance->prefixlevel_parse( info, "OKGEO"), app1, NULL );
#endif
#ifdef WITH_OGLRAP
    OGLRAP_LOG::Initialize(instance->prefixlevel_parse( info, "OGLRAP"), app1, NULL );
#endif
#ifdef WITH_OKGL
    OKGL_LOG::Initialize(instance->prefixlevel_parse( info, "OKGL"), app1, NULL );
#endif
#ifdef WITH_OK
    OK_LOG::Initialize(instance->prefixlevel_parse( info, "OK"), app1, NULL );
#endif
#ifdef WITH_OKG4
    OKG4_LOG::Initialize(instance->prefixlevel_parse( info, "OKG4"), app1, NULL );
#endif
#ifdef WITH_CUDARAP
    CUDARAP_LOG::Initialize(instance->prefixlevel_parse( info, "CUDARAP"), app1, NULL );
#endif
#ifdef WITH_THRAP
    THRAP_LOG::Initialize(instance->prefixlevel_parse( info, "THRAP"), app1, NULL );
#endif
#ifdef WITH_OXRAP
    OXRAP_LOG::Initialize(instance->prefixlevel_parse( info, "OXRAP"), app1, NULL );
#endif
#ifdef WITH_OKOP
    OKOP_LOG::Initialize(instance->prefixlevel_parse( info, "OKOP"), app1, NULL );
#endif
#ifdef WITH_CFG4
    CFG4_LOG::Initialize(instance->prefixlevel_parse( info, "CFG4"), app1, NULL );
#endif
#ifdef WITH_G4OK
    G4OK_LOG::Initialize(instance->prefixlevel_parse( info, "G4OK"), app1, NULL );
#endif
}

/*
gen-(){ cat << EOG
#ifdef WITH_${1}
    ${1}_LOG::Initialize(instance->prefixlevel_parse( info, "${1}"), app1, NULL );
#endif
EOG
}
gen()
{
    local tags="OKCONF SYSRAP BRAP NPY OKCORE GGEO ASIRAP MESHRAP OKGEO OGLRAP OKGL OK OKG4 CUDARAP THRAP OXRAP OKOP CFG4 G4OK"
    for tag in $tags ; do 
       gen- $tag
    done
} 
*/


void OPTICKS_LOG::Check(const char* msg)
{
    PLOG_CHECK(msg);
}

