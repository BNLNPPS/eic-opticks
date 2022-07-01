#include <sstream>

#include "G4Material.hh"
#include "G4ThreeVector.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Box.hh"
#include "G4Orb.hh"

#include "SStr.hh"
#include "SSys.hh"
#include "PLOG.hh"

#include "U4.hh"
#include "U4Material.hh"
#include "U4Surface.h"
#include "U4SolidMaker.hh"
#include "U4VolumeMaker.hh"

#ifdef WITH_PMTSIM
#include "PMTSim.hh"
#endif

const plog::Severity U4VolumeMaker::LEVEL = PLOG::EnvLevel("U4VolumeMaker", "DEBUG"); 

std::string U4VolumeMaker::Desc() // static
{
    std::stringstream ss ; 
    ss << "U4VolumeMaker::Desc" ; 
#ifdef WITH_PMTSIM
    ss << " WITH_PMTSIM " ; 
#else
    ss << " not-WITH_PMTSIM " ; 
#endif
    std::string s = ss.str(); 
    return s ; 
}



G4VPhysicalVolume* U4VolumeMaker::Make(){ return Make(SSys::getenvvar("GEOM", "BoxOfScintillator")); }

/**
U4VolumeMaker::Make
---------------------

Hmm the input names could be of LV or PV with no consistent
way to distinguish. 


PMTSim::HasManagerPrefix 
    returns true for names starting with one of: hama, nnvt, hmsk, nmsk, lchi

PMTSim::GetLV PMTSim::GetPV
     these methods act as go betweens to the underlying managers with prefixes
     that identify the managers offset  

**/

G4VPhysicalVolume* U4VolumeMaker::Make(const char* name)
{
    G4VPhysicalVolume* pv = nullptr ; 
    G4LogicalVolume* lv = nullptr ; 
#ifdef WITH_PMTSIM
    bool has_manager_prefix = PMTSim::HasManagerPrefix(name) ;
    LOG(info) << "WITH_PMTSIM name [" << name << "] has_manager_prefix " << has_manager_prefix ; 
    if(has_manager_prefix) 
    {
        lv = PMTSim::GetLV(name) ; 
        if( lv == nullptr ) LOG(fatal) << "PMTSim::GetLV returned nullptr for name [" << name << "]" ; 
        assert( lv ); 
        pv = Wrap( lv, 5000. ) ;          
    }
#else
    LOG(info) << " not-WITH_PMTSIM name [" << name << "]" ; 
#endif
    if(pv == nullptr) pv = Make_(name); 
    if(pv == nullptr) pv = MakePhysical(name) ; 
    LOG(info) << " name " << name << " pv " << pv ; 
    return pv ; 
}
G4VPhysicalVolume* U4VolumeMaker::Make_(const char* name)
{
    G4VPhysicalVolume* pv = nullptr ; 
    if(strcmp(name,"BoxOfScintillator" ) == 0)     pv = BoxOfScintillator(1000.);   
    if(strcmp(name,"RaindropRockAirWater" ) == 0)  pv = RaindropRockAirWater();   
    return pv ; 
}


/**
U4VolumeMaker::Wrap
---------------------

The LV provided is placed within a WorldBox of halfside extent and the world PV is returned. 

**/

G4VPhysicalVolume* U4VolumeMaker::Wrap( G4LogicalVolume* lv, double halfside  )
{
    G4VPhysicalVolume* world_pv = WorldBox(halfside); 
    G4LogicalVolume* world_lv = world_pv->GetLogicalVolume(); 

    G4String name = lv->GetName(); 
    name += "_pv" ; 

    G4ThreeVector tla(0.,0.,0.); 
    G4VPhysicalVolume* pv_item = new G4PVPlacement(0, tla, lv ,name,world_lv,false,0);
    assert( pv_item );

    return world_pv ; 
}










G4LogicalVolume*  U4VolumeMaker::MakeLogical(const char* name)
{
    const G4VSolid* solid_  = U4SolidMaker::Make(name); 
    G4VSolid* solid = const_cast<G4VSolid*>(solid_); 

    G4Material* material = U4Material::Get("Vacuum"); 
    G4LogicalVolume* lv = new G4LogicalVolume( solid, material, name, 0,0,0,true ); 
    return lv ; 
}

