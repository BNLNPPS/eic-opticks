
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "OpticksUtil.hh"

#include "NP.hh"

#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4MaterialPropertyVector.hh"
#include "G4SystemOfUnits.hh"


void OpticksUtil::qvals( std::vector<float>& vals, const char* key, const char* fallback, int num_expect )
{
    char* val = getenv(key);
    char* p = const_cast<char*>( val ? val : fallback );  
    while (*p) 
    {   
        if( (*p >= '0' && *p <= '9') || *p == '+' || *p == '-' || *p == '.') vals.push_back(strtof(p, &p)) ; 
        else p++ ;
    }   
    if( num_expect > 0 ) assert( vals.size() == unsigned(num_expect) );  
}


NP* OpticksUtil::LoadArray(const char* kdpath) // static
{
    const char* keydir = getenv("OPTICKS_KEYDIR") ; 
    assert( keydir ); 
    std::stringstream ss ; 
    ss << keydir << "/" << kdpath ;  
    std::string s = ss.str(); 
    const char* path = s.c_str(); 
    std::cout << "OpticksUtil::LoadArray " << path << std::endl ; 
    NP* a = NP::Load(path); 
    return a ; 
}

G4MaterialPropertyVector* OpticksUtil::MakeProperty(const NP* a)  // static
{
    std::vector<double> d, v ; 
    a->psplit<double>(d,v);   // split array into domain and values 
    assert(d.size() == v.size() && d.size() > 1 ); 

    G4MaterialPropertyVector* mpv = new G4MaterialPropertyVector(d.data(), v.data(), d.size() ); 
    return mpv ; 
}

G4MaterialPropertiesTable*  OpticksUtil::MakeMaterialPropertiesTable( 
     const char* a_key, const G4MaterialPropertyVector* a_prop,
     const char* b_key, const G4MaterialPropertyVector* b_prop,
     const char* c_key, const G4MaterialPropertyVector* c_prop,
     const char* d_key, const G4MaterialPropertyVector* d_prop
)
{
    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();
    if(a_key && a_prop) mpt->AddProperty(a_key,const_cast<G4MaterialPropertyVector*>(a_prop));    //  HUH: why not const ?
    if(b_key && b_prop) mpt->AddProperty(b_key,const_cast<G4MaterialPropertyVector*>(b_prop));   
    if(c_key && c_prop) mpt->AddProperty(c_key,const_cast<G4MaterialPropertyVector*>(c_prop));   
    if(d_key && d_prop) mpt->AddProperty(d_key,const_cast<G4MaterialPropertyVector*>(d_prop));   
    return mpt ; 
}


G4Material* OpticksUtil::MakeMaterial(const G4MaterialPropertyVector* rindex, const char* name)  // static
{
    // its Water, but that makes no difference for Cerenkov 
    // the only thing that matters us the rindex property
    G4double a, z, density;
    G4int nelements;
    G4Element* O = new G4Element("Oxygen"  , "O", z=8 , a=16.00*CLHEP::g/CLHEP::mole);
    G4Element* H = new G4Element("Hydrogen", "H", z=1 , a=1.01*CLHEP::g/CLHEP::mole);
    G4Material* mat = new G4Material(name, density= 1.0*CLHEP::g/CLHEP::cm3, nelements=2);
    mat->AddElement(H, 2);
    mat->AddElement(O, 1);

    G4MaterialPropertiesTable* mpt = MakeMaterialPropertiesTable("RINDEX", rindex) ; 
    mat->SetMaterialPropertiesTable(mpt) ;
    return mat ; 
}

int OpticksUtil::getenvint(const char* envkey, int fallback)
{
    char* val = getenv(envkey);
    int ival = val ? std::atoi(val) : fallback ;
    return ival ; 
}



bool OpticksUtil::ExistsPath(const char* base_, const char* reldir_, const char* name_ )
{
    fs::path fpath(base_);
    if(reldir_) fpath /= reldir_ ;
    if(name_) fpath /= name_ ;
    bool x = fs::exists(fpath); 
    std::cout << "OpticksUtil::ExistsPath " << ( x ? "Y" : "N" ) << " " << fpath.string().c_str() << std::endl ; 
    return x ; 
}
std::string OpticksUtil::prepare_path(const char* dir_, const char* reldir_, const char* name )
{   
    fs::path fdir(dir_);
    if(reldir_) fdir /= reldir_ ;

    if(!fs::exists(fdir))
    {   
        if (fs::create_directories(fdir))
        {   
            std::cout << "created directory " << fdir.string().c_str() << std::endl  ;
        }   
    }   

    fs::path fpath(fdir); 
    fpath /= name ;   

    return fpath.string();
}

/**
OpticksUtil::ListDir
-----------------------

From BDir::dirlist, collect names of files within directory *path* with names ending with *ext*.
The names are sorted using default std::sort lexical ordering. 

**/

void OpticksUtil::ListDir(std::vector<std::string>& names,  const char* path, const char* ext) // static
{
    fs::path dir(path);
    bool dir_exists = fs::exists(dir) && fs::is_directory(dir) ; 
    if(!dir_exists) return ; 

    fs::directory_iterator it(dir) ;
    fs::directory_iterator end ;
   
    for(; it != end ; ++it)
    {   
        std::string fname = it->path().filename().string() ;
        const char* fnam = fname.c_str();

        if(strlen(fnam) > strlen(ext) && strcmp(fnam + strlen(fnam) - strlen(ext), ext)==0)
        {   
            names.push_back(fnam);
        }   
    }   

    std::sort( names.begin(), names.end() ); 
}


/**
Functionality of former OpticksUtil::LoadConcat now provided by NP::Load
----------------------------------------------------------------------------

If *concat_path* ends with ".npy" simply loads it into seq array
otherwise *concat_path* is assumed to be a directory containing multiple ".npy"
to be concatenated.  The names of the paths in the directory are obtained using 
OpticksUtil::ListDir 

**/


