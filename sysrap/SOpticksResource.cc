#include <cassert>
#include <sstream>
#include <cstdlib>

#include "SSys.hh"
#include "SStr.hh"
#include "SPath.hh"
#include "SProc.hh"
#include "SOpticksResource.hh"
#include "SOpticksKey.hh"
#include "PLOG.hh"

#include "NP.hh"


const plog::Severity SOpticksResource::LEVEL = PLOG::EnvLevel("SOpticksResource", "DEBUG"); 

const char* SOpticksResource::GEOCACHE_PREFIX_KEY = "OPTICKS_GEOCACHE_PREFIX" ; 
const char* SOpticksResource::RNGCACHE_PREFIX_KEY = "OPTICKS_RNGCACHE_PREFIX" ; 
const char* SOpticksResource::USERCACHE_PREFIX_KEY = "OPTICKS_USERCACHE_PREFIX" ; 
const char* SOpticksResource::PRECOOKED_PREFIX_KEY = "OPTICKS_PRECOOKED_PREFIX" ; 

const char* SOpticksResource::MakeUserDir(const char* sub) 
{
    return SPath::Resolve("$HOME", sub, DIRPATH) ; 
}


/**
SOpticksResource::ResolveUserPrefix
----------------------------------------

1. sensitive to envvars :  OPTICKS_GEOCACHE_PREFIX OPTICKS_RNGCACHE_PREFIX OPTICKS_USERCACHE_PREFIX OPTICKS_PRECOOKED_PREFIX
2. if envvar not defined defaults to $HOME/.opticks 
3. the envvar is subsequently internally set for consistency 

NB changes to layout need to be done in several places C++/bash/py::

   ana/geocache.bash
   ana/key.py
   boostrap/BOpticksResource.cc
   sysrap/SOpticksResource.cc

**/

const char* SOpticksResource::ResolveUserPrefix(const char* envkey, bool envset)  // static
{
    const char* evalue = SSys::getenvvar(envkey);    
    const char* prefix = evalue == nullptr ?  MakeUserDir(".opticks") : evalue ; 
    if(envset)
    {
        bool overwrite = true ; 
        int rc = SSys::setenvvar(envkey, prefix, overwrite );
        LOG(LEVEL) 
             << " envkey " << envkey
             << " prefix " << prefix
             << " rc " << rc
             ;    
        assert( rc == 0 );            
    } 
    return prefix ; 
}

const char* SOpticksResource::ResolveGeoCachePrefix() { return ResolveUserPrefix(GEOCACHE_PREFIX_KEY, true) ; }
const char* SOpticksResource::ResolveRngCachePrefix() { return ResolveUserPrefix(RNGCACHE_PREFIX_KEY, true) ; }
const char* SOpticksResource::ResolveUserCachePrefix(){ return ResolveUserPrefix(USERCACHE_PREFIX_KEY, true) ; }
const char* SOpticksResource::ResolvePrecookedPrefix(){ return ResolveUserPrefix(PRECOOKED_PREFIX_KEY, true) ; }

const char* SOpticksResource::GeocacheDir(){        return SPath::Resolve(ResolveGeoCachePrefix(), "geocache", NOOP); }
const char* SOpticksResource::GeocacheScriptPath(){ return SPath::Resolve(GeocacheDir(), "geocache.sh", NOOP); }

const char* SOpticksResource::RNGCacheDir(){    return SPath::Resolve(ResolveRngCachePrefix(), "rngcache", NOOP); }
const char* SOpticksResource::RNGDir(){         return SPath::Resolve(RNGCacheDir(), "RNG", NOOP); }
const char* SOpticksResource::RuncacheDir(){    return SPath::Resolve(ResolveUserCachePrefix(), "runcache", NOOP); }
const char* SOpticksResource::PrecookedDir(){   return SPath::Resolve(ResolvePrecookedPrefix(), "precooked", NOOP); }
const char* SOpticksResource::DefaultOutputDir(){  return SPath::Resolve("$TMP", SProc::ExecutableName(), SSys::getenvvar("GEOM"),  NOOP); }


