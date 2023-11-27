
#include "scuda.h"
#include "squad.h"
#include "sqat4.h"
#include "sframe.h"

#include "SSim.hh"
#include "SSys.hh"
#include "SPath.hh"
#include "SEvt.hh"
#include "SFrameGenstep.hh"
#include "NP.hh"
#include "OPTICKS_LOG.hh"
#include "CSGFoundry.h"

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    SEvt* evt = SEvt::Create(0) ;
    assert( evt ); 

    SSim::Create(); 

    CSGFoundry* fd = CSGFoundry::Load();

    sframe fr = fd->getFrame() ;  // depends on MOI, fr.ce fr.m2w fr.w2m are set by CSGTarget::getFrame 

    SEvt::AddGenstep( SFrameGenstep::MakeCenterExtentGenstep(fr) ); 

    NP* gs = SEvt::GatherGenstep(0); 
    NP* pp = SFrameGenstep::GenerateCenterExtentGenstepPhotons_( gs, fr.gridscale() );  

    std::cout << " fr " << std::endl << fr << std::endl ; 



    // HMM: want to use SEvt for saving not QEvent 
    const char* dir = spath::Resolve("$TMP/CSG/CSGFoundry_MakeCenterExtentGensteps_Test"); 
    std::cout << dir << std::endl ; 
    gs->save(dir, "genstep.npy"); 
    pp->save(dir, "photon.npy"); 
    fr.save(dir) ; 


    return 0 ; 
}

