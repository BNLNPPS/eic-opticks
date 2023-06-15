#include <iostream>
#include <iomanip>
#include <sstream>
#include <bitset>
#include <algorithm>
#include <cstring>

#include "ssys.h"

#include "SStr.hh"   // TODO: eliminate in favor of sstr.h
#include "sstr.h"

#include "SBit.hh"   // TODO: sbit.h 
#include "SGeoConfig.hh"
#include "SName.h"

#include "SLOG.hh"

const plog::Severity SGeoConfig::LEVEL = SLOG::EnvLevel("SGeoConfig", "DEBUG"); 


unsigned long long SGeoConfig::_EMM = SBit::FromEString(kEMM, "~0");  
const char* SGeoConfig::_ELVSelection   = ssys::getenvvar(kELVSelection, nullptr ); 
const char* SGeoConfig::_SolidSelection = ssys::getenvvar(kSolidSelection, nullptr ); 
const char* SGeoConfig::_FlightConfig   = ssys::getenvvar(kFlightConfig  , nullptr ); 
const char* SGeoConfig::_ArglistPath    = ssys::getenvvar(kArglistPath  , nullptr ); 
const char* SGeoConfig::_CXSkipLV       = ssys::getenvvar(kCXSkipLV  , nullptr ); 
const char* SGeoConfig::_CXSkipLV_IDXList = ssys::getenvvar(kCXSkipLV_IDXList, nullptr ); 

void SGeoConfig::SetELVSelection(  const char* es){  _ELVSelection   = es ? strdup(es) : nullptr ; }
void SGeoConfig::SetSolidSelection(const char* ss){  _SolidSelection = ss ? strdup(ss) : nullptr ; }
void SGeoConfig::SetFlightConfig(  const char* fc){  _FlightConfig   = fc ? strdup(fc) : nullptr ; }
void SGeoConfig::SetArglistPath(   const char* ap){  _ArglistPath    = ap ? strdup(ap) : nullptr ; }
void SGeoConfig::SetCXSkipLV(      const char* cx){  _CXSkipLV       = cx ? strdup(cx) : nullptr ; }

unsigned long long SGeoConfig::EnabledMergedMesh(){  return _EMM ; } 
const char* SGeoConfig::SolidSelection(){ return _SolidSelection ; }
const char* SGeoConfig::FlightConfig(){   return _FlightConfig ; }
const char* SGeoConfig::ArglistPath(){    return _ArglistPath ; }
const char* SGeoConfig::CXSkipLV(){       return _CXSkipLV ? _CXSkipLV : "" ; }
const char* SGeoConfig::CXSkipLV_IDXList(){  return _CXSkipLV_IDXList ? _CXSkipLV_IDXList : "" ; }

/**
SGeoConfig::ELVSelection
--------------------------
**/

const char* SGeoConfig::ELVSelection(){   return _ELVSelection ; }
const char* SGeoConfig::ELVSelection(const SName* id )
{
    const char* elv_selection_ = ELVSelection() ; 
    const char* elv = nullptr ; 
    char delim = ',' ; 

    if(VERBOSE) std::cerr 
        << "SGeoConfig::ELVSelection"
        << " elv_selection_ " << ( elv_selection_ ? elv_selection_ : "-" )
        << std::endl 
        ;

    if( elv_selection_ )
    {   
        const char* prefix = ELVPrefix(elv_selection_); 

        if(VERBOSE) std::cerr 
            << "SGeoConfig::ELVSelection"
            << " prefix " << ( prefix ? prefix : "-" )
            << " strlen(prefix) " << ( prefix ? strlen(prefix) : 0 ) 
            << std::endl
            ;

        bool has_names = id->hasNames(elv_selection_, delim, prefix );  

        if(VERBOSE) std::cerr
            << "SGeoConfig::ELVSelection" 
            << " after has_names " << ( has_names ? "Y" : "N" )
            << std::endl 
            ;

        if(has_names)
        {    
            elv = id->getIDXListFromNames(elv_selection_, delim, prefix ); 
        }    
        else 
        {    
            elv = elv_selection_ ;  // eg when just numbers 
        }    
    }    
    return elv ;  
}



/**
SGeoConfig::ELVPrefix
-----------------------

As the above ELVSelection has a problem for solids 
with names starting with "t" are starting to transition
to separate the modifier from the list of ints or solid names. 

TODO: adopt "t:" prefix for the tilde modifier. 

*/


const char* SGeoConfig::ELVPrefix(const char* elvarg)
{
    if(elvarg == nullptr) return nullptr ; 
    if(strlen(elvarg) == 0) return nullptr ;

    std::string prefix ; 
    if(sstr::StartsWith(elvarg, "t:") || sstr::StartsWith(elvarg, "~:"))
    {
        prefix.assign("t:") ; 
    }
    else if( sstr::StartsWith(elvarg,"t" ) || sstr::StartsWith(elvarg,"~") )
    {
        prefix.assign("t") ; 
    }
    return prefix.empty() ? nullptr : strdup(prefix.c_str()) ; 
}


