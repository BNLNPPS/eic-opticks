/*
 * Copyright (c) 2019 Opticks Team. All Rights Reserved.
 *
 * This file is part of Opticks
 * (see https://bitbucket.org/simoncblyth/opticks).
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <csignal>


#include "snode.h"
#include "stree.h"

#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4Material.hh"
#include "G4VSolid.hh"
#include "G4TransportationManager.hh"
#include "G4VSensitiveDetector.hh"
#include "G4PVPlacement.hh"

#include "G4OpticalPhoton.hh"
#include "G4ProcessManager.hh"


#include "X4.hh"
#include "X4PhysicalVolume.hh"
#include "X4Material.hh"
#include "X4MaterialWater.hh"
#include "X4MaterialTable.hh"
#include "X4Scintillation.hh"
#include "X4LogicalBorderSurface.hh"
#include "X4LogicalBorderSurfaceTable.hh"
#include "X4LogicalSkinSurfaceTable.hh"
#include "X4Solid.hh"
#include "X4CSG.hh"
#include "X4GDMLParser.hh"
#include "X4Mesh.hh"
#include "X4Transform3D.hh"

#include "SStr.hh"
#include "SSys.hh"
#include "SDigest.hh"
#include "SGDML.hh"
#include "SLOG.hh"

#include "BStr.hh"
#include "BFile.hh"
#include "BTimeStamp.hh"
#include "SOpticksKey.hh"


#include "NXform.hpp"  // header with the implementation
template struct nxform<X4Nd> ; 

#include "NGLMExt.hpp"
#include "NCSG.hpp"
#include "NNode.hpp"
#include "NNodeNudger.hpp"
#include "NTreeProcess.hpp"
#include "GLMFormat.hpp"

#include "GMesh.hh"
#include "GVolume.hh"

#include "GPt.hh"
#include "GPts.hh"

#include "GGeo.hh"
#include "GMaterial.hh"
#include "GMaterialLib.hh"
#include "GScintillatorLib.hh"
#include "GSurfaceLib.hh"
#include "GBorderSurface.hh"
#include "GSkinSurface.hh"
#include "GBndLib.hh"
#include "GMeshLib.hh"
#include "GProperty.hh"

#include "Opticks.hh"
#include "OpticksQuery.hh"

/**
X4PhysicalVolume
==================


**/


const plog::Severity X4PhysicalVolume::LEVEL = SLOG::EnvLevel("X4PhysicalVolume", "DEBUG") ;
const bool           X4PhysicalVolume::DBG = true ;



const G4VPhysicalVolume* const X4PhysicalVolume::Top()
{
    G4TransportationManager* tranMgr = G4TransportationManager::GetTransportationManager() ; 
    G4Navigator* nav = tranMgr->GetNavigatorForTracking() ; 
    const G4VPhysicalVolume* const top = nav->GetWorldVolume() ;
    return top ; 
}

GGeo* X4PhysicalVolume::Convert(const G4VPhysicalVolume* const top, const char* argforce)
{
    assert(0) ; // NOT USED IN NEW WORKFLOW : NOW USING X4Geo::Translate ? 

    const char* key = X4PhysicalVolume::Key(top) ; 

    SOpticksKey::SetKey(key);

    LOG(error) << " SetKey " << key  ; 

    Opticks* ok = new Opticks(0,0, argforce);  // Opticks instanciation must be after SOpticksKey::SetKey

    ok->configure();   // see notes/issues/configuration_resource_rejig.rst

    GGeo* gg = new GGeo(ok) ;

    X4PhysicalVolume xtop(gg, top) ;   // <-- populates gg 

    return gg ; 
}




X4PhysicalVolume::X4PhysicalVolume(GGeo* ggeo, const G4VPhysicalVolume* const top)
    :
    X4Named("X4PhysicalVolume"),
    m_tree(nullptr),
    m_ggeo(ggeo),
    m_top(top),
    m_ok(m_ggeo->getOpticks()), 
    m_lvsdname(m_ok->getLVSDName()),
    m_query(m_ok->getQuery()),
    m_gltfpath(m_ok->getGLTFPath()),

    m_mlib(m_ggeo->getMaterialLib()),
    m_sclib(m_ggeo->getScintillatorLib()),
    m_slib(m_ggeo->getSurfaceLib()),
    m_blib(m_ggeo->getBndLib()),
    m_hlib(m_ggeo->getMeshLib()),
    //m_meshes(m_hlib->getMeshes()), 
    m_xform(new nxform<X4Nd>(0,false)),
    m_verbosity(m_ok->getVerbosity()),
    m_node_count(0),
    m_selected_node_count(0),
#ifdef X4_PROFILE
    m_convertNode_dt(0.f),
    m_convertNode_boundary_dt(0.f),
    m_convertNode_transformsA_dt(0.f),
    m_convertNode_transformsB_dt(0.f),
    m_convertNode_transformsC_dt(0.f),
    m_convertNode_transformsD_dt(0.f),
    m_convertNode_transformsE_dt(0.f),
    m_convertNode_GVolume_dt(0.f),
#endif
#ifdef X4_TRANSFORM
    m_is_identity0(0),
    m_is_identity1(0),
#endif
    m_dummy(0)
{
    const char* msg = "GGeo ctor argument of X4PhysicalVolume must have mlib, slib, blib and hlib already " ; 

    // trying to Opticks::configure earlier, from Opticks::init trips these asserts
    assert( m_mlib && msg ); 
    assert( m_slib && msg ); 
    assert( m_blib && msg ); 
    assert( m_hlib && msg ); 

    init();
}

GGeo* X4PhysicalVolume::getGGeo()
{
    return m_ggeo ; 
}

void X4PhysicalVolume::init()
{
    LOG(LEVEL) << "[" ; 
    LOG(LEVEL) << " query : " << m_query->desc() ; 


    convertWater();       // special casing in Geant4 forces special casing here
    convertMaterials();   // populate GMaterialLib
    convertScintillators(); 


    convertSurfaces();    // populate GSurfaceLib
    closeSurfaces();
    convertSolids();      // populate GMeshLib with GMesh converted from each G4VSolid (postorder traverse processing first occurrence of G4LogicalVolume)  
    convertStructure();   // populate GNodeLib with GVolume converted from each G4VPhysicalVolume (preorder traverse) 
    convertCheck();       // checking found some nodes

    postConvert();        // just reporting 

    LOG(LEVEL) << "]" ; 
}



void X4PhysicalVolume::postConvert() const 
{
    LOG(LEVEL) 
        << " GGeo::getNumVolumes() " << m_ggeo->getNumVolumes() 
        << " GGeo::getNumSensorVolumes() " << m_ggeo->getNumSensorVolumes() 
        << std::endl 
        << " GGeo::getSensorBoundaryReport() "
        << std::endl 
        << m_ggeo->getSensorBoundaryReport()
        ;


    LOG(LEVEL) << m_blib->getAddBoundaryReport() ;

    // too soon for sensor dumping as instances not yet formed, see GGeo::postDirectTranslationDump 
    //m_ggeo->dumpSensorVolumes("X4PhysicalVolume::postConvert"); 

    //m_ggeo->dumpSurfaces("X4PhysicalVolume::postConvert" ); 


    LOG(info) << m_blib->descSensorBoundary() ; 


}




bool X4PhysicalVolume::hasEfficiency(const G4Material* mat)
{
    return std::find(m_material_with_efficiency.begin(), m_material_with_efficiency.end(), mat ) != m_material_with_efficiency.end() ;
}


/**
X4PhysicalVolume::convertMaterials
-----------------------------------

1. recursive traverse collecting actively used material pointers into m_mtlist vector 
2. X4MaterialTable::Convert via X4MaterialTable::init populates GMaterialLib with the methods::

   GMaterialLib::add
   GMaterialLib::addRaw
   GPropertyLib::addRawEnergy

   
**/

void X4PhysicalVolume::convertMaterials()
{
    OK_PROFILE("_X4PhysicalVolume::convertMaterials");
    LOG(LEVEL) << "[" ; 

    const G4VPhysicalVolume* pv = m_top ; 
    int depth = 0 ;
    convertMaterials_r(pv, depth);

    LOG(LEVEL) << X4Material::Desc(m_mtlist); 

    const std::vector<G4Material*>& used_materials = m_mtlist ; 
    X4MaterialTable::Convert(m_mlib, m_material_with_efficiency, used_materials );
    size_t num_material_with_efficiency = m_material_with_efficiency.size() ;

    m_mlib->close();   // may change order if prefs dictate

    LOG(verbose) << "]" ;
    LOG(LEVEL)
          << " used_materials.size " << used_materials.size()
          << " num_material_with_efficiency " << num_material_with_efficiency
          ; 

    m_mlib->dumpSensitiveMaterials("X4PhysicalVolume::convertMaterials");


    LOG(LEVEL) << "]" ;
    OK_PROFILE("X4PhysicalVolume::convertMaterials");
}



const char* X4PhysicalVolume::SCINTILLATOR_PROPERTIES = "SLOWCOMPONENT,FASTCOMPONENT,REEMISSIONPROB" ; 


void X4PhysicalVolume::convertScintillators_OLD()
{
    LOG(LEVEL) << "[" ; 

    assert( m_sclib ); 

    std::vector<GMaterial*>  scintillators_raw = m_mlib->getRawMaterialsWithProperties(SCINTILLATOR_PROPERTIES, ',' );  

    unsigned num_scint = scintillators_raw.size() ; 

    typedef GPropertyMap<double> PMAP ;  

    if(num_scint == 0)
    {   
        LOG(LEVEL) << " found no scintillator materials  " ; 
    }   
    else
    {   
        LOG(LEVEL) << " found " << num_scint << " scintillator materials  " ; 

        for(unsigned i=0 ; i < num_scint ; i++)
        {   
            GMaterial* mat_ = scintillators_raw[i] ; 

            PMAP* mat = dynamic_cast<PMAP*>(mat_);  

            m_sclib->addRaw(mat);
        }   
        m_sclib->close();   // creates and sets "THE" buffer 
    }   

    LOG(LEVEL) << "]" ; 
}


