// TEST=GMakerTest om-t
// op --gmaker
//
#include "OPTICKS_LOG.hh"
#include "OpticksCSG.h"

#include "NGLM.hpp"
#include "NCSG.hpp"
#include "NPYBase.hpp"
#include "NPYMeta.hpp"
#include "NNode.hpp"
#include "NNodeSample.hpp"
#include "NSceneConfig.hpp"

#include "Opticks.hh"

#include "GMesh.hh"
#include "GVolume.hh"
#include "GBndLib.hh"
#include "GMeshLib.hh"
#include "GMaker.hh"

#include "OPTICKS_LOG.hh"

#include "GGEO_BODY.hh"

class GMakerTest 
{
   public:
       GMakerTest(Opticks* ok, GBndLib* blib, GMeshLib* meshlib);
       void makeSphere();
       void makeFromCSG();
   private:
       Opticks* m_ok  ;
       GMaker*  m_maker ;  
};

GMakerTest::GMakerTest(Opticks* ok, GBndLib* blib, GMeshLib* meshlib)
   :
   m_ok(ok),
   m_maker(new GMaker(ok, blib, meshlib))
{
}

void GMakerTest::makeSphere()
{
    glm::vec4 param(0.f,0.f,0.f,100.f) ; 

    const char* spec = "Rock//perfectAbsorbSurface/Vacuum" ; 

    GVolume* volume = m_maker->make(0u, CSG_SPHERE, param, spec );

    volume->Summary();

    const GMesh* mesh = volume->getMesh();

    mesh->dump();
}




void GMakerTest::makeFromCSG()
{
    typedef std::vector<nnode*> VN ;
    VN nodes ; 
    NNodeSample::Tests(nodes);

    const char* spec = "Rock//perfectAbsorbSurface/Vacuum" ; 

    unsigned verbosity = 1 ; 

    const NSceneConfig* config = new NSceneConfig(m_ok->getGLTFConfig()); 

    for(VN::const_iterator it=nodes.begin() ; it != nodes.end() ; it++)
    {
        nnode* n = *it ; 
        n->dump();

        n->set_boundary(spec);

        unsigned soIdx = 0 ; 
        unsigned lvIdx = 0 ; 

        NCSG* csg = NCSG::Adopt( n, config, soIdx, lvIdx );
        csg->getMeta()->setValue<std::string>("poly", "IM");

        csg->setVerbosity(verbosity);

        const GMesh* mesh = m_maker->makeMeshFromCSG(csg) ;   
        
        GVolume* volume = m_maker->makeFromMesh(mesh);

        mesh->Summary();

        volume->Summary();
   }
}


int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv);

    Opticks ok(argc, argv);
    ok.configure();

    bool constituents = true ; 
    GBndLib* blib = GBndLib::load(&ok, constituents ); 
    blib->closeConstituents();

    GMeshLib* meshlib = GMeshLib::Load(&ok); 

    LOG(error) << " after load " ; 

    GMakerTest tst(&ok, blib, meshlib);

    LOG(error) << " after ctor " ; 

    tst.makeSphere();
    LOG(error) << " after makeSphere  " ; 
    tst.makeFromCSG();
    LOG(error) << " after makeFromCSG   " ; 

}

