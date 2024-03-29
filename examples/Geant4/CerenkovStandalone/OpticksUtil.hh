#pragma once
/**
OpticksUtil.hh
=================

DONT USE THIS IN NEW DEVELOPMENT USE u4/U4 instead 

TODO: switch over all use of OpticksUtil to U4 

**/

#include <vector>
#include <string>

struct NP ; 
#include "G4MaterialPropertyVector.hh"
class G4Material ; 
class G4MaterialPropertiesTable ; 

struct OpticksUtil
{
    static void qvals( std::vector<float>& vals, const char* key, const char* fallback, int num_expect ); 

   // static NP* LoadArray(const char* kdpath);  // just use NP::Load

    static G4MaterialPropertyVector* MakeProperty(const NP* a);
    static G4MaterialPropertiesTable*  MakeMaterialPropertiesTable( 
         const char* a_key=nullptr, const G4MaterialPropertyVector* a_prop=nullptr,
         const char* b_key=nullptr, const G4MaterialPropertyVector* b_prop=nullptr,
         const char* c_key=nullptr, const G4MaterialPropertyVector* c_prop=nullptr,
         const char* d_key=nullptr, const G4MaterialPropertyVector* d_prop=nullptr
     ); 
    static G4Material* MakeMaterial(const G4MaterialPropertyVector* rindex, const char* name="Water") ; 

    static int getenvint(const char* envkey, int fallback);

    // below functionality now in NP.hh 
    //static bool ExistsPath(const char* base_, const char* reldir_=nullptr, const char* name_=nullptr );
    //static std::string prepare_path(const char* dir_, const char* reldir_, const char* name );
    //static void ListDir(std::vector<std::string>& names,  const char* path, const char* ext); 

}; 