/**
X4PhysicalVolume::collectScintillatorMaterials
------------------------------------------------

Invoked by X4PhysicalVolume::init/X4PhysicalVolume::convertScintillators


1. search GMaterialLib for materials with the three SCINTILLATOR_PROPERTIES, when none found there is nothing to do here
2. when scintillators are found assert that both wavelength and energy domain versions of the materials are present
3. copy the raw wavelength and energy material pointers from GMaterialLib to GScintillatorLib in order for these
   materials to get persisted in the GScintillatorLib geocache directory. 

Note that both wavelength and unusually energy domain materials are collected 
as some Geant4 integration code is required.

**/

void X4PhysicalVolume::collectScintillatorMaterials()
{
    LOG(LEVEL) << "[" ; 
    assert( m_sclib ); 
    std::vector<GMaterial*>  scintillators_raw = m_mlib->getRawMaterialsWithProperties(SCINTILLATOR_PROPERTIES, ',' );  

    typedef GPropertyMap<double> PMAP ;  
    std::vector<PMAP*> raw_energy_pmaps ;  
    m_mlib->findRawOriginalMapsWithProperties( raw_energy_pmaps, SCINTILLATOR_PROPERTIES, ',' );

    bool consistent = scintillators_raw.size() == raw_energy_pmaps.size()  ;
    LOG_IF(fatal, !consistent) 
        << " scintillators_raw.size " << scintillators_raw.size()
        << " raw_energy_pmaps.size " << raw_energy_pmaps.size()
        ; 
 
    assert( consistent ); 
    unsigned num_scint = scintillators_raw.size() ; 

    if(num_scint == 0)
    {   
        LOG(LEVEL) << " found no scintillator materials  " ; 
        return ; 
    }   

    LOG(LEVEL) << " found " << num_scint << " scintillator materials  " ; 

    // wavelength domain 
    for(unsigned i=0 ; i < num_scint ; i++)
    {   
        GMaterial* mat_ = scintillators_raw[i] ; 
        PMAP* mat = dynamic_cast<PMAP*>(mat_);
        m_sclib->addRaw(mat);      
    }   

    // original energy domain 
    for(unsigned i=0 ; i < num_scint ; i++)
    {
        PMAP* pmap = raw_energy_pmaps[i] ; 
        m_sclib->addRawOriginal(pmap);      
    }

    LOG(LEVEL) << "[ m_sclib.desc " ; 
    LOG(LEVEL) << m_sclib->desc(); 
    LOG(LEVEL) << "] m_sclib.desc " ; 

    LOG(LEVEL) << "]" ; 
}

void X4PhysicalVolume::createScintillatorGeant4InterpolatedICDF()
{
    unsigned num_scint = m_sclib->getNumRawOriginal() ; 
    if( num_scint == 0 ) return ; 
    //assert( num_scint == 1 ); 

    typedef GPropertyMap<double> PMAP ;  
    PMAP* pmap_en = m_sclib->getRawOriginal(0u); 
    assert( pmap_en ); 
    assert( pmap_en->hasOriginalDomain() ); 

    NPY<double>* slow_en = pmap_en->getProperty("SLOWCOMPONENT")->makeArray(); 
    NPY<double>* fast_en = pmap_en->getProperty("FASTCOMPONENT")->makeArray(); 

    //slow_en->save("/tmp/slow_en.npy"); 
    //fast_en->save("/tmp/fast_en.npy"); 

    X4Scintillation xs(slow_en, fast_en); 

    unsigned num_bins = 4096 ; 
    unsigned hd_factor = 20 ; 
    const char* material_name = pmap_en->getName() ;  

    NPY<double>* g4icdf = xs.createGeant4InterpolatedInverseCDF(num_bins, hd_factor, material_name ) ;

    LOG(LEVEL) 
        << " num_scint " << num_scint
        << " slow_en " << slow_en->getShapeString()
        << " fast_en " << fast_en->getShapeString()
        << " num_bins " << num_bins
        << " hd_factor " << hd_factor
        << " material_name " << material_name
        << " g4icdf " << g4icdf->getShapeString()
        ;

    m_sclib->setGeant4InterpolatedICDF(g4icdf);   // trumps legacyCreateBuffer
    m_sclib->close();   // creates and sets "THE" buffer 
}



void X4PhysicalVolume::convertScintillators()
{
    LOG(LEVEL) << "[" ; 
    collectScintillatorMaterials(); 

#ifdef __APPLE__
    //LOG(fatal) << " __APPLE__ early SIGINT " ; 
    //std::raise(SIGINT); 
#endif

    createScintillatorGeant4InterpolatedICDF(); 
    LOG(LEVEL) << "]" ; 
}








/**
X4PhysicalVolume::convertWater
--------------------------------

Geant4 special casing of G4Material with name "Water" in G4OpRayleigh 
that do not have a "RAYLEIGH" property but do have a "RINDEX" property
makes it necessary for this separate X4PhysicalVolume::convertWater.

The instanciation of X4MaterialWater in this method uses a G4OpRayleigh 
process instance internally to calculate the RAYLEIGH scattering length 
from the RINDEX and adds that new property to the properties of the "Water" G4Material. 

This is needed as this Opticks convert is based on materials and does not look 
inside G4OpRayleigh::thePhysicsTable. In any case standard process setup 
happens much later that the Opticks geometry conversion is typically done.

**/

void X4PhysicalVolume::convertWater()
{
    bool applicable = X4MaterialWater::IsApplicable(); 
    if(!applicable)
    {
        LOG(LEVEL) << "X4MaterialWater::IsApplicable returned false, so cannot calculate \"RAYLEIGH\" using G4OpRayleigh " ;  
        return ; 
    }

    X4MaterialWater Water ;  
    assert(Water.rayleigh);  

    G4MaterialPropertyVector* RAYLEIGH = X4MaterialWater::GetRAYLEIGH() ; 
    assert( RAYLEIGH == Water.rayleigh ); 
}



/**
X4PhysicalVolume::convertMaterials_r
--------------------------------------

Recursive collection of pointers for all materials referenced from active LV 
into m_mtlist in postorder of first encounter.

**/
void X4PhysicalVolume::convertMaterials_r(const G4VPhysicalVolume* const pv, int depth)
{
    const G4LogicalVolume* lv = pv->GetLogicalVolume() ;

    for (size_t i=0 ; i < size_t(lv->GetNoDaughters()) ;i++ )  // G4LogicalVolume::GetNoDaughters returns 1042:G4int, 1062:size_t
    {
        const G4VPhysicalVolume* const daughter_pv = lv->GetDaughter(i);
        convertMaterials_r( daughter_pv , depth + 1 );
    }

    G4Material* mt = lv->GetMaterial() ; 
    if(mt && std::find(m_mtlist.begin(), m_mtlist.end(), mt) == m_mtlist.end()) m_mtlist.push_back(mt);  
}






/**
X4PhysicalVolume::convertImplicitSurfaces_r
---------------------------------------------

Recursively look for "implicit" surfaces, eg from the mis-use of Geant4 NoRINDEX 
behaviour to effectively mimic surface absorption without actually setting an 100% 
absorption property. See::

   g4-cls G4OpBoundaryProcess

Photons at the border from a transparent material with RINDEX (eg Water) 
to a NoRINDEX material (eg Tyvek) get fStopAndKill-ed by G4OpBoundaryProcess::PostStepDoIt

RINDEX_NoRINDEX
    parent with RINDEX but daughter without, 
    there tend to be large numbers of these 
    eg from every piece of metal in Air or Water etc..

NoRINDEX_RINDEX
    parent without RINDEX and daughter with,
    there tend to be only a few of these : but they are optically important as "containers" 
    eg Tyvek or Rock containing Water   

**/

void X4PhysicalVolume::convertImplicitSurfaces_r(const G4VPhysicalVolume* const parent_pv, int depth)
{
    const G4LogicalVolume* parent_lv = parent_pv->GetLogicalVolume() ;
    const G4Material* parent_mt = parent_lv->GetMaterial() ; 
    const G4String& parent_mtName = parent_mt->GetName(); 

    G4MaterialPropertiesTable* parent_mpt = parent_mt->GetMaterialPropertiesTable();
    const G4MaterialPropertyVector* parent_rindex = parent_mpt ? parent_mpt->GetProperty(kRINDEX) : nullptr ;     // WHAT: cannot do this with const mpt 

    for (size_t i=0 ; i < size_t(parent_lv->GetNoDaughters()) ;i++ )  // G4LogicalVolume::GetNoDaughters returns 1042:G4int, 1062:size_t
    {
        const G4VPhysicalVolume* const daughter_pv = parent_lv->GetDaughter(i);
        const G4LogicalVolume* daughter_lv = daughter_pv->GetLogicalVolume() ;
        const G4Material* daughter_mt = daughter_lv->GetMaterial() ; 
        G4MaterialPropertiesTable* daughter_mpt = daughter_mt->GetMaterialPropertiesTable();  
        const G4MaterialPropertyVector* daughter_rindex = daughter_mpt ? daughter_mpt->GetProperty(kRINDEX) : nullptr ; // WHAT: cannot do this with const mpt 
        const G4String& daughter_mtName = daughter_mt->GetName(); 
  
        // naming order for outgoing photons, not ingoing volume traversal  
        bool RINDEX_NoRINDEX = daughter_rindex != nullptr && parent_rindex == nullptr ;   
        bool NoRINDEX_RINDEX = daughter_rindex == nullptr && parent_rindex != nullptr ; 

        //if(RINDEX_NoRINDEX || NoRINDEX_RINDEX)
        if(RINDEX_NoRINDEX)
        {
            const char* pv1 = X4::Name( daughter_pv ) ; 
            const char* pv2 = X4::Name( parent_pv ) ; 
            GBorderSurface* bs = m_slib->findBorderSurface(pv1, pv2);  

            LOG(LEVEL) 
               << " parent_mtName " << parent_mtName
               << " daughter_mtName " << daughter_mtName
               ;

            LOG(LEVEL)
                << " RINDEX_NoRINDEX " << RINDEX_NoRINDEX 
                << " NoRINDEX_RINDEX " << NoRINDEX_RINDEX
                << " pv1 " << std::setw(30) << pv1
                << " pv2 " << std::setw(30) << pv2
                << " bs " << bs 
                <<  ( bs ? " preexisting-border-surface-NOT-adding-implicit " : " no-prior-border-surface-adding-implicit " )
                ;


            if( bs == nullptr )
            {
                m_slib->addImplicitBorderSurface_RINDEX_NoRINDEX(pv1, pv2); 
            }
           
        } 
        convertImplicitSurfaces_r( daughter_pv , depth + 1 );
    }
}