void U4VolumeMaker::MakeLogical(std::vector<G4LogicalVolume*>& lvs , const char* names_ )
{
    std::vector<std::string> names ; 
    SStr::Split(names_ , ',', names ); 
    unsigned num_names = names.size(); 
    LOG(LEVEL) << " names_ " << names_ << " num_names " << num_names ;  
    assert( num_names > 1 ); 

    for(unsigned i=0 ; i < num_names ; i++)
    {
        const char* name = names[i].c_str(); 
        G4LogicalVolume* lv = MakeLogical(name) ; 
        assert(lv); 
        lvs.push_back(lv);   
    }
}

G4VPhysicalVolume* U4VolumeMaker::MakePhysical(const char* name)
{
    bool list = strstr(name, "List") != nullptr ; 
    G4VPhysicalVolume* pv = list ? MakePhysicalList_(name) : MakePhysicalOne_(name)  ; 
    return pv ; 
}
G4VPhysicalVolume* U4VolumeMaker::MakePhysicalList_(const char* name)
{
    assert( SStr::StartsWith(name, "List") ); 
    std::vector<G4LogicalVolume*> lvs ; 
    MakeLogical(lvs, name + strlen("List") ); 
    G4VPhysicalVolume* pv = WrapLVGrid(lvs, 1, 1, 1 ); 
    return pv ; 
}

G4VPhysicalVolume* U4VolumeMaker::MakePhysicalOne_(const char* name)
{
    G4VPhysicalVolume* pv = nullptr ; 

    bool grid = strstr(name, "Grid") != nullptr ; 
    bool cube = strstr(name, "Cube") != nullptr ; 
    bool xoff = strstr(name, "Xoff") != nullptr ; 
    bool yoff = strstr(name, "Yoff") != nullptr ; 
    bool zoff = strstr(name, "Zoff") != nullptr ; 

    G4LogicalVolume* lv = MakeLogical(name) ; 

    if(grid)      pv =   WrapLVGrid(lv, 1, 1, 1 ); 
    else if(cube) pv =   WrapLVTranslate(lv, 100., 100., 100. ); 
    else if(xoff) pv =   WrapLVOffset(lv, 200.,   0.,   0. ); 
    else if(yoff) pv =   WrapLVOffset(lv,   0., 200.,   0. ); 
    else if(zoff) pv =   WrapLVOffset(lv,   0.,   0., 200. ); 
    else          pv =   WrapLVTranslate(lv, 0., 0., 0. ); 

    return pv ; 
}



void U4VolumeMaker::AddPlacement( G4LogicalVolume* mother,  G4LogicalVolume* lv,  double tx, double ty, double tz )
{
    G4ThreeVector tla(tx,ty,tz); 
    std::string name = lv->GetName(); 
    name += "_placement" ; 
    LOG(LEVEL) << Desc(tla) << " " << name ;  
    G4VPhysicalVolume* pv = new G4PVPlacement(0,tla,lv,name,mother,false,0);
    assert(pv); 
} 


void U4VolumeMaker::AddPlacement( G4LogicalVolume* mother, const char* name,  double tx, double ty, double tz )
{
    G4LogicalVolume* lv = MakeLogical(name); 
    AddPlacement( mother, lv, tx, ty, tz ); 
}

G4VPhysicalVolume* U4VolumeMaker::WrapLVOffset( G4LogicalVolume* lv, double tx, double ty, double tz )
{
    G4String name = lv->GetName(); 
    name += "_Offset" ; 

    double halfside = 3.*std::max( std::max( tx, ty ), tz ); 
    G4VPhysicalVolume* world_pv = WorldBox(halfside); 
    G4LogicalVolume* world_lv = world_pv->GetLogicalVolume(); 

    AddPlacement(world_lv, lv, tx, ty, tz ); 

    bool bmo = true ; 
    if(bmo) AddPlacement( world_lv, "BoxMinusOrb", 0., 0., 0. ); 

    return world_pv ;  
}

/**
U4VolumeMaker::WrapLVTranslate
-------------------------------

Place the lv at 8 positions at the corners of a cube. 

  ZYX
0 000
1 001
2 010          
3 011         
4 100        
5 101           
6 110
7 111

              110             111
                +-----------+
               /|          /| 
              / |         / | 
             /  |        /  |
            +-----------+   |
            |   +-------|---+ 011
            |  /        |  / 
            | /         | /
            |/          |/
            +-----------+
          000          001
            
   Z   Y
   | /   
   |/
   0-- X

**/

