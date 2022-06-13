#include <sstream>
#include <cstring>
#include <cassert>
#include <iostream>
#include <iomanip>

#include "SSys.hh"
#include "SPath.hh"
#include "SEventConfig.hh"
#include "SRG.h"
#include "SComp.h"
#include "OpticksPhoton.hh"


int SEventConfig::_MaxGenstepDefault = 1000*K ; 
int SEventConfig::_MaxBounceDefault = 9 ; 
int SEventConfig::_MaxRecordDefault = 0 ; 
int SEventConfig::_MaxRecDefault = 0 ; 
int SEventConfig::_MaxSeqDefault = 0 ; 
float SEventConfig::_MaxExtentDefault = 1000.f ;  // mm  : domain compression used by *rec*
float SEventConfig::_MaxTimeDefault = 10.f ; // ns 
const char* SEventConfig::_OutFoldDefault = "$TMP" ; 
const char* SEventConfig::_OutNameDefault = nullptr ; 
const char* SEventConfig::_RGModeDefault = "simulate" ; 
const char* SEventConfig::_HitMaskDefault = "SD" ; 

#ifdef __APPLE__
int  SEventConfig::_MaxPhotonDefault = 1*M ; 
int  SEventConfig::_MaxSimtraceDefault = 1*M ; 
#else
int  SEventConfig::_MaxPhotonDefault = 3*M ; 
int  SEventConfig::_MaxSimtraceDefault = 3*M ; 
#endif

const char* SEventConfig::_CompMaskDefault = SComp::ALL_ ; 
float SEventConfig::_PropagateEpsilonDefault = 0.05f ; 
const char* SEventConfig::_InputPhotonDefault = nullptr ; 



int SEventConfig::_MaxGenstep   = SSys::getenvint(kMaxGenstep,  _MaxGenstepDefault ) ; 
int SEventConfig::_MaxPhoton    = SSys::getenvint(kMaxPhoton,   _MaxPhotonDefault ) ; 
int SEventConfig::_MaxSimtrace  = SSys::getenvint(kMaxSimtrace,   _MaxSimtraceDefault ) ; 
int SEventConfig::_MaxBounce    = SSys::getenvint(kMaxBounce, _MaxBounceDefault ) ; 
int SEventConfig::_MaxRecord    = SSys::getenvint(kMaxRecord, _MaxRecordDefault ) ;    
int SEventConfig::_MaxRec       = SSys::getenvint(kMaxRec, _MaxRecDefault ) ;   
int SEventConfig::_MaxSeq       = SSys::getenvint(kMaxSeq,  _MaxSeqDefault ) ;  
float SEventConfig::_MaxExtent  = SSys::getenvfloat(kMaxExtent, _MaxExtentDefault );  
float SEventConfig::_MaxTime    = SSys::getenvfloat(kMaxTime,   _MaxTimeDefault );    // ns
const char* SEventConfig::_OutFold = SSys::getenvvar(kOutFold, _OutFoldDefault ); 
const char* SEventConfig::_OutName = SSys::getenvvar(kOutName, _OutNameDefault ); 
int SEventConfig::_RGMode = SRG::Type(SSys::getenvvar(kRGMode, _RGModeDefault)) ;    
unsigned SEventConfig::_HitMask  = OpticksPhoton::GetHitMask(SSys::getenvvar(kHitMask, _HitMaskDefault )) ;   
unsigned SEventConfig::_CompMask  = SComp::Mask(SSys::getenvvar(kCompMask, _CompMaskDefault )) ;   
float SEventConfig::_PropagateEpsilon = SSys::getenvfloat(kPropagateEpsilon, _PropagateEpsilonDefault ) ; 
const char* SEventConfig::_InputPhoton = SSys::getenvvar(kInputPhoton, _InputPhotonDefault ); 