/**
X4PhysicalVolume::convertSurfaces
-------------------------------------

Populates GSurfaceLib 

* G4LogicalSkinSurface -> GSkinSurface -> slib
* G4LogicalBorderSurface -> GBorderSurface -> slib


**/

void X4PhysicalVolume::convertSurfaces()
{
    LOG(LEVEL) << "[" ;

    size_t num_surf0, num_surf1 ; 
    num_surf0 = m_slib->getNumSurfaces() ; 
    assert( num_surf0 == 0 );

    char mode_g4interpolate = 'G' ; 
    //char mode_oldstandardize = 'S' ; 
    //char mode_asis = 'A' ; 
    char mode = mode_g4interpolate ; 

    X4LogicalBorderSurfaceTable::Convert(m_slib, mode);
    num_surf1 = m_slib->getNumSurfaces() ; 

    size_t num_lbs = num_surf1 - num_surf0 ; num_surf0 = num_surf1 ;   

    X4LogicalSkinSurfaceTable::Convert(m_slib, mode);
    num_surf1 = m_slib->getNumSurfaces() ; 

    size_t num_sks = num_surf1 - num_surf0 ; num_surf0 = num_surf1 ;  

    const G4VPhysicalVolume* pv = m_top ; 
    int depth = 0 ;
    convertImplicitSurfaces_r(pv, depth);
    num_surf1 = m_slib->getNumSurfaces() ; 

    size_t num_ibs = num_surf1 - num_surf0 ; num_surf0 = num_surf1 ;  


    //m_slib->dumpImplicitBorderSurfaces("X4PhysicalVolume::convertSurfaces");  

    m_slib->addPerfectSurfaces();
    //m_slib->dumpSurfaces("X4PhysicalVolume::convertSurfaces");

    m_slib->collectSensorIndices(); 
    //m_slib->dumpSensorIndices("X4PhysicalVolume::convertSurfaces"); 

    LOG(LEVEL) 
        << "]" 
        << " num_lbs " << num_lbs
        << " num_sks " << num_sks
        << " num_ibs " << num_ibs
        ;

}





void X4PhysicalVolume::closeSurfaces()
{
    m_slib->close();  // may change order if prefs dictate

    m_ggeo->dumpSkinSurface("dumpSkinSurface"); 
}


/**
X4PhysicalVolume::Digest
--------------------------

Looks like not succeeding to spot changes.

**/


void X4PhysicalVolume::Digest( const G4LogicalVolume* const lv, const G4int depth, SDigest* dig )
{

    for (unsigned i=0; i < unsigned(lv->GetNoDaughters()) ; i++)
    {
        const G4VPhysicalVolume* const d_pv = lv->GetDaughter(i);

        G4RotationMatrix rot, invrot;

        if (d_pv->GetFrameRotation() != 0)
        {
           rot = *(d_pv->GetFrameRotation());
           invrot = rot.inverse();
        }

        Digest(d_pv->GetLogicalVolume(),depth+1, dig);

        // postorder visit region is here after the recursive call

        G4Transform3D P(invrot,d_pv->GetObjectTranslation());

        std::string p_dig = X4Transform3D::Digest(P) ; 
    
        dig->update( const_cast<char*>(p_dig.data()), p_dig.size() );  
    }

    // Avoid pointless repetition of full material digests for every 
    // volume by digesting just the material name (could use index instead)
    // within the recursion.
    //
    // Full material digests of all properties are included after the recursion.

    G4Material* material = lv->GetMaterial();
    const G4String& name = material->GetName();    
    dig->update( const_cast<char*>(name.data()), name.size() );  

}


std::string X4PhysicalVolume::Digest( const G4VPhysicalVolume* const top, const char* digestextra, const char* digestextra2 )
{
    SDigest dig ;
    const G4LogicalVolume* lv = top->GetLogicalVolume() ;
    Digest(lv, 0, &dig ); 
    std::string mats = X4Material::Digest(); 

    dig.update( const_cast<char*>(mats.data()), mats.size() );  

    if(digestextra)
    {
        LOG(LEVEL) << "digestextra " << digestextra ; 
        dig.update_str( digestextra );  
    }
    if(digestextra2)
    {
        LOG(LEVEL) << "digestextra2 " << digestextra2 ; 
        dig.update_str( digestextra2 );  
    }


    return dig.finalize();
}


const char* X4PhysicalVolume::Key(const G4VPhysicalVolume* const top, const char* digestextra, const char* digestextra2 )
{
    std::string digest = Digest(top, digestextra, digestextra2 );
    const char* exename = SLOG::instance ? SLOG::instance->args.exename() : "OpticksEmbedded" ; 
    std::stringstream ss ; 
    ss 
       << exename
       << "."
       << "X4PhysicalVolume"
       << "."
       << top->GetName()
       << "."
       << digest 
       ;
       
    std::string key = ss.str();
    return strdup(key.c_str());
}   


/**
X4PhysicalVolume::findSurface
------------------------------

1. look for a border surface from PV_a to PV_b (do not look for the opposite direction)
2. if no border surface look for a logical skin surface with the lv of the first PV_a otherwise the lv of PV_b 
   (or vv when first_priority is false) 

**/

G4LogicalSurface* X4PhysicalVolume::findSurface( const G4VPhysicalVolume* const a, const G4VPhysicalVolume* const b, bool first_skin_priority ) const 
{
     G4LogicalSurface* surf = G4LogicalBorderSurface::GetSurface(a, b) ;

     const G4VPhysicalVolume* const first  = first_skin_priority ? a : b ; 
     const G4VPhysicalVolume* const second = first_skin_priority ? b : a ; 

     if(surf == NULL)
         surf = G4LogicalSkinSurface::GetSurface(first ? first->GetLogicalVolume() : NULL );

     if(surf == NULL)
         surf = G4LogicalSkinSurface::GetSurface(second ? second->GetLogicalVolume() : NULL );

     return surf ; 
}


//#define X4_DEBUG_OPTICAL 1 


GPropertyMap<double>* X4PhysicalVolume::findSurfaceOK(const G4VPhysicalVolume* const a, const G4VPhysicalVolume* const b, bool first_skin_priority ) const 
{
     GPropertyMap<double>* surf = nullptr ; 

     GBorderSurface* bs = findBorderSurfaceOK( a, b ); 
     surf = dynamic_cast<GPropertyMap<double>*>(bs); 

     const G4VPhysicalVolume* const first  = first_skin_priority ? a : b ; 
     const G4VPhysicalVolume* const second = first_skin_priority ? b : a ; 

     if(surf == nullptr)
     {
         G4LogicalVolume* first_lv = first ? first->GetLogicalVolume() : nullptr ; 
         GSkinSurface* sk = findSkinSurfaceOK( first_lv );
         surf = dynamic_cast<GPropertyMap<double>*>(sk); 

#ifdef X4_DEBUG_OPTICAL
         G4VSolid* first_so = first_lv ? first_lv->GetSolid() : nullptr ; 
         G4String first_so_name = first_so ? first_so->GetName() : "-" ; 
         if(strcmp(first_so_name.c_str(), "sStrutBallhead") == 0 )
         {
             std::cout 
                 << "X4PhysicalVolume::findSurfaceOK"
                 << " first_so_name " << first_so_name
                 << " surf " << ( surf ? "YES" : "NO " )
                 << std::endl 
                 ;   
         }
#endif

     }

     if(surf == nullptr)
     {
         G4LogicalVolume* second_lv = second ? second->GetLogicalVolume() : nullptr ; 
         GSkinSurface* sk = findSkinSurfaceOK( second_lv );
         surf = dynamic_cast<GPropertyMap<double>*>(sk); 

#ifdef X4_DEBUG_OPTICAL
         G4VSolid* second_so = second_lv ? second_lv->GetSolid() : nullptr ; 
         G4String second_so_name = second_so ? second_so->GetName() : "-"  ; 
         if(strcmp(second_so_name.c_str(), "sStrutBallhead") == 0 )
         {
             std::cout 
                 << "X4PhysicalVolume::findSurfaceOK"
                 << " second_so_name " << second_so_name
                 << " surf " << ( surf ? "YES" : "NO " )
                 << std::endl 
                 ;   
         }
#endif

     }


     return surf ; 
}


GBorderSurface* X4PhysicalVolume::findBorderSurfaceOK( const G4VPhysicalVolume* const a, const G4VPhysicalVolume* const b) const 
{
    const char* pv1 = a ? X4::Name( a ) : nullptr  ; 
    const char* pv2 = b ? X4::Name( b ) : nullptr ; 
    GBorderSurface* bs =  nullptr ; 
    if( pv1 != nullptr && pv2 != nullptr)
    {
        bs = m_slib->findBorderSurface(pv1, pv2) ;  

        bool dump = false ; 
        if(dump)
        {
            LOG(LEVEL) 
                << " pv1 " << std::setw(40) << pv1 
                << " pv2 " << std::setw(40) << pv2
                << " bs " << bs 
                ; 
        }

    }
    return bs ; 
}

