#include "SSys.hh"
#include "SSim.hh"
#include "scuda.h"
#include "CSGFoundry.h"

#include "OPTICKS_LOG.hh"


void test_findSolidIdx(const CSGFoundry* fd, int argc, char** argv)
{
    LOG(info) << "[" ; 
    std::vector<unsigned> solid_selection ; 
    for(int i=1 ; i < argc ; i++)
    {
        const char* sla = argv[i] ;  
        solid_selection.clear(); 
        fd->findSolidIdx(solid_selection, sla );   

        LOG(info) 
            << " SLA " << sla 
            << " solid_selection.size " << solid_selection.size() 
            << std::endl 
            << fd->descSolids(solid_selection) 
            ; 


         for(int j=0 ; j < int(solid_selection.size()) ; j++)
         {
             unsigned solidIdx = solid_selection[j]; 
             LOG(info) << fd->descPrim(solidIdx) ;  
             LOG(info) << fd->descNode(solidIdx) ;  
             LOG(info) << fd->descTran(solidIdx) ;  
         }
    }
    LOG(info) << "]"  ;
    LOG_IF(error, argc == 1) << " enter args such as r0/r1/r2/r3/... to select and dump composite solids " ; 
 
}

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    SSim::Create(); 
    CSGFoundry* fd = CSGFoundry::Load(); 
 
    LOG(info) << "foundry " << fd->desc() ; 
    fd->summary(); 

    test_findSolidIdx(fd, argc, argv); 

    return 0 ; 
}