std::string SOpticksResource::Dump()
{

    const char* geocache_prefix = ResolveGeoCachePrefix()  ;
    const char* rngcache_prefix = ResolveRngCachePrefix() ; 
    const char* usercache_prefix = ResolveUserCachePrefix() ;
    const char* precooked_prefix = ResolvePrecookedPrefix() ;
    const char* geocache_dir = GeocacheDir() ; 
    const char* geocache_scriptpath = GeocacheScriptPath() ; 
    const char* rngcache_dir = RNGCacheDir() ; 
    const char* rng_dir = RNGDir() ; 
    const char* precooked_dir = PrecookedDir() ; 
    const char* outputdir = DefaultOutputDir(); 
    const char* runcache_dir = RuncacheDir() ; 

    bool setkey = true ; 
    const char* idpath = IDPath(setkey) ; 
    const char* cgdir = CGDir(setkey) ; 
    const char* cfbase = CFBase(); 
    const char* cfbase_alt = CFBaseAlt(); 
    const char* cfbase_fg = CFBaseFromGEOM(); 


    std::stringstream ss ; 
    ss 
        << std::endl 
        << "GEOCACHE_PREFIX_KEY                        " << GEOCACHE_PREFIX_KEY  << std::endl 
        << "RNGCACHE_PREFIX_KEY                        " << RNGCACHE_PREFIX_KEY  << std::endl 
        << "USERCACHE_PREFIX_KEY                       " << USERCACHE_PREFIX_KEY  << std::endl 
        << "SOpticksResource::ResolveGeoCachePrefix()  " << ( geocache_prefix ? geocache_prefix : "-" ) << std::endl 
        << "SOpticksResource::ResolveRngCachePrefix()  " << ( rngcache_prefix ? rngcache_prefix : "-" )  << std::endl 
        << "SOpticksResource::ResolveUserCachePrefix() " << ( usercache_prefix ? usercache_prefix : "-" )  << std::endl 
        << "SOpticksResource::ResolvePrecookedPrefix() " << ( precooked_prefix ? precooked_prefix : "-" )  << std::endl 
        << "SOpticksResource::GeocacheDir()            " << ( geocache_dir  ? geocache_dir : "-" ) << std::endl 
        << "SOpticksResource::GeocacheScriptPath()     " << ( geocache_scriptpath ? geocache_scriptpath : "-" ) << std::endl 
        << "SOpticksResource::RNGCacheDir()            " << ( rngcache_dir  ? rngcache_dir : "-" )  << std::endl 
        << "SOpticksResource::RNGDir()                 " << ( rng_dir ? rng_dir : "-" )  << std::endl 
        << "SOpticksResource::PrecookedDir()           " << ( precooked_dir ? precooked_dir : "-" )  << std::endl 
        << "SOpticksResource::DefaultOutputDir()       " << ( outputdir ? outputdir : "-" ) << std::endl 
        << "SOpticksResource::RuncacheDir()            " << ( runcache_dir ? runcache_dir : "-" )  << std::endl 
        << "SOpticksResource::IDPath(true)             " << ( idpath ? idpath : "-" ) << std::endl  
        << "SOpticksResource::CGDir(true)              " << ( cgdir ? cgdir : "-" )  << std::endl 
        << "SOpticksResource::CFBase()                 " << ( cfbase ? cfbase : "-" ) << std::endl 
        << "SOpticksResource::CFBaseAlt()              " << ( cfbase_alt ? cfbase_alt : "-" ) << std::endl 
        << "SOpticksResource::CFBaseFromGEOM()         " << ( cfbase_fg ? cfbase_fg : "-" ) << std::endl 
        ;

    std::string s = ss.str(); 
    return s ; 
}

/**
SOpticksResource::IDPath
-------------------------

The default setkey:true means that the OPTICKS_KEY from the environment is used. 

**/

const char* SOpticksResource::IDPath(bool setkey) 
{  
    if(setkey)
    {
        SOpticksKey::SetKey(); 
    }
    const char* base = GeocacheDir(); 
    const SOpticksKey* key = SOpticksKey::GetKey() ; 
    return key == nullptr ? nullptr : key->getIdPath(base)  ;  
}

const NP* SOpticksResource::IDLoad(const char* relpath)
{
    const char* idpath = SOpticksResource::IDPath();
    return NP::Load(idpath, relpath) ; 
}


const char* SOpticksResource::CGDir_NAME = "CSG_GGeo" ; 
const char* SOpticksResource::CGDir(bool setkey){ return CGDir_(setkey, CGDir_NAME) ; }
const char* SOpticksResource::CGDir_(bool setkey, const char* rel)  
{
    const char* idpath = IDPath(setkey) ; 
    assert( idpath ); 
    return SPath::Resolve( idpath, rel , NOOP ); 
}

const char* SOpticksResource::CGDir_NAME_Alt = "CSG_GGeo_Alt" ; 
const char* SOpticksResource::CGDirAlt(bool setkey){ return CGDir_(setkey, CGDir_NAME_Alt) ; }



/**
SOpticksResource::CFBase
--------------------------

Return the directory path within which the CSGFoundry directory 
will be expected.  The path returned dependes on 
environment variables : CFBASE, OPTICKS_KEY, OPTICKS_GEOCACHE_PREFIX

Precedence order:

1. CFBASE envvar values directly providing CFBASE directory 

2. CFBASE directory derived from OPTICKS_KEY and OPTICKS_GEOCACHE_PREFIX 

When the *ekey* envvar (default CFBASE) is defined its 
value is returned otherwise the CFDir obtained from the 
OPTICKS_KEY is returned.  
**/


const char* SOpticksResource::CFBase()
{
    const char* cfbase = SSys::getenvvar("CFBASE") ; 
    if( cfbase == nullptr )
    {
        bool setkey = true ; 
        cfbase = CGDir(setkey); 
    }
    return cfbase ; 
}