/**
X4PhysicalVolume::findSkinSurfaceOK
--------------------------------------

Changed to using GSurfaceLib::findSkinSurfaceLV, NOT: GSurfaceLib::findSkinSurface,
to avoid notes/issues/old_workflow_finds_extra_lSteel_skin_surface.rst

**/

GSkinSurface* X4PhysicalVolume::findSkinSurfaceOK( const G4LogicalVolume* const lv) const 
{
    /* 
    const char* _lv = X4::Name( lv ) ; 
    GSkinSurface* sk = _lv ? m_slib->findSkinSurface(_lv) : nullptr ;  
    return sk ; 
    */

    GSkinSurface* sk = lv ? m_slib->findSkinSurfaceLV(lv) : nullptr ;  
    return sk ; 
}










/**
X4PhysicalVolume::convertSolids
-----------------------------------

Populates GGeo/GMeshLib

Uses postorder recursive traverse, ie the "visit" is in the 
tail after the recursive call, to match the traverse used 
by GDML, and hence giving the same "postorder" indices
for the solid lvIdx.

The entire volume tree is recursed, but only the 
first occurence of each LV solid gets converted 
(because they are all the same).
Done this way to have consistent lvIdx soIdx indexing with GDML ?

**/

void X4PhysicalVolume::convertSolids()
{
    OK_PROFILE("_X4PhysicalVolume::convertSolids");
    LOG(LEVEL) << "[" ; 

    const G4VPhysicalVolume* pv = m_top ; 
    int depth = 0 ;
    convertSolids_r(pv, depth);
    
    if(m_verbosity > 5) dumpLV();
    LOG(debug) << "]" ; 

    dumpTorusLV();
    dumpLV();

    LOG(LEVEL) << "]" ;
    OK_PROFILE("X4PhysicalVolume::convertSolids");
}

/**
X4PhysicalVolume::convertSolids_r
------------------------------------

G4VSolid is converted to GMesh with associated analytic NCSG 
and added to GGeo/GMeshLib.

If the conversion from G4VSolid to GMesh/NCSG/nnode required
balancing of the nnode then the conversion is repeated 
without the balancing and an alt reference is to the alternative 
GMesh/NCSG/nnode is kept in the primary GMesh. 

Note that only the nnode is different due to the balancing, however
its simpler to keep a one-to-one relationship between these three instances
for persistency convenience.

Note that convertSolid is called for newly encountered lv
in the postorder tail after the recursive call in order for soIdx/lvIdx
to match Geant4. 

**/

void X4PhysicalVolume::convertSolids_r(const G4VPhysicalVolume* const pv, int depth)
{
    const G4LogicalVolume* lv = pv->GetLogicalVolume() ;

    // G4LogicalVolume::GetNoDaughters returns 1042:G4int, 1062:size_t
    for (size_t i=0 ; i < size_t(lv->GetNoDaughters()) ;i++ )
    {
        const G4VPhysicalVolume* const daughter_pv = lv->GetDaughter(i);
        convertSolids_r( daughter_pv , depth + 1 );
    }

    // for newly encountered lv record the tail/postorder idx for the lv
    if(std::find(m_lvlist.begin(), m_lvlist.end(), lv) == m_lvlist.end())
    {
        convertSolid( lv );  
    }  
}

/**
X4PhysicalVolume::convertSolid
-------------------------------

Hmm : how to do early x4skipsolid ?

* proceed normally adding to GGeo but set skip flag on the GMesh for checking later in convertStructure, 
  that gets propagated to the GNode tree : that might be workable as GInstancer already partitions the 
  GNodes according to ridx so should be easy to do the actual skip at that point 
  
**/

void X4PhysicalVolume::convertSolid( const G4LogicalVolume* lv )
{
    const G4VSolid* const solid = lv->GetSolid(); 

    G4String  lvname_ = lv->GetName() ;      // returns by reference, but take a copied value 
    G4String  soname_ = solid->GetName() ;   // returns by value, not reference

    const char* lvname = strdup(lvname_.c_str());  // may need these names beyond this scope, so strdup     
    const char* soname = strdup(soname_.c_str()); 

    bool x4skipsolid = m_ok->isX4SkipSolidName(soname); 

    int lvIdx = m_lvlist.size();  
    int soIdx = lvIdx ; // when converting in postorder soIdx is the same as lvIdx
    m_lvidx[lv] = lvIdx ;  
    m_lvlist.push_back(lv);  

    LOG(LEVEL) 
        << "[ " << std::setw(4) << lvIdx
        << " lvname " << lvname 
        << " soname " << soname 
        << " [--x4skipsolidname] " << ( x4skipsolid ? "YES" : "n" )
        ;


    GMesh* mesh = ConvertSolid(m_ok, lvIdx, soIdx, solid, soname, lvname ); 
    mesh->setX4SkipSolid(x4skipsolid); 

    m_lvname.push_back( lvname ); 
    m_soname.push_back( soname ); 

    const nnode* root = mesh->getRoot(); 
    if( root->has_torus() )
    {
        LOG(fatal) << " has_torus lvIdx " << lvIdx << " " << lvname ;  
        m_lv_with_torus.push_back( lvIdx ); 
        m_lvname_with_torus.push_back( lvname ); 
        m_soname_with_torus.push_back( soname ); 
    }

    m_ggeo->add( mesh ) ; 

    LOG(LEVEL) << "] " << std::setw(4) << lvIdx ; 
}




/**
X4PhysicalVolume::ConvertSolid
--------------------------------

Converts G4VSolid into two things:

1. analytic CSG nnode tree, boolean solids or polycones convert to trees of multiple nodes,
   deep trees are balanced to reduce their height
2. triangulated vertices and faces held in GMesh instance

As YOG doesnt depend on GGeo, and as workaround for GMesh/GBuffer deficiencies 
the source NPY arrays are also tacked on to the Mh instance.


--x4polyskip 211,232
~~~~~~~~~~~~~~~~~~~~~~

For DYB Near geometry two depth 12 CSG trees needed to be 
skipped as the G4 polygonization goes into an infinite (or at least 
beyond my patience) loop.::

     so:029 lv:211 rmx:12 bmx:04 soName: near_pool_iws_box0xc288ce8
     so:027 lv:232 rmx:12 bmx:04 soName: near_pool_ows_box0xbf8c8a8

Skipping results in placeholder bounding box meshes being
used instead of he real shape. 

**/

GMesh* X4PhysicalVolume::ConvertSolid( const Opticks* ok, int lvIdx, int soIdx, const G4VSolid* const solid, const char* soname, const char* lvname ) // static
{
    bool balance_deep_tree = true ;  
    GMesh* mesh = ConvertSolid_( ok, lvIdx, soIdx, solid, soname, lvname, balance_deep_tree ) ;  

    mesh->setIndex( lvIdx ) ;   

    // raw unbalanced tree height  
    const nnode* root = mesh->getRoot(); 
    assert( root ); 

    LOG(LEVEL)
        << " lvIdx " << lvIdx
        << " root.brief " << root->brief()
        << " soname " << soname
        ;


    const nnode* unbalanced = root->other ? root->other : root  ; 
    assert( unbalanced ); 

    unsigned unbalanced_height = unbalanced->maxdepth() ;  
    bool can_export_unbalanced = unbalanced_height <= NCSG::MAX_EXPORT_HEIGHT ;  

    LOG(LEVEL) 
        << " lvIdx " << lvIdx   
        << " soIdx " << soIdx   
        << " unbalanced_height " << unbalanced_height
        << " NCSG::MAX_EXPORT_HEIGHT " << NCSG::MAX_EXPORT_HEIGHT
        << " can_export_unbalanced " << can_export_unbalanced
        ; 
 
    const NCSG* csg = mesh->getCSG(); 
    if( csg->is_balanced() )  // when balancing done, also convert without it 
    {
        if( can_export_unbalanced )
        {  
            balance_deep_tree = false ;  
            GMesh* rawmesh = ConvertSolid_( ok, lvIdx, soIdx, solid, soname, lvname, balance_deep_tree ) ;  
            rawmesh->setIndex( lvIdx ) ;   

            const NCSG* rawcsg = rawmesh->getCSG(); 
            assert( rawmesh->getIndex() == rawcsg->getIndex() ) ;   

            mesh->setAlt(rawmesh);  // <-- this association is preserved (and made symmetric) thru metadata by GMeshLib 
        } 
        else
        {
            LOG(error)
                << " Cannot export the unbalanced tree as raw height exceeds the maximum. " << std::endl 
                << " unbalanced_height " << unbalanced_height 
                << " NCSG::MAX_EXPORT_HEIGHT " << NCSG::MAX_EXPORT_HEIGHT
                ;     
        } 
    }
    return mesh ; 
}


/**
X4PhysicalVolume::ConvertSolid_
----------------------------------


**/

GMesh* X4PhysicalVolume::ConvertSolid_( const Opticks* ok, int lvIdx, int soIdx, const G4VSolid* const solid, const char* soname, const char* lvname, bool balance_deep_tree ) // static
{
    assert( lvIdx == soIdx );  
    bool dbglv = lvIdx == ok->getDbgLV() ; 

    bool is_x4balanceskip = ok->isX4BalanceSkip(lvIdx) ; 
    LOG_IF(fatal, is_x4balanceskip) << " is_x4balanceskip " << " soIdx " << soIdx  << " lvIdx " << lvIdx ;  

    bool is_x4nudgeskip = ok->isX4NudgeSkip(lvIdx) ; 
    LOG_IF(fatal, is_x4nudgeskip ) << " is_x4nudgeskip " << " soIdx " << soIdx  << " lvIdx " << lvIdx ;  

    bool is_x4pointskip = ok->isX4PointSkip(lvIdx) ; 
    LOG_IF(fatal, is_x4pointskip) << " is_x4pointskip " << " soIdx " << soIdx  << " lvIdx " << lvIdx ;  


    LOG(LEVEL)
        << "[ "  
        << lvIdx
        << ( dbglv ? " --dbglv " : "" ) 
        << " soname " << soname
        << " lvname " << lvname
        ;

    X4Solid::Banner( lvIdx, soIdx, lvname, soname ); 
 
    const char* boundary = nullptr ; 
    nnode* raw = X4Solid::Convert(solid, ok, boundary, lvIdx )  ; 
    raw->set_nudgeskip( is_x4nudgeskip );   
    raw->set_pointskip( is_x4pointskip );  
    raw->set_treeidx( lvIdx ); 

    // At first glance these settings might look too late to do anything, but that 
    // is not the case as for example the *nudgeskip* setting is acted upon by the NCSG::NCSG(nnode*) cto, 
    // which is invoked from NCSG::Adopt below which branches in NCSG::MakeNudger based on the setting.

    bool g4codegen = ok->isG4CodeGen() ; 

    if(g4codegen) GenerateTestG4Code(ok, lvIdx, solid, raw); 

    GMesh* mesh = ConvertSolid_FromRawNode( ok, lvIdx, soIdx, solid, soname, lvname, balance_deep_tree, raw ); 

    return mesh ; 
}