G4VPhysicalVolume* U4VolumeMaker::WrapLVTranslate( G4LogicalVolume* lv, double tx, double ty, double tz )
{
    double halfside = 3.*std::max( std::max( tx, ty ), tz ); 

    G4VPhysicalVolume* world_pv = WorldBox(halfside); 
    G4LogicalVolume* world_lv = world_pv->GetLogicalVolume(); 
    G4String name = lv->GetName(); 
    
    for(unsigned i=0 ; i < 8 ; i++)
    {
        bool px = ((i & 0x1) != 0) ; 
        bool py = ((i & 0x2) != 0) ; 
        bool pz = ((i & 0x4) != 0) ; 
        G4ThreeVector tla( px ? tx : -tx ,  py ? ty : -ty,  pz ? tz : -tz ); 
        const char* iname = GridName(name.c_str(), int(px), int(py), int(pz), "" );    
        G4VPhysicalVolume* pv = new G4PVPlacement(0,tla,lv,iname,world_lv,false,0);
        assert( pv );  
    }
    return world_pv ;  
}

/**
U4VolumeMaker::WorldBox
--------------------------


**/

G4VPhysicalVolume* U4VolumeMaker::WorldBox( double halfside, const char* mat )
{
    G4Material* material= U4Material::Get(mat); 
    return WorldBox(halfside, material); 
}

G4VPhysicalVolume* U4VolumeMaker::WorldBox( double halfside, G4Material* material )
{
    G4Box* solid = new G4Box("World_solid", halfside, halfside, halfside );  
    G4LogicalVolume* lv = new G4LogicalVolume(solid,material,"World_lv",0,0,0); 
    G4LogicalVolume* mother_lv = nullptr ; 
    G4VPhysicalVolume* pv = new G4PVPlacement(0,G4ThreeVector(), lv ,"World_pv",mother_lv,false,0);
    return pv ; 
}

G4VPhysicalVolume* U4VolumeMaker::BoxOfScintillator( double halfside )
{
    G4Material* mat = U4Material::MakeScintillator() ; 
    return WorldBox(halfside, mat);
}
/**
U4VolumeMaker::RaindropRockAirWater
-------------------------------------

cf CSG/CSGMaker.cc CSGMaker::makeBoxedSphere


   +------------------------+ 
   | Rock                   |
   |    +-----------+       |
   |    | Air       |       |    
   |    |    . .    |       |    
   |    |   . Wa.   |       |    
   |    |    . .    |       |    
   |    |           |       |    
   |    +-----|-----+       |
   |                        |
   +----------|--rock_halfs-+ 
                   

Defaults::

    HALFSIDE: 100. 
    FACTOR: 1. 
   
    water_radius   :  halfside/2.      : 50. 
    air_halfside   :  halfside*factor  : 100.
    rock_halfside  :  2.*halfside*factor : 200. 

**/




G4VPhysicalVolume* U4VolumeMaker::RaindropRockAirWater()
{
    double halfside = SSys::getenvdouble(U4VolumeMaker_RaindropRockAirWater_HALFSIDE, 100.); 
    double factor   = SSys::getenvdouble(U4VolumeMaker_RaindropRockAirWater_FACTOR,   1.); 

    LOG(info) << U4VolumeMaker_RaindropRockAirWater_HALFSIDE << " " << halfside ; 
    LOG(info) << U4VolumeMaker_RaindropRockAirWater_FACTOR   << " " << factor ; 

    return RaindropRockAirWater(halfside, factor); 
}

G4VPhysicalVolume* U4VolumeMaker::RaindropRockAirWater( double halfside, double factor )
{
    float water_radius = halfside/2. ; 
    float air_halfside = halfside*factor ; 
    float rock_halfside = 2.*halfside*factor ; 

    G4Material* water_material  = G4Material::GetMaterial("Water");   assert(water_material); 
    G4Material* air_material  = G4Material::GetMaterial("Air");   assert(air_material); 
    G4Material* rock_material = G4Material::GetMaterial("Rock");  assert(rock_material); 

    G4Orb* water_solid = new G4Orb("water_solid", water_radius ); 
    G4Box* air_solid = new G4Box("air_solid", air_halfside, air_halfside, air_halfside );
    G4Box* rock_solid = new G4Box("rock_solid", rock_halfside, rock_halfside, rock_halfside );

    G4LogicalVolume* water_lv = new G4LogicalVolume( water_solid, water_material, "water_lv"); 
    G4LogicalVolume* air_lv = new G4LogicalVolume( air_solid, air_material, "air_lv"); 
    G4LogicalVolume* rock_lv = new G4LogicalVolume( rock_solid, rock_material, "rock_lv" ); 

    G4VPhysicalVolume* water_pv = new G4PVPlacement(0,G4ThreeVector(), water_lv ,"water_pv", air_lv,false,0);
    G4VPhysicalVolume* air_pv = new G4PVPlacement(0,G4ThreeVector(),   air_lv ,  "air_pv",  rock_lv,false,0);
    G4VPhysicalVolume* rock_pv = new G4PVPlacement(0,G4ThreeVector(),  rock_lv ,  "rock_pv", nullptr,false,0);

    assert( water_pv ); 
    assert( air_pv ); 
    assert( rock_pv ); 

    G4LogicalBorderSurface* air_rock_bs = U4Surface::MakePerfectAbsorberSurface("air_rock_bs", air_pv, rock_pv );  
    assert( air_rock_bs ); 

    return rock_pv ; 
}