int SEventConfig::MaxGenstep(){  return _MaxGenstep ; }
int SEventConfig::MaxPhoton(){   return _MaxPhoton ; }
int SEventConfig::MaxSimtrace(){   return _MaxSimtrace ; }
int SEventConfig::MaxBounce(){   return _MaxBounce ; }
int SEventConfig::MaxRecord(){   return _MaxRecord ; }
int SEventConfig::MaxRec(){      return _MaxRec ; }
int SEventConfig::MaxSeq(){      return _MaxSeq ; }
float SEventConfig::MaxExtent(){ return _MaxExtent ; }
float SEventConfig::MaxTime(){   return _MaxTime ; }
const char* SEventConfig::OutFold(){   return _OutFold ; }
const char* SEventConfig::OutName(){   return _OutName ; }
int SEventConfig::RGMode(){  return _RGMode ; } 
unsigned SEventConfig::HitMask(){     return _HitMask ; }
unsigned SEventConfig::CompMask(){  return _CompMask; } 
float SEventConfig::PropagateEpsilon(){ return _PropagateEpsilon ; }
const char* SEventConfig::InputPhoton(){   return _InputPhoton ; }



void SEventConfig::SetMaxGenstep(int max_genstep){ _MaxGenstep = max_genstep ; Check() ; }
void SEventConfig::SetMaxPhoton( int max_photon){  _MaxPhoton  = max_photon  ; Check() ; }
void SEventConfig::SetMaxSimtrace( int max_simtrace){  _MaxSimtrace  = max_simtrace  ; Check() ; }
void SEventConfig::SetMaxBounce( int max_bounce){  _MaxBounce  = max_bounce  ; Check() ; }
void SEventConfig::SetMaxRecord( int max_record){  _MaxRecord  = max_record  ; Check() ; }
void SEventConfig::SetMaxRec(    int max_rec){     _MaxRec     = max_rec     ; Check() ; }
void SEventConfig::SetMaxSeq(    int max_seq){     _MaxSeq     = max_seq     ; Check() ; }
void SEventConfig::SetMaxExtent( float max_extent){ _MaxExtent = max_extent  ; Check() ; }
void SEventConfig::SetMaxTime(   float max_time){   _MaxTime = max_time  ; Check() ; }
void SEventConfig::SetOutFold(   const char* outfold){   _OutFold = outfold ? strdup(outfold) : nullptr ; Check() ; }
void SEventConfig::SetOutName(   const char* outname){   _OutName = outname ? strdup(outname) : nullptr ; Check() ; }
void SEventConfig::SetRGMode(   const char* rg_mode){   _RGMode = SRG::Type(rg_mode) ; Check() ; }
void SEventConfig::SetHitMask(const char* abrseq, char delim){  _HitMask = OpticksPhoton::GetHitMask(abrseq,delim) ; }
void SEventConfig::SetCompMask(const char* names, char delim){  _CompMask = SComp::Mask(names,delim) ; }
void SEventConfig::SetPropagateEpsilon(float eps){ _PropagateEpsilon = eps ; Check() ; }
void SEventConfig::SetInputPhoton(const char* ip){   _InputPhoton = ip ? strdup(ip) : nullptr ; Check() ; }



bool SEventConfig::IsRGModeRender(){   return RGMode() == SRG_RENDER   ; } 
bool SEventConfig::IsRGModeSimtrace(){ return RGMode() == SRG_SIMTRACE ; } 
bool SEventConfig::IsRGModeSimulate(){ return RGMode() == SRG_SIMULATE ; } 

const char* SEventConfig::RGModeLabel(){ return SRG::Name(_RGMode) ; }
std::string SEventConfig::HitMaskLabel(){  return OpticksPhoton::FlagMask( _HitMask ) ; }
std::string SEventConfig::CompMaskLabel(){ return SComp::Desc( _CompMask ) ; }


void SEventConfig::Check()
{
   assert( _MaxBounce >  0 && _MaxBounce <  16 ) ; 
   assert( _MaxRecord >= 0 && _MaxRecord <= 16 ) ; 
   assert( _MaxRec    >= 0 && _MaxRec    <= 16 ) ; 
   assert( _MaxSeq    >= 0 && _MaxSeq    <= 16 ) ; 
}