/**
X4PhysicalVolume::ConvertSolid_FromRawNode
--------------------------------------------

This was split from X4PhysicalVolume::ConvertSolid_ in order to enable geochain starting from nnode

**/

GMesh* X4PhysicalVolume::ConvertSolid_FromRawNode( const Opticks* ok, int lvIdx, int soIdx, const G4VSolid* const solid, const char* soname, const char* lvname, bool balance_deep_tree,
     nnode* raw) 
{
    bool is_x4balanceskip = ok->isX4BalanceSkip(lvIdx) ; 
    bool is_x4polyskip = ok->isX4PolySkip(lvIdx);   // --x4polyskip 211,232
    bool is_x4nudgeskip = ok->isX4NudgeSkip(lvIdx) ; 
    bool is_x4pointskip = ok->isX4PointSkip(lvIdx) ; 
    bool do_balance = balance_deep_tree && !is_x4balanceskip ; 

    nnode* root = do_balance ? NTreeProcess<nnode>::Process(raw, soIdx, lvIdx) : raw ;  

    LOG(LEVEL) << " after NTreeProcess:::Process " ; 

    root->other = raw ; 
    root->set_nudgeskip( is_x4nudgeskip ); 
    root->set_pointskip( is_x4pointskip );  
    root->set_treeidx( lvIdx ); 

    const NSceneConfig* config = NULL ; 

    LOG(LEVEL) << "[ before NCSG::Adopt " ; 
    NCSG* csg = NCSG::Adopt( root, config, soIdx, lvIdx );   // Adopt exports nnode tree to m_nodes buffer in NCSG instance
    LOG(LEVEL) << "] after NCSG::Adopt " ; 
    assert( csg ) ; 
    assert( csg->isUsedGlobally() );

    bool is_balanced = root != raw ; 
    if(is_balanced) assert( balance_deep_tree == true );  

    csg->set_balanced(is_balanced) ;  
    csg->set_soname( soname ) ; 
    csg->set_lvname( lvname ) ; 

    LOG_IF(fatal, is_x4polyskip ) << " is_x4polyskip " << " soIdx " << soIdx  << " lvIdx " << lvIdx ;  

    GMesh* mesh = nullptr ; 
    if(solid)
    {
        mesh =  is_x4polyskip ? X4Mesh::Placeholder(solid ) : X4Mesh::Convert(solid, lvIdx) ; 
    }
    else
    {
        mesh =  X4Mesh::Placeholder(raw) ; 
    } 

    mesh->setCSG( csg ); 

    LOG(LEVEL) << "] " << lvIdx ; 
    return mesh ; 
}


void X4PhysicalVolume::GenerateTestG4Code( const Opticks* ok, int lvIdx, const G4VSolid* const solid, const nnode* raw) // static 
{
    const char* g4codegendir = ok->getG4CodeGenDir(); 
    bool dbglv = lvIdx == ok->getDbgLV() ; 
    const char* gdmlpath = X4CSG::GenerateTestPath( g4codegendir, lvIdx, ".gdml" ) ;  
    bool refs = false ;  
    X4GDMLParser::Write( solid, gdmlpath, refs ); 
    if(dbglv)
    { 
        LOG(LEVEL) 
            << ( dbglv ? " --dbglv " : "" ) 
            << "[--g4codegen]"
            << " lvIdx " << lvIdx
            ;
        raw->dump_g4code();  // just for debug 
    }
    X4CSG::GenerateTest( solid, ok, g4codegendir , lvIdx ) ; 
}











void X4PhysicalVolume::dumpLV(unsigned edgeitems) const 
{
    LOG(LEVEL) << descLV(edgeitems); 
}

std::string X4PhysicalVolume::descLV(unsigned edgeitems) const 
{
    std::stringstream ss ; 
    ss
       << "X4PhysicalVolume::descLV"
       << " m_lvidx.size() " << m_lvidx.size() 
       << " m_lvlist.size() " << m_lvlist.size()
       << " edgeitems " << edgeitems 
       << std::endl 
       ;

   unsigned num_lv = m_lvlist.size() ;  

   assert( num_lv == m_lvname.size() );  
   assert( num_lv == m_soname.size() );  

   for(unsigned i=0 ; i < num_lv ; i++)
   {
       if( i < edgeitems || i > num_lv - edgeitems ) 
       {
           const G4LogicalVolume* lv = m_lvlist[i] ; 
           const std::string& lvn =  lv->GetName() ; 
           assert( strcmp(lvn.c_str(), m_lvname[i].c_str() ) == 0 ); 

           ss
               << " i " << std::setw(5) << i
               << " idx " << std::setw(5) << m_lvidx.at(lv)  
               << " lvname " << std::setw(50) << m_lvname[i]
               << " soname " << std::setw(50) << m_soname[i]
               << std::endl ;  
       }
   }
   std::string s = ss.str(); 
   return s; 
}

void X4PhysicalVolume::dumpTorusLV() const 
{
    assert( m_lv_with_torus.size() == m_lvname_with_torus.size() ); 
    assert( m_lv_with_torus.size() == m_soname_with_torus.size() ); 
    unsigned num_afflicted = m_lv_with_torus.size() ;  
    if(num_afflicted == 0) return ; 


    LOG(LEVEL) << " num_afflicted " << num_afflicted ; 
    std::cout << " lvIdx ( " ; 
    for(unsigned i=0 ; i < num_afflicted ; i++) std::cout << m_lv_with_torus[i] << " " ; 
    std::cout << " ) " << std::endl ;  

    for(unsigned i=0 ; i < num_afflicted ; i++) 
    {
        std::cout 
            << " lv "     << std::setw(4)  << m_lv_with_torus[i] 
            << " lvname " << std::setw(50) << m_lvname_with_torus[i] 
            << " soname " << std::setw(50) << m_soname_with_torus[i] 
            << std::endl 
            ; 
    }

}

std::string X4PhysicalVolume::brief() const 
{
    std::stringstream ss ; 
    ss
        << " selected_node_count " << m_selected_node_count
        << " node_count " << m_selected_node_count
        ;

    return ss.str(); 
}


void X4PhysicalVolume::convertCheck() const 
{
    bool no_nodes = m_selected_node_count == 0 || m_node_count == 0 ; 
    if(no_nodes) 
    {
        LOG(fatal)
            << " NO_NODES ERROR " 
            << brief()
            << std::endl
            << " query " 
            << m_query->desc()
            ;
        assert(0) ; 
    }
}



/**
convertStructure
--------------------

Preorder traverse conversion of the full tree of G4VPhysicalVolume 
into a tree of GVolume, the work mostly done in X4PhysicalVolume::convertNode.
GVolume instances are collected into GGeo/GNodeLib.

Old Notes
~~~~~~~~~~~~

Note that its the YOG model that is updated, that gets
converted to glTF later.  This is done to help keeping 
this code independant of the actual glTF implementation 
used.

* NB this is very similar to the ancient AssimpGGeo::convertStructure, GScene::createVolumeTree

**/


const char* X4PhysicalVolume::TMPDIR = "$TMP/extg4/X4PhysicalVolume" ; 

void X4PhysicalVolume::convertStructure()
{
    assert(m_top) ;
    LOG(LEVEL) << "[ creating large tree of GVolume instances" ; 

    m_tree = new stree ;   // HMM: m_tree is a spy from the future 
    m_ggeo->setTree(m_tree); 
    

    const G4VPhysicalVolume* pv = m_top ; 
    GVolume* parent = NULL ; 
    const G4VPhysicalVolume* parent_pv = NULL ; 
    int depth = 0 ;
    bool recursive_select = false ;


    OK_PROFILE("_X4PhysicalVolume::convertStructure");

    int parent_sibdex = -1 ; 
    int parent_nidx = -1 ; 

    m_root = convertStructure_r(pv, parent, depth, parent_sibdex, parent_nidx, parent_pv, recursive_select );  // set root GVolume 

    m_ggeo->setRootVolume(m_root); 

    OK_PROFILE("X4PhysicalVolume::convertStructure");

    convertStructureChecks(); 
}


void X4PhysicalVolume::convertStructureChecks() const 
{
    NTreeProcess<nnode>::SaveBuffer(TMPDIR, "NTreeProcess.npy");      
    NNodeNudger::SaveBuffer(TMPDIR, "NNodeNudger.npy"); 
    X4Transform3D::SaveBuffer(TMPDIR, "X4Transform3D.npy"); 


#ifdef X4_TRANSFORM
    LOG(LEVEL) 
        << " m_is_identity0 " << m_is_identity0
        << std::endl 
        << " m_is_identity1 " << m_is_identity1 
        ;
#endif


#ifdef X4_PROFILE    
    LOG(LEVEL) 
        << " m_convertNode_dt " << m_convertNode_dt 
        << std::endl 
        << " m_convertNode_boundary_dt " << m_convertNode_boundary_dt 
        << std::endl 
        << " m_convertNode_transformsA_dt " << m_convertNode_transformsA_dt 
        << std::endl 
        << " m_convertNode_transformsB_dt " << m_convertNode_transformsB_dt 
        << std::endl 
        << " m_convertNode_transformsC_dt " << m_convertNode_transformsC_dt 
        << std::endl 
        << " m_convertNode_transformsD_dt " << m_convertNode_transformsD_dt 
        << std::endl 
        << " m_convertNode_transformsE_dt " << m_convertNode_transformsE_dt 
        << std::endl 
        << " m_convertNode_GVolume_dt " << m_convertNode_GVolume_dt 
        ;
#endif
}



