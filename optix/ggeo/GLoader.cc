#include "GLoader.hh"

// look no hands : no dependency on AssimpWrap 

#include "GVector.hh"
#include "GMergedMesh.hh"
#include "GBoundaryLib.hh"
#include "GScintillatorLib.hh"
#include "GBoundaryLibMetadata.hh"
#include "GTraverse.hh"
#include "GColorizer.hh"
#include "GTreeCheck.hh"
#include "GTreePresent.hh"
#include "GItemIndex.hh"
#include "GBuffer.hh"
#include "GMaterial.hh"

#include "GGeo.hh"
#include "GCache.hh"
#include "GColors.hh"
#include "GColorMap.hh"

// npy-
#include "NSensorList.hpp"
#include "stringutil.hpp"
#include "Lookup.hpp"
#include "Types.hpp"
#include "Timer.hpp"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <boost/log/trivial.hpp>
#define LOG BOOST_LOG_TRIVIAL
// trace/debug/info/warning/error/fatal


void GLoader::load(bool verbose)
{
    Timer t("GLoader::load") ; 
    t.setVerbose(verbose);
    t.start();

    assert(m_cache);

    const char* idpath = m_cache->getIdPath() ;

    LOG(info) << "GLoader::load start " 
              << " idpath " << idpath 
              << " repeatidx " << m_repeatidx 
              ;


    if(m_ggeo->isLoaded()) 
    {
        LOG(info) << "GLoader::load loading from cache directory " << idpath ;

        m_ggeo->loadFromCache() ; 

        t("loadFromCache"); 

        GBoundaryLib* blib = m_ggeo->getBoundaryLib();

        t("getBoundaryLib"); 
        // TODO: move below into GGeo::load 

        m_metadata = blib->getMetadata() ; 

        m_materials  = GItemIndex::load(idpath, "GMaterialIndex"); 
        m_surfaces   = GItemIndex::load(idpath, "GSurfaceIndex");
        m_meshes     = GItemIndex::load(idpath, "MeshIndex");

        t("indices load"); 
        if(verbose)
            m_materials->dump("GLoader::load original materials from GItemIndex::load(idpath, \"GMaterialIndex\") ");
    } 
    else
    {
        LOG(info) << "GLoader::load slow loading using m_loader_imp (disguised AssimpGGeo) " ;


        m_ggeo->loadFromG4DAE(); 

        t("create m_ggeo from G4DAE"); 

        GBoundaryLib* blib = m_ggeo->getBoundaryLib();

        // material customization must be done prior to creating buffers, as they contain the customized indices ?
        //  

        m_materials = blib->getMaterials();  
        m_materials->loadIndex("$HOME/.opticks"); 

        // material/surface indices obtained from GItemIndex::getIndexLocal(shortname)
        blib->createWavelengthAndOpticalBuffers();

        t("createWavelengthAndOpticalBuffers"); 

        m_metadata = blib->getMetadata();
        m_surfaces = blib->getSurfaces();   

        GColorMap* sixc = GColorMap::load("$HOME/.opticks", "GSurfaceIndexColors.json");
        m_surfaces->setColorMap(sixc);   
        m_surfaces->setColorSource(m_ggeo->getColors());


        t("GColorizer"); 


        m_ggeo->save(idpath );

        LOG(info) << "GLoader::load saving to cache directory " << idpath ;

        // TODO: consolidate persistency management of below inside m_ggeo

        m_metadata->save(idpath);
        m_materials->save(idpath);
        m_surfaces->save(idpath);

        t("save geocache"); 
    } 
  
    // hmm not routing via cache 
    m_lookup = new Lookup() ; 
    m_lookup->create(idpath);
    //m_lookup->dump("GLoader::load");  

    Index* idx = m_types->getFlagsIndex() ;    
    m_flags = new GItemIndex( idx );     //GFlagIndex::load(idpath); 

    // itemname => colorname 
    m_flags->setColorMap(GColorMap::load("$HOME/.opticks", "GFlagIndexColors.json"));    
    m_materials->setColorMap(GColorMap::load("$HOME/.opticks", "GMaterialIndexColors.json")); 
    m_surfaces->setColorMap(GColorMap::load("$HOME/.opticks", "GSurfaceIndexColors.json"));   

    m_materials->setLabeller(GItemIndex::COLORKEY);
    m_surfaces->setLabeller(GItemIndex::COLORKEY);
    m_flags->setLabeller(GItemIndex::COLORKEY);

    GColors* colors = m_ggeo->getColors();
    
    m_materials->setColorSource(colors);
    m_surfaces->setColorSource(colors);
    m_flags->setColorSource(colors);

    // formTable is needed to construct labels and codes when not pulling a buffer
    // TODO: avoid this requirement
    // TODO: move above color prep into GColors 

    m_surfaces->formTable();
    m_flags->formTable(); 
    m_materials->formTable();


    std::vector<unsigned int>& material_codes = m_materials->getCodes() ; 
    std::vector<unsigned int>& flag_codes     = m_flags->getCodes() ; 

    colors->setupCompositeColorBuffer( material_codes, flag_codes  );
    
    //m_ggeo->getBoundaryLib()->setColorBuffer(colors->getCompositeBuffer());
    //m_ggeo->getBoundaryLib()->setColorDomain(colors->getCompositeDomain());


    LOG(info) << "GLoader::load done " << idpath ;
    //assert(m_mergedmesh);
    t.stop();
    if(verbose) t.dump();
}


