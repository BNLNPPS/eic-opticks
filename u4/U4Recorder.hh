#pragma once
/**
U4Recorder
===========

U4Recorder is NOT a G4UserRunAction, G4UserEventAction, ... despite 
having the corresponding method names. 

The U4Recorder relies on the RunAction, EventAction  etc.. classes 
calling those lifecycle methods.   

**/

#include <vector>
#include <string>

class G4Run ; 
class G4Event ; 
class G4Track ; 
class G4Step ; 
class G4VSolid ; 
class G4StepPoint ; 

struct spho ; 

#include "plog/Severity.h"
#include "G4TrackStatus.hh"
#include "U4_API_EXPORT.hh"


struct U4_API U4Recorder 
{
    static const plog::Severity LEVEL ; 
    static const int PIDX ; 
    static const int GIDX ; 
    static std::string Desc(); 
    static bool Enabled(const spho& label); 

    static U4Recorder* INSTANCE ; 
    static U4Recorder* Get(); 

    U4Recorder(); 

    void BeginOfRunAction(const G4Run*);
    void EndOfRunAction(const G4Run*);

    void BeginOfEventAction(const G4Event*);
    void EndOfEventAction(const G4Event*);

    void PreUserTrackingAction(const G4Track*);
    void PostUserTrackingAction(const G4Track*);
    void UserSteppingAction(const G4Step*);

    void PreUserTrackingAction_Optical(const G4Track*);
    void PostUserTrackingAction_Optical(const G4Track*);
    void UserSteppingAction_Optical(const G4Step*); 

    void Check_TrackStatus_Flag(G4TrackStatus tstat, unsigned flag); 

};


