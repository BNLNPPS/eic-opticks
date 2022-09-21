/**
X4_GetSolid.hh
================

Depending on the PMTSim CMake target is needed to access the JUNO Specific j/PMTSim solids 
and volumes. Only a few separately listed extg4 tests depend on PMTSim.
Access to these solids and volumes requires separate building of PMTSim 
and installation into the Opticks CMAKE_PREFIX_PATH 
This can be done with::

    cd ~/j/PMTSim   # jps  
    om 

**/

#include "SStr.hh"
#include "X4SolidMaker.hh"
#ifdef WITH_PMTSIM
#include "PMTSim.hh"
#endif

const G4VSolid* X4_GetSolid(const char* geom, std::string* meta=nullptr )
{
    const char* geom_upto_first_underscore = SStr::HeadFirst(geom, '_');  

    const G4VSolid* solid = nullptr ; 
    if(X4SolidMaker::CanMake(geom_upto_first_underscore ))
    {
        solid = X4SolidMaker::Make(geom_upto_first_underscore , meta); 
    }
    else
    {
#ifdef WITH_PMTSIM
        std::cout << "extg4/tests/X4_GetSolid.hh : X4_GetSolid : WITH_PMTSIM invoking PMTSim::GetSolid(\"" << geom << "\") " << std::endl ; 
        solid = PMTSim::GetSolid(geom);   // need geom for option passing following "__" 
#else
        std::cout << "extg4/tests/X4_GetSolid.hh : X4_GetSolid :  not WITH_PMTSIM " << geom << std::endl ;  
#endif
    }
    return solid ; 
}


