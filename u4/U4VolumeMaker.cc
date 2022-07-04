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
const char* U4VolumeMaker::GEOM = SSys::getenvvar("GEOM", "BoxOfScintillator"); 

std::string U4VolumeMaker::Desc() // static
{
    std::stringstream ss ; 
    ss << "U4VolumeMaker::Desc" ; 
    ss << " GEOM " << GEOM ; 
#ifdef WITH_PMTSIM
    ss << " WITH_PMTSIM " ; 
#else
    ss << " not-WITH_PMTSIM " ; 
#endif
    std::string s = ss.str(); 
    return s ; 
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
U4VolumeMaker::PV
---------------------

Invokes several PV getter methods the first to provide a PV wins.

PVP_
    PMTSim getter, requiring WITH_PMTSIM macro to be set meaning that PMTSim pkg was found by CMake
PVS_
    Specials provided locally 
PVL_
    For names starting with List, multiple volumes are created and arranged eg into a grid. 
    The names to create are extracted using the name argumnent as a comma delimited list
PV1_
    Places single lv once or duplicates it several times depending on 
    strings found in the name such as Grid,Cube,Xoff,Yoff,Zoff

**/

G4VPhysicalVolume* U4VolumeMaker::PV(){ return PV(GEOM); }
G4VPhysicalVolume* U4VolumeMaker::PV(const char* name)
{
    LOG(LEVEL) << "[" << name ; 
    G4VPhysicalVolume* pv = nullptr ; 
    if(pv == nullptr) pv = PVP_(name); 
    if(pv == nullptr) pv = PVS_(name); 
    if(pv == nullptr) pv = PVL_(name); 
    if(pv == nullptr) pv = PV1_(name); 
    if(pv == nullptr) LOG(error) << "returning nullptr for name [" << name << "]" ; 
    LOG(LEVEL) << "]" << name ; 
    return pv ; 
}

/**
U4VolumeMaker::PVP_ : Get PMTSim PV
------------------------------------ 

PMTSim::HasManagerPrefix 
    returns true for names starting with one of: hama, nnvt, hmsk, nmsk, lchi

PMTSim::GetLV PMTSim::GetPV
     these methods act as go betweens to the underlying managers with prefixes
     that identify the managers offset  


TODO: need to generalize the wrapping  

**/

G4VPhysicalVolume* U4VolumeMaker::PVP_(const char* name)
{
    G4VPhysicalVolume* pv = nullptr ; 
#ifdef WITH_PMTSIM
    bool has_manager_prefix = PMTSim::HasManagerPrefix(name) ;
    LOG(LEVEL) << "[ WITH_PMTSIM name [" << name << "] has_manager_prefix " << has_manager_prefix ; 
    if(has_manager_prefix) 
    {
        G4LogicalVolume* lv = PMTSim::GetLV(name) ; 
        if( lv == nullptr ) LOG(fatal) << "PMTSim::GetLV returned nullptr for name [" << name << "]" ; 
        assert( lv ); 

        //pv = WrapVacuum( lv, 5000. ) ;          
        pv = WrapRockWater( lv, 5000. ) ;          

    }
    LOG(LEVEL) << "]" ; 
#else
    LOG(info) << " not-WITH_PMTSIM name [" << name << "]" ; 
#endif
    return pv ; 
}


G4VPhysicalVolume* U4VolumeMaker::PVS_(const char* name)
{
    G4VPhysicalVolume* pv = nullptr ; 
    if(strcmp(name,"BoxOfScintillator" ) == 0)     pv = BoxOfScintillator(1000.);   
    if(strcmp(name,"RaindropRockAirWater" ) == 0)  pv = RaindropRockAirWater();   
    if(strcmp(name,"RaindropRockAirWater2" ) == 0) pv = RaindropRockAirWater2();   
    return pv ; 
}
G4VPhysicalVolume* U4VolumeMaker::PVL_(const char* name)
{
    if(!SStr::StartsWith(name, "List")) return nullptr  ; 
    std::vector<G4LogicalVolume*> lvs ; 
    LV(lvs, name + strlen("List") ); 
    G4VPhysicalVolume* pv = WrapLVGrid(lvs, 1, 1, 1 ); 
    return pv ; 
}
G4VPhysicalVolume* U4VolumeMaker::PV1_(const char* name)
{
    G4VPhysicalVolume* pv = nullptr ; 
    bool grid = strstr(name, "Grid") != nullptr ; 
    bool cube = strstr(name, "Cube") != nullptr ; 
    bool xoff = strstr(name, "Xoff") != nullptr ; 
    bool yoff = strstr(name, "Yoff") != nullptr ; 
    bool zoff = strstr(name, "Zoff") != nullptr ; 

    G4LogicalVolume* lv = LV(name) ; 

    if(grid)      pv =   WrapLVGrid(     lv, 1, 1, 1 ); 
    else if(cube) pv =   WrapLVCube(     lv, 100., 100., 100. ); 
    else if(xoff) pv =   WrapLVOffset(   lv, 200.,   0.,   0. ); 
    else if(yoff) pv =   WrapLVOffset(   lv,   0., 200.,   0. ); 
    else if(zoff) pv =   WrapLVOffset(   lv,   0.,   0., 200. ); 
    else          pv =   WrapLVCube(     lv,   0.,   0.,   0. ); 
    return pv ; 
}


/**
U4VolumeMaker::LV
---------------------------

Note the generality: 

* U4SolidMaker::Make can create many different solid shapes based on the start of the name
* U4Material::FindMaterialName extracts the material name by looking for material names within *name*

HMM: maybe provide PMTSim LV this way too ? Based on some prefix ? 

**/

G4LogicalVolume*  U4VolumeMaker::LV(const char* name)
{
    const G4VSolid* solid_  = U4SolidMaker::Make(name); 
    G4VSolid* solid = const_cast<G4VSolid*>(solid_); 
    const char* matname = U4Material::FindMaterialName(name) ; 
    G4Material* material = U4Material::Get( matname ? matname : U4Material::VACUUM ); 
    G4LogicalVolume* lv = new G4LogicalVolume( solid, material, name, 0,0,0,true ); 
    return lv ; 
}

/**
U4VolumeMaker::LV vector interface : creates multiple LV using delimited names string 
--------------------------------------------------------------------------------------
**/

void U4VolumeMaker::LV(std::vector<G4LogicalVolume*>& lvs , const char* names_, char delim )
{
    std::vector<std::string> names ; 
    SStr::Split(names_ , delim, names ); 
    unsigned num_names = names.size(); 
    LOG(LEVEL) << " names_ " << names_ << " num_names " << num_names ;  
    assert( num_names > 1 ); 

    for(unsigned i=0 ; i < num_names ; i++)
    {
        const char* name = names[i].c_str(); 
        G4LogicalVolume* lv = LV(name) ; 
        assert(lv); 
        lvs.push_back(lv);   
    }
}

/**
U4VolumeMaker::WrapRockWater
-------------------------------

**/

G4VPhysicalVolume* U4VolumeMaker::WrapRockWater( G4LogicalVolume* item_lv, double halfside  )
{
    LOG(LEVEL) << "["  ; 

    G4LogicalVolume*  rock_lv  = Box_(2.*halfside, "Rock" );
    G4LogicalVolume*  water_lv = Box_(1.*halfside, "Water" );
 
    G4VPhysicalVolume* item_pv  = Place(item_lv,  water_lv);  assert( item_pv ); 
    G4VPhysicalVolume* water_pv = Place(water_lv,  rock_lv);  assert( water_pv ); 
    G4VPhysicalVolume* rock_pv  = Place(rock_lv,  nullptr );  

    G4LogicalBorderSurface* water_rock_bs = U4Surface::MakePerfectAbsorberSurface("water_rock_bs", water_pv, rock_pv );  
    assert( water_rock_bs ); 

    LOG(LEVEL) << "]"  ; 
    return rock_pv ; 
}


/**
U4VolumeMaker::WrapVacuum
----------------------------

The LV provided is placed within a WorldBox of halfside extent and the world PV is returned. 

**/

G4VPhysicalVolume* U4VolumeMaker::WrapVacuum( G4LogicalVolume* item_lv, double halfside  )
{
    G4LogicalVolume*   vac_lv  = Box_(halfside, "Vacuum" );
 
    G4VPhysicalVolume* item_pv = Place(item_lv, vac_lv);   assert( item_pv ); 
    G4VPhysicalVolume* vac_pv  = Place(vac_lv, nullptr );

    return vac_pv ;      
}





/**
U4VolumeMaker::WrapLVGrid
---------------------------

Returns a physical volume with the argument lv placed multiple times 
in a grid specified by (nx,ny,nz) integers. (1,1,1) yields 3x3x3 grid.

The vector argument method places the lv in a grid within a box. 
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

G4VPhysicalVolume* U4VolumeMaker::WrapLVGrid( G4LogicalVolume* lv, int nx, int ny, int nz  )
{
    std::vector<G4LogicalVolume*> lvs ; 
    lvs.push_back(lv); 
    return WrapLVGrid(lvs, nx, ny, nz ) ; 
}

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


/**
U4VolumeMaker::WrapLVOffset
-----------------------------

This is used from PV1_ for Xoff Yoff Zoff names

1. use maximum of tx,ty,tz to define world box halfside 
2. places lv within world volume 
3. adds BoxMinusOrb at origin 

**/

G4VPhysicalVolume* U4VolumeMaker::WrapLVOffset( G4LogicalVolume* lv, double tx, double ty, double tz )
{
    double halfside = 3.*std::max( std::max( tx, ty ), tz ); 
    assert( halfside > 0. ); 

    G4VPhysicalVolume* world_pv = WorldBox(halfside); 
    G4LogicalVolume* world_lv = world_pv->GetLogicalVolume(); 

    AddPlacement(world_lv, lv, tx, ty, tz ); 

    bool bmo = true ; 
    if(bmo) AddPlacement( world_lv, "BoxMinusOrb", 0., 0., 0. ); 

    return world_pv ;  
}

/**
U4VolumeMaker::WrapLVCube
-------------------------------

This if used from PV1_ for "Cube" string.  
Places the lv at 8 positions at the corners of a cube. 

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

G4VPhysicalVolume* U4VolumeMaker::WrapLVCube( G4LogicalVolume* lv, double tx, double ty, double tz )
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
U4VolumeMaker::AddPlacement : Translation places *lv* within *mother_lv*  
---------------------------------------------------------------------------

Used from U4VolumeMaker::WrapLVOffset

**/

G4VPhysicalVolume* U4VolumeMaker::AddPlacement( G4LogicalVolume* mother_lv,  G4LogicalVolume* lv,  double tx, double ty, double tz )
{
    G4ThreeVector tla(tx,ty,tz); 
    const char* pv_name = SStr::Name( lv->GetName().c_str(), "_placement" ); 
    LOG(LEVEL) << Desc(tla) << " " << pv_name ;  
    G4VPhysicalVolume* pv = new G4PVPlacement(0,tla,lv,pv_name,mother_lv,false,0);
    return pv ; 
} 
G4VPhysicalVolume* U4VolumeMaker::AddPlacement( G4LogicalVolume* mother, const char* name,  double tx, double ty, double tz )
{
    G4LogicalVolume* lv = LV(name); 
    return AddPlacement( mother, lv, tx, ty, tz ); 
}

const char* U4VolumeMaker::GridName(const char* prefix, int ix, int iy, int iz, const char* suffix)
{
    std::stringstream ss ; 
    ss << prefix << ix << "_" << iy << "_" << iz << suffix ; 
    std::string s = ss.str();
    return strdup(s.c_str()); 
}


/**
U4VolumeMaker::WorldBox
--------------------------

Used from U4VolumeMaker::WrapLVOffset U4VolumeMaker::WrapLVCube

**/

G4VPhysicalVolume* U4VolumeMaker::WorldBox( double halfside, const char* mat )
{
    return Box(halfside, mat, "World", nullptr ); 
}
G4VPhysicalVolume* U4VolumeMaker::BoxOfScintillator( double halfside )
{
    return BoxOfScintillator(halfside, "BoxOfScintillator", nullptr ); 
}
G4VPhysicalVolume* U4VolumeMaker::BoxOfScintillator( double halfside, const char* prefix, G4LogicalVolume* mother_lv )
{
    return Box(halfside, U4Material::SCINTILLATOR, "BoxOfScintillator", mother_lv);
}
G4VPhysicalVolume* U4VolumeMaker::Box(double halfside, const char* mat, const char* prefix, G4LogicalVolume* mother_lv )
{
    if(prefix == nullptr) prefix = mat ; 
    G4LogicalVolume* lv = Box_(halfside, mat, prefix); 
    return Place(lv, mother_lv); 
}

G4VPhysicalVolume* U4VolumeMaker::Place( G4LogicalVolume* lv, G4LogicalVolume* mother_lv )
{
    const char* lv_name = lv->GetName().c_str() ; 
    const char* pv_name = SStr::Name(lv_name, "_pv") ; 
    return new G4PVPlacement(0,G4ThreeVector(), lv, pv_name, mother_lv, false, 0);
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

void U4VolumeMaker::RaindropRockAirWater_Configure( double& rock_halfside, double& air_halfside, double& water_radius )
{
    double halfside = SSys::getenvdouble(U4VolumeMaker_RaindropRockAirWater_HALFSIDE, 100.); 
    double factor   = SSys::getenvdouble(U4VolumeMaker_RaindropRockAirWater_FACTOR,   1.); 

    LOG(LEVEL) << U4VolumeMaker_RaindropRockAirWater_HALFSIDE << " " << halfside ; 
    LOG(LEVEL) << U4VolumeMaker_RaindropRockAirWater_FACTOR   << " " << factor ; 
 
    rock_halfside = 2.*halfside*factor ; 
    air_halfside = halfside*factor ; 
    water_radius = halfside/2. ; 
}

G4VPhysicalVolume* U4VolumeMaker::RaindropRockAirWater()
{
    double rock_halfside, air_halfside, water_radius ; 
    RaindropRockAirWater_Configure( rock_halfside, air_halfside, water_radius); 

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
U4VolumeMaker::RaindropRockAirWater2
--------------------------------------

Notice that so long as all the LV are created prior to creating the PV, 
which need the LV for placement and mother logical, there is no need to be 
careful with creation order of the volumes. 

This is suggestive of how to organize the API, instead of focussing on methods 
to create PV it is more flexible to have API that create LV that are then put 
together by the higher level methods that make less sense to generalize. 

**/

G4VPhysicalVolume* U4VolumeMaker::RaindropRockAirWater2()
{
    double rock_halfside, air_halfside, water_radius ; 
    RaindropRockAirWater_Configure( rock_halfside, air_halfside, water_radius ); 

    G4LogicalVolume* rock_lv  = Box_(rock_halfside, "Rock" ); 
    G4LogicalVolume* air_lv   = Box_(air_halfside, "Air" ); 
    G4LogicalVolume* water_lv = Orb_(water_radius, "Water" ); 

    G4VPhysicalVolume* rock_pv  = new G4PVPlacement(0,G4ThreeVector(), rock_lv ,  "rock_pv", nullptr,false,0);
    G4VPhysicalVolume* air_pv   = new G4PVPlacement(0,G4ThreeVector(), air_lv  ,  "air_pv",  rock_lv,false,0);
    G4VPhysicalVolume* water_pv = new G4PVPlacement(0,G4ThreeVector(), water_lv , "water_pv", air_lv,false,0);

    assert( rock_pv ); 
    assert( air_pv ); 
    assert( water_pv ); 

    G4LogicalBorderSurface* air_rock_bs = U4Surface::MakePerfectAbsorberSurface("air_rock_bs", air_pv, rock_pv );  
    assert( air_rock_bs ); 

    return rock_pv ; 
}
G4LogicalVolume* U4VolumeMaker::Orb_( double radius, const char* mat, const char* prefix )
{
    if( prefix == nullptr ) prefix = mat ; 
    G4Material* material  = U4Material::Get(mat);   assert(material); 
    G4Orb* solid = new G4Orb( SStr::Name(prefix,"_solid"), radius ); 
    G4LogicalVolume* lv = new G4LogicalVolume( solid, material, SStr::Name(prefix,"_lv")); 
    return lv ; 
}
G4LogicalVolume* U4VolumeMaker::Box_( double halfside, const char* mat, const char* prefix )
{
    if( prefix == nullptr ) prefix = mat ; 
    G4Material* material  = U4Material::Get(mat);   assert(material); 
    G4Box* solid = new G4Box( SStr::Name(prefix,"_solid"), halfside, halfside, halfside );
    G4LogicalVolume* lv = new G4LogicalVolume( solid, material, SStr::Name(prefix,"_lv")); 
    return lv ; 
}




