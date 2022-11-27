#include "SSys.hh"
#include "SSim.hh"
#include "SOpticksResource.hh"
#include "CSGFoundry.h"
#include "CSGMaker.h"
#include "OPTICKS_LOG.hh"

void GetNames( std::vector<std::string>& names, bool listnames )
{
     const char* geom = SSys::getenvvar("CSGMakerTest_GEOM", nullptr ); 
     // NB ExecutableName_GEOM is treated as GEOM override 

     if( geom == nullptr ) 
     {
         CSGMaker::GetNames(names); 
     }
     else
     { 
         names.push_back(geom); 
     }
     LOG(info) << " names.size " << names.size() ; 
     if(listnames) for(unsigned i=0 ; i < names.size() ; i++) std::cout << names[i] << std::endl ; 
}

int main(int argc, char** argv)
{
     const char* arg =  argc > 1 ? argv[1] : nullptr ; 
     bool listnames = arg && ( strcmp(arg,"N") == 0 || strcmp(arg,"n") == 0 ) ; 
     OPTICKS_LOG(argc, argv); 

     SSim* sim = SSim::Create(); 
     assert(sim); 

     std::vector<std::string> names ; 
     GetNames(names, listnames); 
     if(listnames) return 0 ; 

     for(unsigned i=0 ; i < names.size() ; i++)
     {
         const char* name = names[i].c_str() ; 
         LOG(info) << name ; 

         SOpticksResource::SetGEOM(name);  

         CSGFoundry* fd = CSGFoundry::MakeGeom( name ); 
         LOG(info) << fd->desc();    

         fd->save();  
         // HMM: this is stomping on standard GEOM folder  
         // USING MakeGeom/LoadGeom needs to effectively change GEOM to the name argument
         // and standard corresponding folder 

         CSGFoundry* lfd = CSGFoundry::LoadGeom( name ); 

         LOG(info) << " lfd.loaddir " << lfd->loaddir ; 

         int rc = CSGFoundry::Compare(fd, lfd );  
         assert( 0 == rc );
     }

     return 0 ; 
}