/**

X4PhysicalVolume::convertStructure_r
--------------------------------------

Preorder traverse, with recursive call after the visit, that 
gives access to parent GVolume and G4VPhysicalVolume.

Formerly collected volumes into GGeo/GNodeLib here, 
but thats too soon for tripletIdentity, so instead the GVolume 
are just held in the tree of GVolume until collected 
into GGeo/GNodeLib by a traversal by GInstancer. 

G4LogicalVolume::GetNoDaughters return type change 1042:G4int, 1062:size_t


**/

GVolume* X4PhysicalVolume::convertStructure_r(const G4VPhysicalVolume* const pv, 
        GVolume* parent, int depth, int sibdex, int parent_nidx, 
        const G4VPhysicalVolume* const parent_pv, bool& recursive_select )
{
#ifdef X4_PROFILE
     float t0 = BTimeStamp::RealTime(); 
#endif

     GVolume* volume = convertNode(pv, parent, depth, parent_pv, recursive_select );

#ifdef X4_PROFILE
     float t1 = BTimeStamp::RealTime() ; 
     m_convertNode_dt += t1 - t0 ; 
#endif
     const G4LogicalVolume* const lv = pv->GetLogicalVolume();
     int num_child = int(lv->GetNoDaughters());  

     // follow stree aproach from U4Tree::convertNodes_r
     int nidx = m_tree->nds.size() ;  // 0-based node index
     volume->set_nidx(nidx); 

     int copyno = volume->getCopyNumber(); 
     int lvid = volume->get_lvidx() ; 



     glm::tmat4x4<double> tr_m2w(1.) ;   
     GMatrixF* ltransform = volume->getLevelTransform(); 
     float* ltr = (float*)ltransform->getPointer() ; // CAUTION: THIS IS NOT MEMORY ORDER OF GMatrix 
     strid::Read(tr_m2w, ltr, false );   // transpose:false the getPointer does a transpose


     glm::tmat4x4<double> tr_gtd(1.) ;   // GGeo Transform Debug   
     GMatrixF* transform = volume->getTransform(); 
     float* tr = (float*)transform->getPointer() ;
     strid::Read(tr_gtd, tr, false );   // transpose:false the getPointer does a transpose





     snode nd ;
     nd.index = nidx ;
     nd.depth = depth ;   
     nd.sibdex = sibdex ; 
     nd.parent = parent_nidx ;   

     nd.num_child = num_child ; 
     nd.first_child = -1 ;     // gets changed inplace from lower recursion level 
     nd.next_sibling = -1 ; 
     nd.lvid = lvid ; 
     nd.copyno = copyno ; 

     nd.sensor_id = -1 ;    
     nd.sensor_index = -1 ; 
    
     m_tree->nds.push_back(nd); 
     m_tree->m2w.push_back(tr_m2w);
     m_tree->gtd.push_back(tr_gtd);
      
     if(sibdex == 0 && nd.parent > -1) m_tree->nds[nd.parent].first_child = nd.index ;
     // record first_child nidx into parent snode

     int p_sib = -1 ;
     int i_sib = -1 ;
  
     for (int i=0 ; i < num_child ;i++ )
     {
         p_sib = i_sib ;  // node index of previous child 

         const G4VPhysicalVolume* const child_pv = lv->GetDaughter(i);
         GVolume* child = convertStructure_r(child_pv, volume, depth+1, i, nidx, pv, recursive_select );

         int i_sib = child->get_nidx();  
         if(p_sib > -1) m_tree->nds[p_sib].next_sibling = i_sib ;   // sib->sib linkage 
     }

     return volume   ; 
}

/**
X4PhysicalVolume::addBoundary
------------------------------

Canonically invoked from X4PhysicalVolume::convertNode during the 
main structural traverse.

For a physical volume and its parent physical volume 
adds(if not already present) a boundary to the GBndLib m_blib instance, 
and returns the index of the newly created or pre-existing boundary.
A boundary is a quadruplet of omat/osur/isur/imat indices.

See notes/issues/ab-blib.rst on getting A-B comparisons to match for boundaries.

OLD_ADD_BOUNDARY 
   skin surface consults the GGeo model but border surface hark back to the Geant4 model ?
   In the new variant, remove this legacy consulting GGeo model for both  


Notice that the non-directional skin surface are translated by the osur and isur being the same in the 2nd and 3rd 
elements of the boundary quad.
 
**/

bool X4PhysicalVolume::IsDebugBoundary( const char* omat, const char* osur, const char* isur, const char* imat )
{
    return omat && osur && isur && imat 
        && 
        strcmp(omat, "Water") == 0  
        &&   
        strcmp(osur, "StrutAcrylicOpSurface" ) == 0 
        &&   
        strcmp(isur, "StrutAcrylicOpSurface" ) == 0  
        &&   
        strcmp(imat, "Steel") == 0  
        ;    
}

unsigned X4PhysicalVolume::addBoundary(const G4VPhysicalVolume* const pv, const G4VPhysicalVolume* const pv_p )
{
    const G4LogicalVolume* const lv   = pv->GetLogicalVolume() ;
    const G4LogicalVolume* const lv_p = pv_p ? pv_p->GetLogicalVolume() : NULL ;



    const G4VSolid* const so = lv->GetSolid() ; 
    const G4VSolid* const so_p = lv_p ? lv_p->GetSolid() : nullptr ; 
    
    G4String so_name = so->GetName() ; 
    G4String so_p_name = so_p ? so_p->GetName() : "-" ; 
    const char* _so_name = so_name.c_str(); 
    const char* _so_p_name = so_p_name.c_str(); 



    // GDMLName adds pointer suffix to the object name, returns null when object is null : eg parent of world 

    const char* _pv = X4::GDMLName(pv) ;   
    const char* _pv_p = X4::GDMLName(pv_p) ; 


    const G4Material* const imat_ = lv->GetMaterial() ;
    const G4Material* const omat_ = lv_p ? lv_p->GetMaterial() : imat_ ;  // top omat -> imat 

    // input material names are not of form "/dd/Materials/Air" but rather "_dd_Materials_Air"
    const char* omat = X4::BaseName(omat_) ; 
    const char* imat = X4::BaseName(imat_) ; 

    LOG(debug)
        << " imat_.GetName " << std::setw(50) << imat_->GetName()
        << " omat_.GetName " << std::setw(50) << omat_->GetName()
        << " omat " << std::setw(50) << omat
        << " imat " << std::setw(50) << imat
        ;

#ifdef OLD_ADD_BOUNDARY
    assert(0) ; // NOT EXPECTING OLD_ADD_BOUNDARY to be used 
    static const char* IMPLICIT_PREFIX = "Implicit_RINDEX_NoRINDEX" ; 
    // look for a border surface defined between this and the parent volume, in either direction
    bool first_skin_priority = true ;   // controls fallback skin lv order when bordersurface a->b not found 
    const G4LogicalSurface* const isur_ = findSurface( pv  , pv_p , first_skin_priority );
    const G4LogicalSurface* const osur_ = findSurface( pv_p, pv   , first_skin_priority );  
    // doubtful of findSurface priority with double skin surfaces, see g4op-

    const GPropertyMap<double>* const isur2_ = findSurfaceOK(  pv  , pv_p, first_skin_priority ); 
    const GPropertyMap<double>* const osur2_ = findSurfaceOK(  pv_p, pv  , first_skin_priority ); 

    if( isur2_ != nullptr && isur_ == nullptr )  // find from OK but not G4  : only implicits should do this 
    {
        const char* isur2_name = isur2_->getName(); 
        LOG(LEVEL) << " isur2_name " << isur2_name ; 
        assert( SStr::StartsWith(isur2_name,  IMPLICIT_PREFIX )); 
    }
    if( osur2_ != nullptr && osur_ == nullptr )  // find from OK but not G4 : only implicits should do this 
    {
        const char* osur2_name = osur2_->getName(); 
        LOG(LEVEL) << " osur2_name " << osur2_name ; 
        assert( SStr::StartsWith(osur2_name,  IMPLICIT_PREFIX )); 
    }


    if( isur2_ == nullptr && isur_ != nullptr )  // find from G4 but not from OK : should not happen 
    {
        LOG(fatal) << " isur_ : find from G4 but not from OK " ; 
        assert(0); 
    }
    if( osur2_ == nullptr && osur_ != nullptr )  // find from G4 but not from OK : should not happen 
    {
        LOG(fatal) << " osur_ : find from G4 but not from OK " ; 
        assert(0); 
    }


    if( isur_ != nullptr && isur2_ != nullptr )
    {
        const G4String& isur_name = isur_->GetName() ; 
        const char* isur2_name = isur2_->getName(); 
        //LOG(LEVEL) << " isur2_name " << isur2_name ; 
        assert( strcmp( isur_name.c_str(), isur2_name ) == 0 ); 
    }

    if( osur_ != nullptr && osur2_ != nullptr )
    {
        const G4String& osur_name = osur_->GetName() ; 
        const char* osur2_name = osur2_->getName(); 
        //LOG(LEVEL) << " osur2_name " << osur2_name ; 
        assert( strcmp( osur_name.c_str(), osur2_name ) == 0 ); 
    }

#else
    // look for a border surface defined between this and the parent volume, in either direction
    bool first_skin_priority = true ;   // controls fallback skin lv order when bordersurface a->b not found 
    const GPropertyMap<double>* const isur_ = findSurfaceOK(  pv  , pv_p, first_skin_priority ); 
    const GPropertyMap<double>* const osur_ = findSurfaceOK(  pv_p, pv  , first_skin_priority ); 
#endif

    const char* _lv = X4::GDMLName(lv) ;    
    const char* _lv_p = X4::GDMLName(lv_p) ; 

    bool ps = SStr::HasPointerSuffix(_lv, 6, 12) ;  // 9,12 on macOS 
    LOG_IF(fatal, !ps) << " unexpected pointer suffix _lv " << _lv ;  
    assert(ps) ;    

    if( _lv_p )
    {
        bool ps = SStr::HasPointerSuffix(_lv_p, 6, 12); 
        LOG_IF(fatal, !ps) << " unexpected pointer suffix _lv " << _lv_p ;  
        assert(ps) ;    
    }

    LOG(debug)
        << " lv " << lv 
        << " _lv " << _lv
        << " lv_p " << lv_p 
        << " _lv_p " << _lv_p
        ;

    LOG(debug)
        << " pv " << pv 
        << " _pv " << _pv
        << " pv_p " << pv_p 
        << " _pv_p " << _pv_p
        ;


    const GSkinSurface* g_sslv = m_ggeo->findSkinSurface(_lv) ;  
    const GSkinSurface* g_sslv_p = _lv_p ? m_ggeo->findSkinSurface(_lv_p) : NULL ;  

    LOG_IF(debug, g_sslv_p) 
        << " node_count " << m_node_count 
        << " _lv_p   " << _lv_p
        << " g_sslv_p " << g_sslv_p->getName()
        ; 



    LOG(debug) 
         << " addBoundary "
         << " omat " << omat 
         << " imat " << imat 
         ;



     // CURIOUS : WHY CHECK FOR NON-EXISTANCE OF SKIN SURFACE, BEFORE TRYING BORDER SURFACE ?

    unsigned boundary = 0 ; 
    if( g_sslv == NULL && g_sslv_p == NULL  )   // no skin surface on this or parent volume, just use bordersurface if there are any
    {

#ifdef OLD_ADD_BOUNDARY
        const char* osur = X4::BaseName( osur_ ); 
        const char* isur = X4::BaseName( isur_ ); 
#else
        const char* osur = osur_ ? osur_->getName() : nullptr ; 
        const char* isur = isur_ ? isur_->getName() : nullptr ; 
#endif

        bool match = IsDebugBoundary( omat, osur, isur, imat ); 
        if(match) 
        {
            std::cout 
                << "X4PhysicalVolume::addBoundary"
                << " IsDebugBoundary "
                << " omat " << omat 
                << " osur " << osur
                << " isur " << isur
                << " imat " << imat
                << std::endl 
                ;
         
            std::cout
                << "X4PhysicalVolume::addBoundary" << std::endl 
                << " _pv        " << _pv  << std::endl 
                << " _pv_p      " << _pv_p  << std::endl
                << " _lv        " << _lv  << std::endl 
                << " _lv_p      " << _lv_p  << std::endl 
                << " _so_name   " << _so_name << std::endl 
                << " _so_p_name " << _so_p_name << std::endl 
                ;

            //std::raise(SIGINT) ;
        }

        boundary = m_blib->addBoundary( omat, osur, isur, imat ); 
    }
    else if( g_sslv && !g_sslv_p )   // skin surface on this volume but not parent : set both osur and isur to this 
    {
        const char* osur = g_sslv->getName(); 
        const char* isur = osur ; 
        boundary = m_blib->addBoundary( omat, osur, isur, imat ); 
    }
    else if( g_sslv_p && !g_sslv )  // skin surface on parent volume but not this : set both osur and isur to this
    {
        const char* osur = g_sslv_p->getName(); 
        const char* isur = osur ; 
        boundary = m_blib->addBoundary( omat, osur, isur, imat ); 
    } 
    else if( g_sslv_p && g_sslv )
    {
        assert( 0 && "fabled double skin found : see notes/issues/ab-blib.rst  " ); 
    }

    return boundary ; 
}



