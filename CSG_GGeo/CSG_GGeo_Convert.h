#pragma once

#include <vector>
#include "plog/Severity.h"

#include "CSG_GGEO_API_EXPORT.hh"

struct SSim ; 
struct stree ; 
struct float4 ; 
struct CSGFoundry ;
struct CSGSolid ; 
struct CSGPrim ; 
struct CSGNode ; 

class GGeo ;
class GParts ; 

struct CSG_GGEO_API CSG_GGeo_Convert
{   
    static CSGFoundry* Translate(const GGeo* ggeo ); 
    static const plog::Severity LEVEL ; 
    static unsigned CountSolidPrim( const GParts* comp ); 

    CSGFoundry* foundry ; 
    const GGeo* ggeo ; 
    SSim* sim ; 
    const stree* tree ; 
    bool reverse ; 
    int dump_ridx ; 
    const char* meta ; 

    CSG_GGeo_Convert(CSGFoundry* foundry, const GGeo* ggeo ) ; 
    void init();

    void convert();   

    void convertGeometry(int repeatIdx=-1,  int primIdx=-1, int partIdxRel=-1 );

    // collect inputs for creating GPU boundary and scintillation textures 
    void convertSim() ;

    // below all for geometry conversion 

    void convertAllSolid();
    CSGSolid* convertSolid(unsigned repeatIdx );
    void addInstances(unsigned repeatIdx );

    static int CheckConsistent(const GGeo* gg, const stree* st ); 
    static std::string DescConsistent(const GGeo* gg, const stree* st ); 


    CSGPrim*  convertPrim(const GParts* comp, unsigned primIdx );
    CSGNode*  convertNode(const GParts* comp, unsigned primIdx, unsigned partIdxRel );
    static std::vector<float4>* GetPlanes(const GParts* comp, unsigned primIdx, unsigned partIdxRel ); 
    static void DumpPlanes( const std::vector<float4>& planes , const char* msg="CSG_GGeo_Convert::DumpPlanes" ); 


    // below is called non-standardly during debugging when envvar ONE_PRIM_SOLID is defined 
    void addOnePrimSolid();
    void addOnePrimSolid(unsigned solidIdx);

    // below is called non-standardly during debugging when envvar ONE_NODE_SOLID is defined 
    void addOneNodeSolid();
    void addOneNodeSolid(unsigned solidIdx);
    void addOneNodeSolid(unsigned solidIdx, unsigned primIdx, unsigned primIdxRel);

    // below is called non-standardly during debugging when envvar DEEP_COPY_SOLID is defined 
    void addDeepCopySolid();

    // below is called non-standardly during debugging when envvar KLUDGE_SCALE_PRIM_BBOX is defined 
    void kludgeScalePrimBBox();

};