const char* SOpticksResource::CFBaseAlt()
{
    const char* cfbase = SSys::getenvvar("CFBASE_ALT") ; 
    if( cfbase == nullptr )
    {
        bool setkey = true ; 
        cfbase = CGDirAlt(setkey); 
    }
    return cfbase ; 
}



/**
SOpticksResource::CFBaseFromGEOM
----------------------------------

Construct a CFBASE directory from GEOM envvar.

Have stopped using this automatically from SOpticksResource::CFBase
as GEOM envvar is too commonly used that this can kick in unexpectedly. 

GEOM envvar value such as AltXJfixtureConstruction_FirstSuffix_XY leads for CFBASE folder 
such as /tmp/$USER/opticks/GeoChain_Darwin/AltXJfixtureConstruction_FirstSuffix

**/

const char* SOpticksResource::CFBaseFromGEOM()
{
    const char* cfbase = nullptr ; 
    const char* geom = SSys::getenvvar("GEOM"); 
    if( geom != nullptr )
    {
        const char* gcn =  SStr::HeadLast(geom, '_'); 
#ifdef __APPLE__
        const char* rel = "GeoChain_Darwin" ; 
#else
        const char* rel = "GeoChain" ; 
#endif
        cfbase = SPath::Resolve("$TMP", rel, gcn, NOOP  );    
    }
    return cfbase ;  
}


const char* SOpticksResource::KEYS = "IDPath CFBase CFBaseAlt GeocacheDir RuncacheDir RNGDir PrecookedDir DefaultOutputDir" ; 

/**
SOpticksResource::Get
-----------------------

The below keys have default values derived from the OPTICKS_KEY envvars, however
envvars with the same keys can be used to override these defaults. 

+-------------------------+-----------------------------------------------------+
| key                     |  notes                                              |
+=========================+=====================================================+
|   IDPath                |                                                     |
+-------------------------+-----------------------------------------------------+
|   CFBase                |                                                     |
+-------------------------+-----------------------------------------------------+
|   CFBaseAlt             |                                                     |
+-------------------------+-----------------------------------------------------+
|   GeocacheDir           |                                                     |
+-------------------------+-----------------------------------------------------+
|   RuncacheDir           |                                                     |
+-------------------------+-----------------------------------------------------+
|   RNGDir                |                                                     |
+-------------------------+-----------------------------------------------------+
|   PrecookedDir          |                                                     |
+-------------------------+-----------------------------------------------------+
|   DefaultOutputDir      |                                                     |
+-------------------------+-----------------------------------------------------+





**/
const char* SOpticksResource::Get(const char* key) // static
{
    const char* tok = getenv(key) ; 
    if(tok) return tok ;  

    if(      strcmp(key, "IDPath")==0)      tok = SOpticksResource::IDPath(); 
    else if( strcmp(key, "CFBase")==0)      tok = SOpticksResource::CFBase(); 
    else if( strcmp(key, "CFBaseAlt")==0)   tok = SOpticksResource::CFBaseAlt(); 
    else if( strcmp(key, "GeocacheDir")==0) tok = SOpticksResource::GeocacheDir(); 
    else if( strcmp(key, "RuncacheDir")==0) tok = SOpticksResource::RuncacheDir(); 
    else if( strcmp(key, "RNGDir")==0)      tok = SOpticksResource::RNGDir(); 
    else if( strcmp(key, "PrecookedDir")==0) tok = SOpticksResource::PrecookedDir(); 
    else if( strcmp(key, "DefaultOutputDir")==0)    tok = SOpticksResource::DefaultOutputDir(); 
    return tok ;  
}


std::string SOpticksResource::Desc() 
{
    std::vector<std::string> keys ; 
    SStr::Split(KEYS, ' ', keys); 

    std::stringstream ss ; 
    ss << "SOpticksResource::Desc" << std::endl ; 
    for(unsigned i=0 ; i < keys.size() ; i++ ) 
    {
        const char* key = keys[i].c_str() ; 
        std::string lab = SStr::Format("SOpticksResource::Get(\"%s\") ", key) ; 
        const char* val = Get(key); 
        ss 
            << std::setw(70) << lab.c_str() 
            << " : "
            << ( val ? val : "-" ) 
            << std::endl 
            ;
    }


    

    const char* gdml_key = "$IDPath/origin_GDMLKludge.gdml" ; 

    for(int pass=0 ; pass < 5 ; pass++)
    {
        switch(pass)
        {
           case 1: SSys::setenvvar("IDPath", "/some/override/IDPath/via/envvar", true );  break ; 
           case 2: SSys::unsetenv("IDPath") ; break ; 
           case 3: SSys::setenvvar("IDPath", "/another/override/IDPath/via/envvar", true );  break ; 
           case 4: SSys::unsetenv("IDPath") ; break ; 
        }
        const char* gdml_path = SPath::Resolve(gdml_key, NOOP );
        ss 
            << std::setw(70) << gdml_key 
            << " : "
            << ( gdml_path ? gdml_path : "-" )
            << std::endl 
            ;
    }



    std::string s = ss.str(); 
    return s ; 
}




