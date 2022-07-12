/**
G4CXRenderTest.cc
===================

GGeo creation done when starting from a gdml or live G4,  still needs Opticks instance,  
TODO: avoid this by replacing with automated SOpticks instanciated by OPTICKS_LOG


HMM: no SEvt, saving the frame and render files could be managed by SEvt too to make the environment 
the same in all RGNode

**/
#include "SEventConfig.hh"
#include "OPTICKS_LOG.hh"
#include "Opticks.hh"
#include "G4CXOpticks.hh"

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    SEventConfig::SetRGModeRender();  

    Opticks::Configure(argc, argv, "--gparts_transform_offset" );  

    G4CXOpticks gx ;  
    gx.setGeometry();  // sensitive to SomGDMLPath, GEOM, CFBASE

    gx.render();       // sensitive to MOI, EYE, LOOK, UP
 
    return 0 ; 
}
