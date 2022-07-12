/**
CSGOptiXSimtraceTest : used from cxs.sh
===========================================

Using as much as possible the CSGOptiX rendering machinery 
to do simulation. Using CSGOptiX raygen mode 1 which flips the case statement 
in the __raygen__rg program. 

The idea is to minimize code divergence between ray trace rendering and 
ray trace enabled simulation. Because after all the point of the rendering 
is to be able to see the exact same geometry that the simulation is using.

::

     MOI=Hama CEGS=5:0:5:1000   CSGOptiXSimtraceTest
     MOI=Hama CEGS=10:0:10:1000 CSGOptiXSimtraceTest

TODO: 
    find way to get sdf distances for all intersects GPU side 
    using GPU side buffer of positions 

    just need to run the distance function for all points and with 
    the appropriate CSGNode root and num_node

    does not need OptiX, just CUDA (or it can be done CPU side also)

**/

#include <cuda_runtime.h>

#include "scuda.h"
#include "sqat4.h"
#include "sframe.h"

#include "SSim.hh"
#include "SOpticks.hh"
#include "SEvt.hh"
#include "SFrameGenstep.hh"
#include "SEventConfig.hh"
#include "QEvent.hh"

#include "OPTICKS_LOG.hh"
#include "CSGFoundry.h"
#include "CSGOptiX.h"
#include "QSim.hh"

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 
    SEventConfig::SetRGModeSimtrace(); 
    SEventConfig::SetCompMask("genstep,simtrace"); 

    SOpticks::WriteOutputDirScript() ; // writes CSGOptiXSimtraceTest_OUTPUT_DIR.sh in PWD 
   
    SEvt evt ; 
 
    CSGFoundry* fd = CSGFoundry::Load(); 

    sframe fr = fd->getFrame() ;  // depends on MOI, fr.ce fr.m2w fr.w2m set by CSGTarget::getFrame 

    std::cout << "[ main fr" << std::endl << fr << std::endl << "] main fr" << std::endl  ; 

    evt.setFrame(fr);   // adds CE gensteps

 
    CSGOptiX* cx = CSGOptiX::Create(fd); 
    QSim* qs = cx->sim ; 

    cx->setFrame(fr);    

    // This seems funny as cx has fd which has the fr already : so could be automatic ?  
    // Not so easy as which frame to use depends on running mode and user input 
    // so best to not hide it. 


    qs->simtrace();  

    cudaDeviceSynchronize(); 

    evt.save(); // uses SGeo::LastUploadCFBase_OutDir to place outputs into CFBase/ExecutableName folder sibling to CSGFoundry   

 
    return 0 ; 
}