/**
U4VolumeMaker::WrapLVGrid
---------------------------

Returns a physical volume with the argument lv placed multiple times 
in a grid specified by (nx,ny,nz) integers. (1,1,1) yields 3x3x3 grid.

**/

G4VPhysicalVolume* U4VolumeMaker::WrapLVGrid( G4LogicalVolume* lv, int nx, int ny, int nz  )
{
    std::vector<G4LogicalVolume*> lvs ; 
    lvs.push_back(lv); 
    return WrapLVGrid(lvs, nx, ny, nz ) ; 
}


std::string U4VolumeMaker::Desc( const G4ThreeVector& tla )
{
    std::stringstream ss ; 
    ss << " tla (" 
       <<  " " << std::fixed << std::setw(10) << std::setprecision(3) << tla.x()
       <<  " " << std::fixed << std::setw(10) << std::setprecision(3) << tla.y()
       <<  " " << std::fixed << std::setw(10) << std::setprecision(3) << tla.z()
       <<  ") " 
       ;

    std::string s = ss.str(); 
    return s ; 
}

/**
U4VolumeMaker::WrapLVGrid
---------------------------

The LV provided in the lvs vector are arranged in a grid this is placed within a box. 
Grid ranges::

    -nx:nx+1
    -ny:ny+1
    -nz:nz+1

Example (nx,ny,nz):

1,1,1 
     yields a grid with 3 elements on each side, for 3*3*3=27 
0,0,0
     yields a single element 

**/

G4VPhysicalVolume* U4VolumeMaker::WrapLVGrid( std::vector<G4LogicalVolume*>& lvs, int nx, int ny, int nz  )
{
    unsigned num_lv = lvs.size(); 

    int extent = std::max(std::max(nx, ny), nz); 
    G4double sc = 500. ; 
    G4double halfside = sc*extent*3. ; 

    LOG(LEVEL) 
        << " num_lv " << num_lv 
        << " (nx,ny,nz) (" << nx << "," << ny << " " << nz << ")" 
        << " extent " << extent
        << " halfside " << halfside
        ;

    G4VPhysicalVolume* world_pv = WorldBox(halfside); 
    G4LogicalVolume* world_lv = world_pv->GetLogicalVolume(); 

    unsigned count = 0 ; 
    for(int ix=-nx ; ix < nx+1 ; ix++)
    for(int iy=-ny ; iy < ny+1 ; iy++)
    for(int iz=-nz ; iz < nz+1 ; iz++)
    {   
        G4LogicalVolume* ulv = lvs[count%num_lv] ; 
        count += 1 ; 
        assert( ulv ); 

         const G4String& ulv_name_ = ulv->GetName();  
         std::string ulv_name = ulv_name_ ; 
         ulv_name += "_plc_" ; 

        const char* iname = GridName( ulv_name.c_str(), ix, iy, iz, "" );    
        G4ThreeVector tla( sc*double(ix), sc*double(iy), sc*double(iz) );  

        LOG(LEVEL) 
           << " G4PVPlacement "
           << " count " << std::setw(3) << count 
           << Desc(tla)
           <<  iname
           ;


        G4VPhysicalVolume* pv_n = new G4PVPlacement(0, tla, ulv ,iname,world_lv,false,0);
        assert( pv_n );  
    }   
    return world_pv ; 
}


const char* U4VolumeMaker::GridName(const char* prefix, int ix, int iy, int iz, const char* suffix)
{
    std::stringstream ss ; 
    ss << prefix << ix << "_" << iy << "_" << iz << suffix ; 
    std::string s = ss.str();
    return strdup(s.c_str()); 
}


