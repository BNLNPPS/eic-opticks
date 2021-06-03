/*
 * Copyright (c) 2019 Opticks Team. All Rights Reserved.
 *
 * This file is part of Opticks
 * (see https://bitbucket.org/simoncblyth/opticks).
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */

#pragma once

#include "CFG4_API_EXPORT.hh"
#include "G4ThreeVector.hh"
#include "G4StepStatus.hh"
#include "CStage.hh"

#include "CProcessSwitches.hh"
#include "plog/Severity.h"

#ifdef USE_CUSTOM_BOUNDARY
#include "DsG4OpBoundaryProcessStatus.h"
#else
#include "G4OpBoundaryProcess.hh"
#endif


#include <string>

class G4Event ; 
class G4Track ; 
class G4ProcessManager ; 
class G4Step ; 

struct CTrk ; 
struct CGenstep ; 
class OpticksEvent ; 
class Opticks ; 

/**
CG4Ctx
=======

Canonical m_ctx instance is ctor resident of CManager, this context is shared with::
 
    CEventAction
    CTrackingAction
    CSteppingAction 
    CRecorder

**/

struct CFG4_API CG4Ctx
{
    static const plog::Severity LEVEL ; 
    static const unsigned CK ; 
    static const unsigned SI ; 
    static const unsigned TO ; 

    Opticks* _ok ; 
    unsigned _mode ; 
    int   _pindex ; 
    bool  _print ; 

    // CG4::init
    bool _dbgrec ; 
    bool _dbgseq ; 
    bool _dbgzero ; 

    // CG4::initEvent
    int  _photons_per_g4event ;
    unsigned  _steps_per_photon  ;
    unsigned  _record_max ; 
    unsigned  _bounce_max ; 
    bool  _ok_event_init ;   // indicates that CG4Ctx::initEvent has been called configuring OpticksEvent recording 

    // CG4Ctx::setEvent
    G4Event* _event ; 
    int  _event_id ;
    int  _event_total ; 
    int  _event_track_count ; 
    int  _number_of_input_photons ; 

    // CG4Ctx::setGenstep
    char       _gentype ;   // 'C' 'S' 'T'
    CGenstep*  _genstep_c ; 
    CGenstep*  _genstep_s ; 
    CGenstep*  _genstep_t ; 

    void      setGenstep(char gentype, int num_photons, int offset);
    CGenstep* getGenstep(char gentype='?') const;

    // *_genstep_index* 
    //     starts at -1 and is reset to -1 by CG4Ctx::setEvent, incremented by CG4Ctx::BeginOfGenstep 
    //     giving a zero based local index of genstep within the event
    //     this is used from CManager::BeginOfGenstep to determine whether to call CManager::EndOfGenstep
    //     avoiding calling that until _genstep_index > -1
    // 
    int      _genstep_index ;  

    // CG4Ctx::setGen
    unsigned  _gen  ;
    unsigned  _genflag  ;

    // CG4Ctx::setTrack
    const G4Track*  _track ; 
    G4TrackStatus   _track_status ;  
    G4ProcessManager* _process_manager ; 

    int  _track_id ;
    int  _track_total ; 
    int  _track_step_count ; 
    int  _parent_id ;
    bool _optical ; 
    int  _pdg_encoding ;

    // CG4Ctx::setTrackOptical
    CTrk* _trk ;  
    int  _primary_id ; // used for reem continuation 
    int  _photon_id ;
    int  _photon_count ; 
    bool _reemtrack ; 
    int  _record_id ;
    int  _tk_record_id ;   // from CTrackInfo 
    double _record_fraction ; // used with --reflectcheat
    int  _mask_index ;        // original _record_id when using mask  

    // zeroed in CG4Ctx::setTrackOptical incremented in CG4Ctx::setStep
    int  _rejoin_count ; 
    int  _primarystep_count ; 
    CStage::CStage_t  _stage ; 

    // CTrackingAction::setTrack
    bool _debug ; 
    bool _other ; 
    bool _dump ; 
    int  _dump_count ; 

    // m_ctx.setStep invoked from CSteppingAction::setStep
    G4Step* _step ; 
    int _noZeroSteps ; 
    int _step_id ; 
    int _step_id_valid ;   // not incremented for zero-steps
    int _step_total ;
    G4ThreeVector _step_origin ; 
    G4StepStatus _step_pre_status ; 
    G4StepStatus _step_post_status ; 
 


#ifdef USE_CUSTOM_BOUNDARY
    Ds::DsG4OpBoundaryProcessStatus _boundary_status ;
    Ds::DsG4OpBoundaryProcessStatus _prior_boundary_status ;
#else
    G4OpBoundaryProcessStatus   _boundary_status ;
    G4OpBoundaryProcessStatus   _prior_boundary_status ;
#endif
  



    CG4Ctx(Opticks* ok);

    void init();
    void initEvent(const OpticksEvent* evt);

    Opticks* getOpticks() const ; 

    void setEvent(const G4Event* event);
    void BeginOfGenstep(char gentype, int num_photons, int offset);
    void EndOfGenstep();
    void setGentype(char gentype);
    void setGen(unsigned gen);

    void setTrack(const G4Track* track);
    void setTrackOptical(G4Track* mtrack);
    unsigned getNumTrackOptical() const ; 

    void setStep(const G4Step* step, int noZeroSteps);
    void setStepOptical();
    unsigned  step_limit() const ; 
    unsigned  point_limit() const ; 
    bool      is_dbg() const ; 

    std::string desc_event() const ;
    std::string desc_step() const ;
    std::string desc() const  ; 
    std::string brief() const  ; 
    std::string desc_stats() const ;

}; 


