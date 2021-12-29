/**
CSGOptiXSimulateTest
======================

Using as much as possible the CSGOptiX rendering machinery 
to do simulation. Using CSGOptiX raygen mode 1 which flips the case statement 
in the __raygen__rg program. 

The idea is to minimize code divergence between ray trace rendering and 
ray trace enabled simulation. Because after all the point of the rendering 
is to be able to see the exact same geometry that the simulation is using.

::

     MOI=Hama CXS_CEGS=5:0:5:1000   CSGOptiXSimulateTest
     MOI=Hama CXS_CEGS=10:0:10:1000 CSGOptiXSimulateTest

**/

#include <cuda_runtime.h>
#include <algorithm>
#include <csignal>
#include <iterator>

#include "scuda.h"
#include "sqat4.h"
#include "stran.h"

#include "SSys.hh"
#include "SPath.hh"

#include "OPTICKS_LOG.hh"
#include "Opticks.hh"

#include "CSGFoundry.h"
#include "CSGOptiX.h"

#include "QSim.hh"
#include "QEvent.hh"
#include "SEvent.hh"



int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    Opticks ok(argc, argv ); 
    ok.configure(); 
    ok.setRaygenMode(1) ; // override --raygenmode option 

    int optix_version_override = CSGOptiX::_OPTIX_VERSION(); 
    const char* out_prefix = ok.getOutPrefix(optix_version_override);   
    // out_prefix includes values of envvars OPTICKS_GEOM and OPTICKS_RELDIR when defined
    LOG(info) 
        << " optix_version_override " << optix_version_override
        << " out_prefix [" << out_prefix << "]" 
        ;

    const char* cfbase = ok.getFoundryBase("CFBASE");  // envvar CFBASE can override 

    int create_dirs = 2 ;  
    const char* default_outdir = SPath::Resolve(cfbase, "CSGOptiXSimulateTest", out_prefix, create_dirs );  
    const char* outdir = SSys::getenvvar("OPTICKS_OUTDIR", default_outdir );  

    ok.setOutDir(outdir); 
    ok.writeOutputDirScript(outdir) ; // writes CSGOptiXSimulateTest_OUTPUT_DIR.sh in PWD 

    const char* outdir2 = ok.getOutDir(); 
    assert( strcmp(outdir2, outdir) == 0 ); 

    LOG(info) 
        << " cfbase " << cfbase 
        << " default_outdir " << default_outdir
        << " outdir " << outdir
        ; 

    ok.dumpArgv("CSGOptiXSimulateTest"); 

    const char* top    = SSys::getenvvar("TOP", "i0" ); 
    const char* botline = SSys::getenvvar("BOTLINE", nullptr ) ; 

    const char* moi = SSys::getenvvar("MOI", "sWorld:0:0");  
    float gridscale = SSys::getenvfloat("GRIDSCALE", 1.0 ); 
    const char* topline = SSys::getenvvar("TOPLINE", "CSGOptiXRender") ; 

    const char* idpath = ok.getIdPath(); 
    const char* rindexpath = SPath::Resolve(idpath, "GScintillatorLib/LS_ori/RINDEX.npy", 0 );  
    
    CSGFoundry* fd = CSGFoundry::Load(cfbase, "CSGFoundry"); 
    fd->upload(); 

    // GPU physics uploads : boundary+scintillation textures, property+randomState arrays    
    QSim<float>::UploadComponents(fd->icdf, fd->bnd, rindexpath ); 

    LOG(info) << "foundry " << fd->desc() ; 
    //fd->summary(); 

    CSGOptiX cx(&ok, fd); 
    cx.setTop(top); 

    if( cx.raygenmode == 0 )
    {
        LOG(fatal) << " WRONG EXECUTABLE FOR CSGOptiX::render cx.raygenmode " << cx.raygenmode ; 
        assert(0); 
    }

    const NP* gs = nullptr ; 
    if( strcmp(moi, "FAKE") == 0 )
    { 
        std::vector<int> photon_counts_per_genstep = { 3, 5, 2, 0, 1, 3, 4, 2, 4 };
        gs = SEvent::MakeCountGensteps(photon_counts_per_genstep) ;
    }
    else
    {
        float4 ce = make_float4(0.f, 0.f, 0.f, 1000.f ); 
        int midx, mord, iidx ; 
        fd->parseMOI(midx, mord, iidx, moi );  
        LOG(info) << " moi " << moi << " midx " << midx << " mord " << mord << " iidx " << iidx ;   

        if( midx == -1 )
        {    
            LOG(fatal)
                << " failed CSGFoundry::parseMOI for moi [" << moi << "]" 
                ;
            return 1 ;             
        }


        qat4 qt ; qt.init();  // initialize ti identity 
        int rc = fd->getCenterExtent(ce, midx, mord, iidx, &qt ) ;
        LOG(info) << " rc " << rc << " MOI.ce (" 
              << ce.x << " " << ce.y << " " << ce.z << " " << ce.w << ")" ;           

        LOG(info) << std::endl << "qt" << qt ; 
        Tran<double>* geotran = Tran<double>::ConvertToTran( &qt );  // houses transform and inverse


        std::vector<int> cegs ; 
        SSys::getenvintvec("CXS_CEGS", cegs, ':', "5:0:5:1000" ); 
        // expect 4 or 7 ints delimited by colon nx:ny:nz:num_pho OR nx:px:ny:py:nz:py:num_pho 

        SEvent::StandardizeCEGS(ce, cegs, gridscale ); 
        assert( cegs.size() == 7 ); 

        float3 mn ; 
        float3 mx ; 

        bool ce_offset_bb = true ; 
        SEvent::GetBoundingBox( mn, mx, ce, cegs, gridscale, ce_offset_bb ); 

        LOG(info) 
            << " ce_offset_bb " << ce_offset_bb
            << " mn " << mn 
            << " mx " << mx
            ; 

        std::vector<int> override_ce ; 
        SSys::getenvintvec("CXS_OVERRIDE_CE",  override_ce, ':', "0:0:0:0" ); 

        if( override_ce.size() == 4 && override_ce[3] > 0 )
        {
            ce.x = float(override_ce[0]); 
            ce.y = float(override_ce[1]); 
            ce.z = float(override_ce[2]); 
            ce.w = float(override_ce[3]); 
            LOG(info) << "override the MOI.ce with CXS_OVERRIDE_CE (" << ce.x << " " << ce.y << " " << ce.z << " " << ce.w << ")" ;  
        } 

        bool ce_offset = false ; 
        gs = SEvent::MakeCenterExtentGensteps(ce, cegs, gridscale, geotran, ce_offset ); 

        cx.setCE(ce); 
        cx.setCEGS(cegs); 
        cx.setMetaTran(geotran); 

        //cx.setNear(0.1); // TODO: not getting 0.1., investigate 
    }

    cx.setGensteps(gs); 

    double dt = cx.simulate();  
    LOG(info) << " dt " << dt ;

    cx.snapSimulateTest(outdir, botline, topline ); 
 

    cudaDeviceSynchronize(); 
    return 0 ; 
}
