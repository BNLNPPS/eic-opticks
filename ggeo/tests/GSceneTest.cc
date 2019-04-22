/**

GSceneTest --okcore debug --gltfname hello.gltf

**/
#include <set>
#include <string>

#include "NScene.hpp"

#include "Opticks.hh"
#include "GGeo.hh"
#include "GMergedMesh.hh"

#include "GScene.hh"

#include "OPTICKS_LOG.hh"


struct GSceneTest
{
    Opticks* ok ;
    GGeo* gg ; 

    GSceneTest(Opticks* ok_) 
        : 
        ok(ok_),
        gg(new GGeo(ok))
    {
        LOG(error) << "loadFromCache" ;  
        gg->loadFromCache();
        LOG(error) << "loadAnalyticFromCache" ;  
        gg->loadAnalyticFromCache();
        LOG(error) << "dumpStats" ;  
        gg->dumpStats();
    }
};



int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv);

    Opticks ok(argc, argv, "--gltf 3");
    ok.configure();

    const char* base = ok.getSrcGLTFBase() ;
    const char* name = ok.getSrcGLTFName() ;
    const char* config = ok.getGLTFConfig() ;
    int gltf = ok.getGLTF();

    assert(gltf == 3);

    LOG(info) << argv[0]
              << " base " << base
              << " name " << name
              << " config " << config
              << " gltf " << gltf 
              ; 

    GSceneTest gst(&ok);
    assert(gst.gg);


    return 0 ; 
}