/**
X4PhysicalVolume::convertNode
--------------------------------

* creates pt(GPt) minimal struct holding indices associating the node(ndIdx) with its shape(lvIdx, csgIdx) and boundaryName   
  (although GPt can hold placement transforms that is not set here) 

* pt(GPt) are associated to the GVolume returned for structural node

* suspect the parallel tree is for gltf creation only ?

* convertNode is hot code : should move whatever possible elsewhere 


node/pv level vs shape/lv level
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are many more nodes/pv(eg 300,000 for JUNO) than there are mesh/lv/shapes (eg ~200 for JUNO)  
so it is beneficial to minimize what is done at node level. Whatever can be 
done at mesh/lv/shape level should be done there.

* boundary and boundaryName are node level things as they require the node pv and parent pv to form them
* GParts::Make is technically a node level thing because it needs the boundaryName but want 
  to avoid doing this rather expensive processing here 

* solution is light weight GPt which collects the arguments needed for GParts::Make allowing 
  deferred postcache GParts::Create from persisted GPts


sensor decision
~~~~~~~~~~~~~~~~~

* sensor indices are assigned to each GVolume based on ``GBndLib::isSensorBoundary(boundary)``



**/


GVolume* X4PhysicalVolume::convertNode(const G4VPhysicalVolume* const pv, GVolume* parent, int depth, const G4VPhysicalVolume* const pv_p, bool& recursive_select )
{
#ifdef X4_PROFILE
    float t00 = BTimeStamp::RealTime(); 
#endif
   
    // record copynumber in GVolume, as thats one way to handle pmtid
    const G4PVPlacement* placement = dynamic_cast<const G4PVPlacement*>(pv); 
    assert(placement); 
    G4int copyNumber = placement->GetCopyNo() ;  

    X4Nd* parent_nd = parent ? static_cast<X4Nd*>(parent->getParallelNode()) : NULL ;

    unsigned boundary = addBoundary( pv, pv_p );
    std::string boundaryName = m_blib->shortname(boundary); 
    int materialIdx = m_blib->getInnerMaterial(boundary); 


    const G4LogicalVolume* const lv   = pv->GetLogicalVolume() ;
    const std::string& lvName = lv->GetName() ; 
    const std::string& pvName = pv->GetName() ; 
    unsigned ndIdx = m_node_count ;       // incremented below after GVolume instanciation

    int lvIdx = m_lvidx[lv] ;   // from postorder traverse in convertSolids to match GDML lvIdx : mesh identity uses lvIdx

    LOG(debug) 
        << " copyNumber " << std::setw(8) << copyNumber
        << " boundary " << std::setw(4) << boundary 
        << " materialIdx " << std::setw(4) << materialIdx
        << " boundaryName " << boundaryName
        << " lvIdx " << lvIdx
        ;

    // THIS IS HOT NODE CODE : ~300,000 TIMES FOR JUNO 


    const GMesh* mesh = m_hlib->getMeshWithIndex(lvIdx);   // GMeshLib 

    const NCSG* csg = mesh->getCSG();  
    unsigned csgIdx = csg->getIndex() ; 



     ///////////////////////////////////////////////////////////////  

#ifdef X4_PROFILE
    float t10 = BTimeStamp::RealTime(); 
#endif

    GPt* pt = new GPt( lvIdx, ndIdx, csgIdx, boundaryName.c_str() )  ;  

    //
    // Q: where is the GPt placement transform set ?
    // A: by GMergedMesh::mergeVolume/GMergedMesh::mergeVolumeAnalytic 
    //    using a base relative or global transform depending on ridx
    //
    // WHY: because before analysis and resulting "factorization" 
    //      of the geometry cannot know the appropriate placement transform to assign to he GPt
    //       
    // Local and global transform triples are collected below into GVolume with::
    //       
    //     GVolume::setLocalTransform(ltriple)
    //     GVolume::setGlobalTransform(gtriple)
    //
    //  Those are the ingredients that later are used to get the appropriate 
    //  combination of transforms.


    glm::mat4 xf_local_t = X4Transform3D::GetObjectTransform(pv);  

#ifdef X4_TRANSFORM
    // check the Object and Frame transforms are inverses of each other
    glm::mat4 xf_local_v = X4Transform3D::GetFrameTransform(pv);  
    glm::mat4 id0 = xf_local_t * xf_local_v ; 
    glm::mat4 id1 = xf_local_v * xf_local_t ; 
    bool is_identity0 = nglmext::is_identity(id0)  ; 
    bool is_identity1 = nglmext::is_identity(id1)  ; 

    m_is_identity0 += ( is_identity0 ? 1 : 0 ); 
    m_is_identity1 += ( is_identity1 ? 1 : 0 ); 

    if(ndIdx < 10 || !(is_identity0 && is_identity1))
    {
        LOG(LEVEL) 
            << " ndIdx  " << ndIdx 
            << " is_identity0 " << is_identity0
            << " is_identity1 " << is_identity1
            << std::endl
            << " id0 " << gformat(id0)
            << std::endl
            << " id1 " << gformat(id1)
            << std::endl
            << " xf_local_t " << gformat(xf_local_t)
            << std::endl
            << " xf_local_v " << gformat(xf_local_v)
            ;       
    }
    //assert( is_identity ) ;  
#endif


#ifdef X4_PROFILE
    float t12 = BTimeStamp::RealTime(); 
#endif

    const nmat4triple* ltriple = m_xform->make_triple( glm::value_ptr(xf_local_t) ) ;   // YIKES does polardecomposition + inversion and checks them 
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifdef X4_PROFILE
    float t13 = BTimeStamp::RealTime(); 
#endif

    GMatrixF* ltransform = new GMatrix<float>(glm::value_ptr(xf_local_t));

#ifdef X4_PROFILE
    float t15 = BTimeStamp::RealTime(); 
#endif

    X4Nd* nd = new X4Nd { parent_nd, ltriple } ;         // X4Nd just struct { parent, transform }

    const nmat4triple* gtriple = nxform<X4Nd>::make_global_transform(nd) ;  // product of transforms up the tree
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifdef X4_PROFILE
    float t17 = BTimeStamp::RealTime(); 
#endif

    glm::mat4 xf_global = gtriple->t ;

    GMatrixF* gtransform = new GMatrix<float>(glm::value_ptr(xf_global));

#ifdef X4_PROFILE
    float t20 = BTimeStamp::RealTime(); 

    m_convertNode_boundary_dt    += t10 - t00 ; 

    m_convertNode_transformsA_dt += t12 - t10 ; 
    m_convertNode_transformsB_dt += t13 - t12 ; 
    m_convertNode_transformsC_dt += t15 - t13 ; 
    m_convertNode_transformsD_dt += t17 - t15 ; 
    m_convertNode_transformsE_dt += t20 - t17 ; 
#endif

/*
     m_convertNode_boundary_dt 3.47852
     m_convertNode_transformsA_dt 0.644531
     m_convertNode_transformsB_dt 6.35547
     m_convertNode_transformsC_dt 0.277344
     m_convertNode_transformsD_dt 7.29492
     m_convertNode_transformsE_dt 0.230469
     m_convertNode_GVolume_dt 3
*/

    ////////////////////////////////////////////////////////////////

    G4PVPlacement* _placement = const_cast<G4PVPlacement*>(placement) ;  
    void* origin_node = static_cast<void*>(_placement) ; 
    int origin_copyNumber = copyNumber ; 


    GVolume* volume = new GVolume(ndIdx, gtransform, mesh, origin_node, origin_copyNumber );
    volume->setBoundary( boundary );   // must setBoundary before adding sensor volume 
    volume->setCopyNumber(copyNumber);  // NB within instances this is changed by GInstancer::labelRepeats_r when m_duplicate_outernode_copynumber is true
    volume->set_lvidx( lvIdx ); 


    LOG(debug) << " instanciate GVolume node_count  " << m_node_count ; 


    m_node_count += 1 ; 

    unsigned lvr_lvIdx = lvIdx ; 
    bool selected = m_query->selected(pvName.c_str(), ndIdx, depth, recursive_select, lvr_lvIdx );
    if(selected) m_selected_node_count += 1 ;  

    LOG(verbose) << " lv_lvIdx " << lvr_lvIdx
                 << " selected " << selected
                 ; 

    ///////// sensor decision for the volume happens here  ////////////////////////
    //////// TODO: encapsulate into a GBndLib::formSensorIndex ? 
    ////// HMM : SOMEWHAT OBTUSE GETTING SOMETHING FROM BOUNDARY WITHIN THE NODE VISIT 

    bool is_sensor = m_blib->isSensorBoundary(boundary) ; // this means that isurf/osurf has non-zero EFFICIENCY property 
    unsigned sensorIndex = GVolume::SENSOR_UNSET ; 
    if(is_sensor)
    {
        sensorIndex = 1 + m_blib->getSensorCount() ;  // 1-based index
        m_blib->countSensorBoundary(boundary); 
    }
    volume->setSensorIndex(sensorIndex);   // must set to GVolume::SENSOR_UNSET for non-sensors, for sensor_indices array  

    ///////////////////////////////////////////////////////////////////////////

    volume->setSelected( selected );
    volume->setLevelTransform(ltransform);
    volume->setLocalTransform(ltriple);
    volume->setGlobalTransform(gtriple);
 
    volume->setParallelNode( nd ); 

    volume->setPt( pt ); 
    volume->setPVName( pvName.c_str() );
    volume->setLVName( lvName.c_str() );
    volume->setName( pvName.c_str() );   // historically (AssimpGGeo) this was set to lvName, but pvName makes more sense for node node

    m_ggeo->countMeshUsage(lvIdx, ndIdx );

    if(parent) 
    {
         parent->addChild(volume);
         volume->setParent(parent);
    } 

#ifdef X4_PROFILE
    float t30 = BTimeStamp::RealTime() ;
    m_convertNode_GVolume_dt     += t30 - t20 ; 
#endif
    return volume ; 
}