std::string SGeoConfig::Desc()
{
    std::stringstream ss ; 
    ss << std::endl ; 
    ss << std::setw(25) << kEMM              << " : " << SBit::HexString(_EMM) << " 0x" << std::hex << _EMM << std::dec << std::endl ;
    ss << std::setw(25) << kELVSelection     << " : " << ( _ELVSelection   ? _ELVSelection   : "-" ) << std::endl ;    
    ss << std::setw(25) << kSolidSelection   << " : " << ( _SolidSelection ? _SolidSelection : "-" ) << std::endl ;    
    ss << std::setw(25) << kFlightConfig     << " : " << ( _FlightConfig   ? _FlightConfig   : "-" ) << std::endl ;    
    ss << std::setw(25) << kArglistPath      << " : " << ( _ArglistPath    ? _ArglistPath    : "-" ) << std::endl ;    
    ss << std::setw(25) << kCXSkipLV         << " : " << CXSkipLV() << std::endl ;    
    ss << std::setw(25) << kCXSkipLV_IDXList << " : " << CXSkipLV_IDXList() << std::endl ;    
    std::string s = ss.str(); 
    return s ; 
}

bool SGeoConfig::IsEnabledMergedMesh(unsigned mm) // static
{
    bool emptylistdefault = true ;   
    bool emm = true ;   
    if(mm < 64) 
    {   
        std::bitset<64> bs(_EMM); 
        emm = bs.count() == 0 ? emptylistdefault : bs[mm] ;   
    }   
    return emm ; 
}


std::string SGeoConfig::DescEMM()
{
    std::stringstream ss ; 
    for(unsigned i=0 ; i < 64 ; i++) 
    {
        bool emm = SGeoConfig::IsEnabledMergedMesh(i) ; 
        if(emm) ss << i << " " ; 
    }
    std::string s = ss.str(); 
    return s ; 
}


std::vector<std::string>*  SGeoConfig::Arglist() 
{
    return SStr::LoadList( _ArglistPath, '\n' );   // TODO: sstr
}



/**
SGeoConfig::CXSkipLV_IDXList
-----------------------------

Translates names in a comma delimited list into indices according to SName. 

**/
void SGeoConfig::SetCXSkipLV_IDXList(const SName* id)
{
    const char* cxskiplv_ = CXSkipLV() ; 
    bool has_names = cxskiplv_ ? id->hasNames(cxskiplv_ ) : false ; 
    _CXSkipLV_IDXList = has_names ? id->getIDXListFromNames(cxskiplv_, ',' ) : nullptr ; 
}

/**
SGeoConfig::IsCXSkipLV
------------------------

This controls mesh/solid skipping during GGeo to CSGFoundry 
translation as this is called from:

1. CSG_GGeo_Convert::CountSolidPrim
2. CSG_GGeo_Convert::convertSolid

For any skips to be applied the below SGeoConfig::GeometrySpecificSetup 
must have been called. 

For example this is used for long term skipping of Water///Water 
virtual solids that are only there for Geant4 performance reasons, 
and do nothing useful for Opticks. 

Note that ELVSelection does something similar to this, but 
that is applied at every CSGFoundry::Load providing dynamic prim selection. 
As maintaining consistency between results and geometry is problematic
with dynamic prim selection it is best to only use the dynamic approach 
for geometry render scanning to find bottlenecks. 

When creating longer lived geometry for analysis with multiple executables
it is more appropriate to use CXSkipLV to effect skipping at translation. 

**/

bool SGeoConfig::IsCXSkipLV(int lv) // static
{
    if( _CXSkipLV_IDXList == nullptr ) return false ; 
    std::vector<int> cxskip ;
    SStr::ISplit(_CXSkipLV_IDXList, cxskip, ',');   // TODO: sstr
    return std::count( cxskip.begin(), cxskip.end(), lv ) == 1 ;   
}


/**
SGeoConfig::GeometrySpecificSetup
-----------------------------------

TODO: compare GPU performance with and without these virtual skips  

This is invoked from:

1. CSG_GGeo_Convert::init prior to GGeo to CSGFoundry translation
2. argumentless CSGFoundry::Load 

The SName* id argument passes the meshnames (aka solid names)
allowing detection of where a geometry appears to be JUNO by 
the presence of a collection of solid names within it. 
If JUNO is detected some JUNO specific static method calls are made.  
This avoids repeating these settings in tests or fiddling 
with envvars to configure these things. 

Previously did something similar using metadata in geocache
or from the Opticks setup code within detector specific code. 
However do not want to require writing cache and prefer to minimize 
detector specific Opticks setup code as it is much easier 
to test in isolation than as an appendage to a detector framework. 

**/
void SGeoConfig::GeometrySpecificSetup(const SName* id)  // static
{
    const char* JUNO_names = "HamamatsuR12860sMask0x,HamamatsuR12860_PMT_20inch,NNVTMCPPMT_PMT_20inch" ;  
    bool JUNO_detected = id->hasNames(JUNO_names); 
    LOG(LEVEL) << " JUNO_detected " << JUNO_detected ; 
    if(JUNO_detected)
    {
        const char* skips = "NNVTMCPPMTsMask_virtual0x,HamamatsuR12860sMask_virtual0x,mask_PMT_20inch_vetosMask_virtual0x" ;
        SetCXSkipLV(skips); 
        SetCXSkipLV_IDXList(id); 
    
        // USING dynamic ELVSelection here would be inappropriate : as dynamic selection 
        // means the persisted geometry does not match the used geometry.   
    }
}