void SEventConfig::SetMax(int max_genstep_, int max_photon_, int max_bounce_, int max_record_, int max_rec_, int max_seq_ )
{ 
    SetMaxGenstep( max_genstep_ ); 
    SetMaxPhoton(  max_photon_  ); 
    SetMaxBounce(  max_bounce_  ); 
    SetMaxRecord(  max_record_  ); 
    SetMaxRec(     max_rec_  ); 
    SetMaxSeq(     max_seq_  ); 
}
  
std::string SEventConfig::Desc()
{
    std::stringstream ss ; 
    ss << "SEventConfig::Desc" << std::endl 
       << std::setw(25) << kMaxGenstep 
       << std::setw(20) << " MaxGenstep " << " : " << MaxGenstep() << std::endl 
       << std::setw(25) << kMaxPhoton 
       << std::setw(20) << " MaxPhoton " << " : " << MaxPhoton() << std::endl 
       << std::setw(25) << kMaxSimtrace 
       << std::setw(20) << " MaxSimtrace " << " : " << MaxSimtrace() << std::endl 
       << std::setw(25) << kMaxBounce
       << std::setw(20) << " MaxBounce " << " : " << MaxBounce() << std::endl 
       << std::setw(25) << kMaxRecord
       << std::setw(20) << " MaxRecord " << " : " << MaxRecord() << std::endl 
       << std::setw(25) << kMaxRec
       << std::setw(20) << " MaxRec " << " : " << MaxRec() << std::endl 
       << std::setw(25) << kMaxSeq
       << std::setw(20) << " MaxSeq " << " : " << MaxSeq() << std::endl 
       << std::setw(25) << kHitMask
       << std::setw(20) << " HitMask " << " : " << HitMask() << std::endl 
       << std::setw(25) << ""
       << std::setw(20) << " HitMaskLabel " << " : " << HitMaskLabel() << std::endl 
       << std::setw(25) << kMaxExtent
       << std::setw(20) << " MaxExtent " << " : " << MaxExtent() << std::endl 
       << std::setw(25) << kMaxTime
       << std::setw(20) << " MaxTime " << " : " << MaxTime() << std::endl 
       << std::setw(25) << kRGMode
       << std::setw(20) << " RGMode " << " : " << RGMode() << std::endl 
       << std::setw(25) << ""
       << std::setw(20) << " RGModeLabel " << " : " << RGModeLabel() << std::endl 
       << std::setw(25) << kCompMask
       << std::setw(20) << " CompMask " << " : " << CompMask() << std::endl 
       << std::setw(25) << ""
       << std::setw(20) << " CompMaskLabel " << " : " << CompMaskLabel() << std::endl 
       << std::setw(25) << kOutFold
       << std::setw(20) << " OutFold " << " : " << OutFold() << std::endl 
       << std::setw(25) << kOutName
       << std::setw(20) << " OutName " << " : " << ( OutName() ? OutName() : "-" )  << std::endl 
       << std::setw(25) << kPropagateEpsilon
       << std::setw(20) << " PropagateEpsilon " << " : " << std::fixed << std::setw(10) << std::setprecision(4) << PropagateEpsilon() << std::endl 
       << std::setw(25) << kInputPhoton
       << std::setw(20) << " InputPhoton " << " : " << ( InputPhoton() ? InputPhoton() : "-" )  << std::endl 
       ;
    std::string s = ss.str(); 
    return s ; 
}



const char* SEventConfig::OutDir()
{
    return SPath::Resolve( OutFold(), OutName(), DIRPATH ); 
}
const char* SEventConfig::OutPath( const char* stem, int index, const char* ext )
{
     return SPath::Make( OutFold(), OutName(), stem, index, ext, FILEPATH);  // HMM: an InPath would use NOOP to not create the dir
}



const char* SEventConfig::OutDir(const char* reldir)
{
    return SPath::Resolve( OutFold(), OutName(), reldir, DIRPATH ); 
}
const char* SEventConfig::OutPath( const char* reldir, const char* stem, int index, const char* ext )
{
     return SPath::Make( OutFold(), OutName(), reldir, stem, index, ext, FILEPATH ); 
}