/**
X4PhysicalVolume::MakePlaceholderNode
---------------------------------------

Currently use of this is limited to very simple single solid geometries. 

**/

GVolume* X4PhysicalVolume::MakePlaceholderNode() // static
{
    int lvIdx = 0 ; 
    unsigned ndIdx = 0 ; 
    unsigned csgIdx = 0 ; 
    const char* boundarySpec = nullptr ; 
    const char* pvName = "PlaceholderPV" ; 
    const char* lvName = "PlaceholderLV" ; 


    GPt* pt = new GPt( lvIdx, ndIdx, csgIdx, boundarySpec )  ;  

    void* origin_node = nullptr ; 
    int origin_copyNumber = 0 ;  

    glm::mat4 xf_local(1.f); 
    glm::mat4 xf_global(1.f); 
    GMatrixF* ltransform = new GMatrix<float>(glm::value_ptr(xf_local));
    GMatrixF* gtransform = new GMatrix<float>(glm::value_ptr(xf_global));

    const GGeo* ggeo = GGeo::Get(); 
    const GMesh* mesh = ggeo->getMesh(lvIdx);   // GMeshLib 

    unsigned boundary = 0 ; 
    unsigned sensorIndex = GVolume::SENSOR_UNSET ; 
    GVolume* volume = new GVolume(ndIdx, gtransform, mesh, origin_node, origin_copyNumber );

    // hmm which GVolume properties are essential ?

    volume->setBoundary(boundary); 
    volume->setSensorIndex(sensorIndex);   // must set to GVolume::SENSOR_UNSET for non-sensors, for sensor_indices array  
    volume->setLevelTransform(ltransform); 
    volume->setPt(pt); 

    volume->setPVName( pvName );
    volume->setLVName( lvName );
 
    return volume ; 
}


void X4PhysicalVolume::DumpSensorVolumes(const GGeo* gg, const char* msg)
{
    unsigned numSensorVolumes = gg->getNumSensorVolumes();
    LOG(LEVEL) 
         << msg
         << " numSensorVolumes " << numSensorVolumes 
         ; 


    unsigned lastTransitionIndex = -2 ; 
    unsigned lastOuterCopyNo = -2 ; 

    for(unsigned i=0 ; i < numSensorVolumes ; i++)
    {   
        unsigned sensorIndex = 1+i ;  // 1-based 

        const GVolume* sensor = gg->getSensorVolume(sensorIndex) ; 
        assert(sensor); 
        const void* const sensorOrigin = sensor->getOriginNode(); 
        assert(sensorOrigin);
        const G4PVPlacement* const sensorPlacement = static_cast<const G4PVPlacement* const>(sensorOrigin);
        assert(sensorPlacement);  
        unsigned sensorCopyNo = sensorPlacement->GetCopyNo() ;  


        const GVolume* outer = sensor->getOuterVolume() ; 
        assert(outer); 
        const void* const outerOrigin = outer->getOriginNode(); 
        assert(outerOrigin); 
        const G4PVPlacement* const outerPlacement = static_cast<const G4PVPlacement* const>(outerOrigin);
        assert(outerPlacement);
        unsigned outerCopyNo = outerPlacement->GetCopyNo() ;  

        if(outerCopyNo != lastOuterCopyNo + 1) lastTransitionIndex = i ; 
        lastOuterCopyNo = outerCopyNo ; 

        if(i - lastTransitionIndex < 10)
        std::cout 
             << " sensorIndex " << std::setw(6) << sensorIndex 
             << " sensorPlacement " << std::setw(8) << sensorPlacement
             << " sensorCopyNo " << std::setw(8) << sensorCopyNo
             << " outerPlacement " << std::setw(8) << outerPlacement
             << " outerCopyNo " << std::setw(8) << outerCopyNo
             << std::endl 
             ;
    }
}

/**
X4PhysicalVolume::GetSensorPlacements
---------------------------------------

Populates placements with the void* origins obtained from ggeo, casting them back to G4PVPlacement.


Invoked from G4Opticks::translateGeometry, kinda feels misplaced being in X4PhysicalVolume
as depends only on GGeo+G4, perhaps should live in G4Opticks ?
Possibly the positioning is side effect from the difficulties of testing G4Opticks 
due to it not being able to boot from cache.

**/

void X4PhysicalVolume::GetSensorPlacements(const GGeo* gg, std::vector<G4PVPlacement*>& placements, bool outer_volume ) // static
{
    placements.clear(); 

    std::vector<void*> placements_ ; 
    gg->getSensorPlacements(placements_, outer_volume); 

    for(unsigned i=0 ; i < placements_.size() ; i++)
    {
         G4PVPlacement* p = static_cast<G4PVPlacement*>(placements_[i]); 
         placements.push_back(p); 
    } 
}


void X4PhysicalVolume::DumpSensorPlacements(const GGeo* gg, const char* msg, bool outer_volume) // static
{
    std::vector<G4PVPlacement*> sensors ; 
    X4PhysicalVolume::GetSensorPlacements(gg, sensors, outer_volume);
    int num_sen = sensors.size();  

    LOG(LEVEL) << msg <<  " num_sen " << num_sen ; 

    int lastCopyNo = -2 ;   
    int lastTransition = -2 ; 
    int margin = 10 ; 

    for(int i=0 ; i < num_sen ; i++)
    {   
         int sensorIndex = 1+i ; 
         const G4PVPlacement* sensor = sensors[sensorIndex-1] ; 
         G4int copyNo = sensor->GetCopyNo(); 

         if( lastCopyNo + 1 != copyNo ) lastTransition = i ; 

         if( i - lastTransition < margin || i < margin || num_sen - 1 - i < margin ) 
         std::cout 
             << " sensorIndex " << std::setw(6) << sensorIndex
             << " sensor " << std::setw(8) << sensor
             << " copyNo " << std::setw(6) << copyNo
             << std::endl 
             ;   

         lastCopyNo = copyNo ; 
    }   
}




