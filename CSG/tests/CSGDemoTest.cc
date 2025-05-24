/**
CSGDemoTest.cc
==================

This executable creates and persists simple demo geometries, using the tests/DemoGeo.h struct. 
Invoke the executable with::

    cd ~/opticks/CSG
    ./CSGDemoTest.sh    

To apply the above script to all the demo geometries use ``./make_demos.sh`` 
Render the geometries with::

    cd ~/opticks/CSGOptiX
    ./cxr_demo.sh 

**/

#include <csignal>
#include "SSys.hh"
#include "SPath.hh"
#include "OPTICKS_LOG.hh"

#include "scuda.h"
#include "CSGFoundry.h"
#include "DemoGeo.h"


struct CSGDemoTest
{
    static const char* BASE ; 
    const char* geom ; 
    CSGFoundry fd ;  

    CSGDemoTest(const char* geom); 
    void init(); 
    void save() const ; 
    void dump() const ; 
};


// IT IS EXPEDIENT FOR ALL TEST GEOMETRIES TO LIVE IN THE SAME TREE 
const char* CSGDemoTest::BASE = "$TMP/GeoChain" ;


CSGDemoTest::CSGDemoTest(const char* geom_)
    :
    geom(strdup(geom_))
{
    init();
}

void CSGDemoTest::init()
{
    DemoGeo dg(&fd, geom) ;  
    LOG(info) << fd.desc(); 
    LOG(info) << fd.descSolids(); 
}

void CSGDemoTest::save() const 
{
    const char* fold = SPath::Resolve(BASE, geom, DIRPATH );
    const char* cfbase = SSys::getenvvar("CFBASE", fold  );
    const char* rel = "CSGFoundry" ;
    fd.save(cfbase, rel );  

    CSGFoundry* lfd = CSGFoundry::Load(cfbase, rel);  // load foundary and check identical bytes
    bool lfd_expect = CSGFoundry::Compare(&fd, lfd ) == 0 ; 
    assert(lfd_expect);
    if(!lfd_expect) std::raise(SIGINT); 

}


void CSGDemoTest::dump() const
{
    unsigned ias_idx = 0u ; 
    unsigned long long emm = 0ull ;
    LOG(info) << "descInst" << std::endl << fd.descInst(ias_idx, emm);  

    AABB bb = fd.iasBB(ias_idx, emm ); 
    float4 ce = bb.center_extent() ;  

    LOG(info) << "bb:" << bb.desc() ; 
    LOG(info) << "ce:" << ce ; 

    std::vector<float3> corners ; 
    AABB::cube_corners(corners, ce ); 
    for(int i=0 ; i < int(corners.size()) ; i++) LOG(info) << corners[i] ;  
}


int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    const char* geom = SSys::getenvvar("GEOM", "sphere" ); 
    CSGDemoTest dt(geom); 
    dt.save(); 
    dt.dump(); 

    return 0 ; 
}
