#include "App.hh"

// oglrap-  Frame brings in GL/glew.h GLFW/glfw3.h gleq.h
#include "Frame.hh"

// oglrap-
#define GUI_ 1
#ifdef GUI_
#include "GUI.hh"
#endif

// optickscore-
#include "Opticks.hh"
#include "OpticksCfg.hh"
#include "OpticksEvent.hh"
#include "OpticksPhoton.h"
#include "OpticksResource.hh"
#include "Bookmarks.hh"
#include "Composition.hh"
#include "InterpolatedView.hh"

// oglrap-
#include "StateGUI.hh"
#include "Scene.hh"
#include "SceneCfg.hh"
#include "Renderer.hh"
#include "RendererCfg.hh"
#include "Interactor.hh"
#include "InteractorCfg.hh"
#include "Rdr.hh"
#include "Texture.hh"
#include "Photons.hh"
#include "DynamicDefine.hh"

// numpyserver-
#ifdef WITH_NPYSERVER
#include "numpydelegate.hpp"
#include "numpydelegateCfg.hpp"
#include "numpyserver.hpp"
#endif

// npy-
#include "NLog.hpp"
#include "NState.hpp"
#include "NPY.hpp"
#include "GLMPrint.hpp"
#include "GLMFormat.hpp"
#include "ViewNPY.hpp"
#include "MultiViewNPY.hpp"
#include "Lookup.hpp"
#include "G4StepNPY.hpp"
#include "TorchStepNPY.hpp"
#include "PhotonsNPY.hpp"
#include "HitsNPY.hpp"
#include "RecordsNPY.hpp"
#include "BoundariesNPY.hpp"
#include "SequenceNPY.hpp"
#include "Types.hpp"
#include "Index.hpp"
#include "stringutil.hpp"
#include "Timer.hpp"
#include "Times.hpp"
#include "TimesTable.hpp"
#include "Parameters.hpp"
#include "Report.hpp"
#include "NSlice.hpp"

// glm-
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ggeo-
#include "GCache.hh"
#include "GGeo.hh"
#include "GMergedMesh.hh"
#include "GGeoLib.hh"
#include "GBndLib.hh"
#include "GMaterialLib.hh"
#include "GSurfaceLib.hh"
#include "GPmt.hh"
#include "GParts.hh"
#include "GFlags.hh"
#include "GColors.hh"
#include "GItemIndex.hh"
#include "GAttrSeq.hh"

// assimpwrap
#include "AssimpGGeo.hh"

// openmeshrap-
#include "MFixer.hh"
#include "MTool.hh"

#ifdef WITH_OPTIX
// optixgl-
#include "OpViz.hh"
// opticksop-
#include "OpEngine.hh"
#endif



#define GLMVEC4(g) glm::vec4((g).x,(g).y,(g).z,(g).w) 

#define TIMER(s) \
    { \
       (*m_timer)((s)); \
       if(m_evt)\
       {\
          Timer& t = *(m_evt->getTimer()) ;\
          t((s)) ;\
       }\
    }


void App::init(int argc, char** argv)
{
    m_opticks = new Opticks(argc, argv);
    m_opticks->Summary("App::init OpticksResource::Summary");

    m_cache = new GCache(m_opticks);

    m_parameters = new Parameters ;  // favor evt params over these, as evt params are persisted with the evt
    m_timer      = new Timer("App::");
    m_timer->setVerbose(true);
    m_timer->start();

    m_composition = new Composition ;   // Composition no longer Viz only

    m_cfg  = new Cfg("umbrella", false) ; 
    m_fcfg = m_opticks->getCfg();

    m_cfg->add(m_fcfg);

#ifdef WITH_NPYSERVER
    m_delegate    = new numpydelegate ; 
    m_cfg->add(new numpydelegateCfg<numpydelegate>("numpydelegate", m_delegate, false));
#endif

    TIMER("init");
}

void App::initViz()
{
    if(m_opticks->isCompute()) return ; 

    // envvars normally not defined, using cmake configure_file values instead
    const char* shader_dir = getenv("OPTICKS_SHADER_DIR"); 
    const char* shader_incl_path = getenv("OPTICKS_SHADER_INCL_PATH"); 
    const char* shader_dynamic_dir = getenv("OPTICKS_SHADER_DYNAMIC_DIR"); 

    m_scene      = new Scene(shader_dir, shader_incl_path, shader_dynamic_dir ) ;
    m_frame       = new Frame ; 
    m_interactor  = new Interactor ; 

    m_interactor->setFrame(m_frame);
    m_interactor->setScene(m_scene);
    m_interactor->setComposition(m_composition);

    m_scene->setInteractor(m_interactor);      

    m_frame->setInteractor(m_interactor);      
    m_frame->setComposition(m_composition);
    m_frame->setScene(m_scene);

    m_cfg->add(new SceneCfg<Scene>(           "scene",       m_scene,                      true));
    m_cfg->add(new RendererCfg<Renderer>(     "renderer",    m_scene->getGeometryRenderer(), true));
    m_cfg->add(new InteractorCfg<Interactor>( "interactor",  m_interactor,                 true));

    TIMER("initViz");
}


void App::configure(int argc, char** argv)
{
    LOG(debug) << "App:configure " << argv[0] ; 

    m_composition->addConfig(m_cfg); 
    //m_cfg->dumpTree();

    m_cfg->commandline(argc, argv);
    m_opticks->configure();        // hmm: m_cfg should live inside Opticks


    if(m_fcfg->hasError())
    {
        LOG(fatal) << "App::config parse error " << m_fcfg->getErrorMessage() ; 
        m_fcfg->dump("App::config m_fcfg");
        setExit(true);
        return ; 
    }

    bool compute = hasOpt("compute") ;
    assert(compute == m_opticks->isCompute() && "App::configure compute mismatch between GCache pre-configure and configure"  ); 

    if(hasOpt("idpath")) std::cout << m_cache->getIdPath() << std::endl ;
    if(hasOpt("help"))   std::cout << m_cfg->getDesc()     << std::endl ;
    if(hasOpt("help|version|idpath"))
    {
        setExit(true);
        return ; 
    }

    if(!m_opticks->isValid())
    {
        // defer death til after getting help
        LOG(fatal) << "App::configure OPTICKS INVALID : missing envvar or geometry path ?" ;
        assert(0);
    }

    if(!hasOpt("noevent"))
    {
        // TODO: try moving event creation after geometry is loaded, to avoid need to update domains 
        // TODO: organize wrt event loading, currently loading happens latter and trumps this evt ?
        m_evt = m_opticks->makeEvent() ; 
        //m_evt->setFlat(true);
    } 

#ifdef WITH_NPYSERVER
    if(!hasOpt("nonet"))
    {
        m_delegate->liveConnect(m_cfg); // hookup live config via UDP messages
        m_delegate->setEvent(m_evt); // allows delegate to update evt when NPY messages arrive, hmm locking needed ?

        try { 
            m_server = new numpyserver<numpydelegate>(m_delegate); // connect to external messages 
        } 
        catch( const std::exception& e)
        {
            LOG(fatal) << "App::config EXCEPTION " << e.what() ; 
            LOG(fatal) << "App::config FAILED to instanciate numpyserver : probably another instance is running : check debugger sessions " ;
        }
    }
#endif

    configureViz();

    TIMER("configure");
}



void App::configureViz()
{
    if(m_opticks->isCompute()) return ; 

    m_state = m_opticks->getState();
    m_state->setVerbose(false);

    LOG(info) << "App::configureViz " << m_state->description();

    assert(m_composition);

    m_state->addConfigurable(m_scene);
    m_composition->addConstituentConfigurables(m_state); // constituents: trackball, view, camera, clipper

    m_composition->setOrbitalViewPeriod(m_fcfg->getOrbitalViewPeriod()); 
    m_composition->setAnimatorPeriod(m_fcfg->getAnimatorPeriod()); 

    if(m_evt)
    { 
        m_composition->setEvt(m_evt);
        m_composition->setTrackViewPeriod(m_fcfg->getTrackViewPeriod()); 

        bool quietly = true ; 
        NPY<float>* track = m_evt->loadGenstepDerivativeFromFile("track", quietly);
        m_composition->setTrack(track);
    }

    LOG(info) << "App::configureViz m_setup bookmarks" ;  

    m_bookmarks   = new Bookmarks(m_state->getDir()) ; 
    m_bookmarks->setState(m_state);
    m_bookmarks->setVerbose();
    m_bookmarks->setInterpolatedViewPeriod(m_fcfg->getInterpolatedViewPeriod());

    if(m_interactor)
    {
        m_interactor->setBookmarks(m_bookmarks);
    }

    TIMER("configureViz");
}



void App::prepareViz()
{
    if(m_opticks->isCompute()) return ; 

    m_size = m_opticks->getSize();
    glm::uvec4 position = m_opticks->getPosition() ;

    LOG(info) << "App::prepareViz"
              << " size " << gformat(m_size)
              << " position " << gformat(position)
              ;

    m_scene->setEvent(m_evt);
    if(m_opticks->isJuno())
    {
        LOG(warning) << "App::prepareViz disable GeometryStyle  WIRE for JUNO as too slow " ;

        m_scene->setNumGeometryStyle(Scene::WIRE); 
        m_scene->setNumGlobalStyle(Scene::GVISVEC); // disable GVISVEC, GVEC debug styles

        m_scene->setRenderMode("bb0,bb1,-global");
        std::string rmode = m_scene->getRenderMode();
        LOG(info) << "App::prepareViz " << rmode ; 
    }
    else if(m_opticks->isDayabay())
    {
        m_scene->setNumGlobalStyle(Scene::GVISVEC);   // disable GVISVEC, GVEC debug styles
    }


    m_composition->setSize( m_size );
    m_composition->setFramePosition( position );

    m_frame->setTitle("GGeoView");
    m_frame->setFullscreen(hasOpt("fullscreen"));

    m_dd = new DynamicDefine();   // configuration used in oglrap- shaders
    m_dd->add("MAXREC",m_fcfg->getRecordMax());    
    m_dd->add("MAXTIME",m_fcfg->getTimeMax());    
    m_dd->add("PNUMQUAD", 4);  // quads per photon
    m_dd->add("RNUMQUAD", 2);  // quads per record 
    m_dd->add("MATERIAL_COLOR_OFFSET", (unsigned int)GColors::MATERIAL_COLOR_OFFSET );
    m_dd->add("FLAG_COLOR_OFFSET", (unsigned int)GColors::FLAG_COLOR_OFFSET );
    m_dd->add("PSYCHEDELIC_COLOR_OFFSET", (unsigned int)GColors::PSYCHEDELIC_COLOR_OFFSET );
    m_dd->add("SPECTRAL_COLOR_OFFSET", (unsigned int)GColors::SPECTRAL_COLOR_OFFSET );


    m_scene->write(m_dd);

    m_scene->initRenderers();  // reading shader source and creating renderers

    m_frame->init();           // creates OpenGL context

    m_window = m_frame->getWindow();

    m_scene->setComposition(m_composition);     // defer until renderers are setup 

    // defer creation of the altview to Interactor KEY_U so newly created bookmarks are included
    m_composition->setBookmarks(m_bookmarks);


    TIMER("prepareViz");
} 




void App::loadGeometry()
{
    bool modify = hasOpt("test") ;

    LOG(info) << "App::loadGeometry START, modifyGeometry? " << modify  ; 

    loadGeometryBase();

    if(!m_ggeo->isValid())
    {
        LOG(warning) << "App::loadGeometry finds invalid geometry, try creating geocache with --nogeocache/-G option " ; 
        setExit(true); 
        return ; 
    }

    if(modify) modifyGeometry() ;


    fixGeometry();

    registerGeometry();

    if(!m_opticks->isGeocache())
    {
        LOG(info) << "App::loadGeometry early exit due to --nogeocache/-G option " ; 
        setExit(true); 
    }

    configureGeometry();

    TIMER("loadGeometry");
}


void App::loadGeometryBase()
{
    // hmm funny placement, move this just after config 
    m_opticks->setGeocache(!m_fcfg->hasOpt("nogeocache"));
    m_opticks->setInstanced( !m_fcfg->hasOpt("noinstanced")  ); // find repeated geometry 

    OpticksResource* resource = m_opticks->getResource();

    m_ggeo = new GGeo(m_cache);

    if(hasOpt("qe1"))
        m_ggeo->getSurfaceLib()->setFakeEfficiency(1.0);

    m_ggeo->setLoaderImp(&AssimpGGeo::load);    // setting GLoaderImpFunctionPtr
    m_ggeo->setLoaderVerbosity(m_fcfg->getLoaderVerbosity());    
    m_ggeo->setMeshVerbosity(m_fcfg->getMeshVerbosity());    


    m_ggeo->setMeshJoinImp(&MTool::joinSplitUnion);
    m_ggeo->setMeshJoinCfg( resource->getMeshfix() );

    std::string meshversion = m_fcfg->getMeshVersion() ;;
    if(!meshversion.empty())
    {
        LOG(warning) << "App::loadGeometry using debug meshversion " << meshversion ;  
        m_ggeo->getGeoLib()->setMeshVersion(meshversion.c_str());
    }

    m_ggeo->loadGeometry();
        
    if(m_ggeo->getMeshVerbosity() > 2)
    {
        GMergedMesh* mesh1 = m_ggeo->getMergedMesh(1);
        if(mesh1)
        {
            mesh1->dumpSolids("App::loadGeometryBase mesh1");
            mesh1->save("/tmp", "GMergedMesh", "baseGeometry") ;
        }
    }

    TIMER("loadGeometryBase");
}

void App::modifyGeometry()
{
    assert(hasOpt("test"));
    LOG(debug) << "App::modifyGeometry" ;

    std::string testconf = m_fcfg->getTestConfig();
    m_ggeo->modifyGeometry( testconf.empty() ? NULL : testconf.c_str() );


    if(m_ggeo->getMeshVerbosity() > 2)
    {
        GMergedMesh* mesh0 = m_ggeo->getMergedMesh(0);
        if(mesh0)
        { 
            mesh0->dumpSolids("App::modifyGeometry mesh0");
            mesh0->save("/tmp", "GMergedMesh", "modifyGeometry") ;
        }
    }


    TIMER("modifyGeometry"); 
}


void App::fixGeometry()
{
    if(m_ggeo->isLoaded())
    {
        LOG(debug) << "App::fixGeometry needs to be done precache " ;
        return ; 
    }
    LOG(info) << "App::fixGeometry" ; 

    MFixer* fixer = new MFixer(m_ggeo);
    fixer->setVerbose(hasOpt("meshfixdbg"));
    fixer->fixMesh();
 
    bool zexplode = m_fcfg->hasOpt("zexplode");
    if(zexplode)
    {
       // for --jdyb --idyb --kdyb testing : making the cleave OR the mend obvious
        glm::vec4 zexplodeconfig = gvec4(m_fcfg->getZExplodeConfig());
        print(zexplodeconfig, "zexplodeconfig");

        GMergedMesh* mesh0 = m_ggeo->getMergedMesh(0);
        mesh0->explodeZVertices(zexplodeconfig.y, zexplodeconfig.x ); 
    }
    TIMER("fixGeometry"); 
}



void App::configureGeometry()
{
    int restrict_mesh = m_fcfg->getRestrictMesh() ;  
    int analytic_mesh = m_fcfg->getAnalyticMesh() ; 

    int nmm = m_ggeo->getNumMergedMesh();

    LOG(info) << "App::configureGeometry" 
              << " restrict_mesh " << restrict_mesh
              << " analytic_mesh " << analytic_mesh
              << " nmm " << nmm
              ;

    std::string instance_slice = m_fcfg->getISlice() ;;
    std::string face_slice = m_fcfg->getFSlice() ;;
    std::string part_slice = m_fcfg->getPSlice() ;;

    NSlice* islice = !instance_slice.empty() ? new NSlice(instance_slice.c_str()) : NULL ; 
    NSlice* fslice = !face_slice.empty() ? new NSlice(face_slice.c_str()) : NULL ; 
    NSlice* pslice = !part_slice.empty() ? new NSlice(part_slice.c_str()) : NULL ; 

    for(int i=0 ; i < nmm ; i++)
    {
        GMergedMesh* mm = m_ggeo->getMergedMesh(i);
        if(restrict_mesh > -1 && i != restrict_mesh ) mm->setGeoCode(Opticks::GEOCODE_SKIP);      
        if(analytic_mesh > -1 && i == analytic_mesh && i > 0) 
        {
            GPmt* pmt = m_ggeo->getPmt(); 
            assert(pmt && "analyticmesh requires PMT resource");

            GParts* analytic = pmt->getParts() ;
            // TODO: the strings should come from config, as detector specific

            analytic->setVerbose(true); 
            analytic->setContainingMaterial("MineralOil");       
            analytic->setSensorSurface("lvPmtHemiCathodeSensorSurface");

            mm->setGeoCode(Opticks::GEOCODE_ANALYTIC);      
            mm->setParts(analytic);  
        }
        if(i>0) mm->setInstanceSlice(islice);

        // restrict to non-global for now
        if(i>0) mm->setFaceSlice(fslice);   
        if(i>0) mm->setPartSlice(pslice);   
    }

    TIMER("configureGeometry"); 
}




void App::registerGeometry()
{
    LOG(info) << "App::registerGeometry" ; 

    //for(unsigned int i=1 ; i < m_ggeo->getNumMergedMesh() ; i++) m_ggeo->dumpNodeInfo(i);

    m_mesh0 = m_ggeo->getMergedMesh(0); 

    m_ggeo->setComposition(m_composition);

    gfloat4 ce0 = m_mesh0->getCenterExtent(0);  // 0 : all geometry of the mesh, >0 : specific volumes
    m_opticks->setSpaceDomain( glm::vec4(ce0.x,ce0.y,ce0.z,ce0.w) );

    if(m_evt)
    {
       // TODO: profit from migrated OpticksEvent 
        LOG(info) << "App::registerGeometry " << m_opticks->description() ;
        m_evt->setSpaceDomain(m_opticks->getSpaceDomain());
    }

    LOG(debug) << "App::registerGeometry ce0: " 
                      << " x " << ce0.x
                      << " y " << ce0.y
                      << " z " << ce0.z
                      << " w " << ce0.w
                      ;
 
    TIMER("registerGeometry"); 
}




void App::uploadGeometryViz()
{
    if(m_opticks->isCompute()) return ; 


    GColors* colors = m_cache->getColors();

    guint4 cd = colors->getCompositeDomain() ; 
    glm::uvec4 cd_(cd.x, cd.y, cd.z, cd.w );
  
    m_composition->setColorDomain(cd_); 

    m_scene->uploadColorBuffer( colors->getCompositeBuffer() );  //     oglrap-/Colors preps texture, available to shaders as "uniform sampler1D Colors"

    m_composition->setTimeDomain( m_opticks->getTimeDomain() );
    m_composition->setDomainCenterExtent(m_opticks->getSpaceDomain());


    m_scene->setGeometry(m_ggeo);

    m_scene->uploadGeometry();

    bool autocam = true ; 

    // handle commandline --target option that needs loaded geometry 
    unsigned int target = m_scene->getTargetDeferred();   // default to 0 
    LOG(debug) << "App::uploadGeometryViz setting target " << target ; 

    m_scene->setTarget(target, autocam);
 
    TIMER("uploadGeometryViz"); 
}









void App::loadGenstep()
{
    if(hasOpt("nooptix|noevent")) 
    {
        LOG(warning) << "App::loadGenstep skip due to --nooptix/--noevent " ;
        return ;
    }

    unsigned int code = m_opticks->getSourceCode();
    Lookup* lookup = m_ggeo->getLookup();


    NPY<float>* npy = NULL ; 
    if( code == CERENKOV || code == SCINTILLATION )
    {
        int modulo = m_fcfg->getModulo();
        npy = m_evt->loadGenstepFromFile(modulo);

        m_g4step = new G4StepNPY(npy);    
        m_g4step->relabel(code); // becomes the ghead.i.x used in cu/generate.cu

        if(m_opticks->isDayabay())
        {   
            m_g4step->setLookup(lookup);   
            m_g4step->applyLookup(0, 2);      
        }
    }
    else if(code == TORCH)
    {
        m_torchstep = m_opticks->makeSimpleTorchStep();

        m_ggeo->targetTorchStep(m_torchstep);

        const char* material = m_torchstep->getMaterial() ;
        unsigned int matline = m_ggeo->getMaterialLine(material);
        m_torchstep->setMaterialLine(matline);  

        LOG(debug) << "App::makeSimpleTorchStep"
                  << " config " << m_torchstep->getConfig() 
                  << " material " << material 
                  << " matline " << matline
                  ;

        bool verbose = hasOpt("torchdbg");
        m_torchstep->addStep(verbose);  // copyies above configured step settings into the NPY and increments the step index, ready for configuring the next step 

        npy = m_torchstep->getNPY(); 
    }
    

    TIMER("loadGenstep"); 

    m_evt->setGenstepData(npy); 

    TIMER("setGenstepData"); 
}



void App::targetViz()
{
    if(m_opticks->isCompute()) return ; 

    glm::vec4 mmce = GLMVEC4(m_mesh0->getCenterExtent(0)) ;

    glm::vec4 gsce = (*m_evt)["genstep.vpos"]->getCenterExtent();
    bool geocenter  = m_fcfg->hasOpt("geocenter");
    glm::vec4 uuce = geocenter ? mmce : gsce ;

    unsigned int target = m_scene->getTarget() ;

    LOG(info) << "App::targetViz"
              << " target " << target      
              << " geocenter " << geocenter      
              << " mmce " << gformat(mmce)
              << " gsce " << gformat(gsce)
              << " uuce " << gformat(uuce)
              ;

    if(target == 0)
    {
        // only pointing based in genstep if not already targetted
        bool autocam = true ; 
        m_composition->setCenterExtent( uuce , autocam );
    }

    m_scene->setRecordStyle( m_fcfg->hasOpt("alt") ? Scene::ALTREC : Scene::REC );    

    TIMER("targetViz"); 
}


void App::loadEvtFromFile()
{
    LOG(info) << "App::loadEvtFromFile START" ;
   
    bool verbose ; 
    m_evt->loadBuffers(verbose=false);

    if(m_evt->isNoLoad())
        LOG(warning) << "App::loadEvtFromFile LOAD FAILED " ;

    TIMER("loadEvtFromFile"); 
}


void App::uploadEvtViz()
{
    if(m_opticks->isCompute()) return ; 

    if(hasOpt("nooptix|noevent")) 
    {
        LOG(warning) << "App::uploadEvtViz skip due to --nooptix/--noevent " ;
        return ;
    }
 
    LOG(info) << "App::uploadEvtViz START " ;

    m_composition->update();

    m_scene->upload();

    m_scene->uploadSelection();

    m_scene->dump_uploads_table("App::uploadEvtViz");


    TIMER("uploadEvtViz"); 
}





void App::indexPresentationPrep()
{
    LOG(info) << "App::indexPresentationPrep" ; 

    if(!m_evt) return ; 

    Index* seqhis = m_evt->getHistoryIndex() ;
    Index* seqmat = m_evt->getMaterialIndex();
    Index* bndidx = m_evt->getBoundaryIndex();

    if(!seqhis)
    {
         LOG(warning) << "App::indexPresentationPrep NULL seqhis" ;
    }
    else
    {
        m_seqhis = new GItemIndex(seqhis) ;  
        m_seqhis->setTitle("Photon Flag Sequence Selection");

        bool oldway = false ; 
        if(oldway)
        {
           // this succeeds to display the labels like : "TORCH BT SA" 
           // BUT: the radio button selection does not select, all photons disappear 
           //
            Types* types = m_cache->getTypes();    
            m_seqhis->setTypes(types);        
            m_seqhis->setLabeller(GItemIndex::HISTORYSEQ);
        }
        else
        {
            GFlags* flags = m_cache->getFlags();
            GAttrSeq* qflg = flags->getAttrIndex();

            qflg->setCtrl(GAttrSeq::SEQUENCE_DEFAULTS);
            //qflg->dumpTable(seqhis, "App::indexPresentationPrep seqhis"); 

            m_seqhis->setHandler(qflg);
        }
        m_seqhis->formTable();
    }


    if(!seqmat)
    {
         LOG(warning) << "App::indexPresentationPrep NULL seqmat" ;
    }
    else
    {
        GAttrSeq* qmat = m_ggeo->getMaterialLib()->getAttrNames(); 
        qmat->setCtrl(GAttrSeq::SEQUENCE_DEFAULTS);
        //qmat->dumpTable(seqmat, "App::indexPresentationPrep seqmat"); 
        m_seqmat = new GItemIndex(seqmat) ;  
        m_seqmat->setTitle("Photon Material Sequence Selection");
        m_seqmat->setHandler(qmat);
        m_seqmat->formTable();
    }


    if(!bndidx)
    {
         LOG(warning) << "App::indexPresentationPrep NULL bndidx" ;
    }
    else
    {
        GBndLib* blib = m_ggeo->getBndLib();
        GAttrSeq* qbnd = blib->getAttrNames();
        if(!qbnd->hasSequence())
        {
            blib->close();
            assert(qbnd->hasSequence());
        }
        qbnd->setCtrl(GAttrSeq::VALUE_DEFAULTS);
        //qbnd->dumpTable(bndidx, "App::indexPresentationPrep bndidx"); 

        m_boundaries = new GItemIndex(bndidx) ;  
        m_boundaries->setTitle("Photon Termination Boundaries");
        m_boundaries->setHandler(qbnd);
        m_boundaries->formTable();
    } 


    TIMER("indexPresentationPrep"); 
}


void App::indexBoundariesHost()
{
    // Indexing the final signed integer boundary code (p.flags.i.x = prd.boundary) from optixrap-/cu/generate.cu
    // see also opop-/OpIndexer::indexBoundaries for GPU version of this indexing 

    if(!m_evt) return ; 

    GBndLib* blib = m_ggeo->getBndLib();
    GAttrSeq* qbnd = blib->getAttrNames();
    if(!qbnd->hasSequence())
    {
         blib->close();
         assert(qbnd->hasSequence());
    }

    std::map<unsigned int, std::string> boundary_names = qbnd->getNamesMap(GAttrSeq::ONEBASED) ;

    NPY<float>* dpho = m_evt->getPhotonData();
    if(dpho && dpho->hasData())
    {
        // host based indexing of unique material codes, requires downloadEvt to pull back the photon data
        LOG(info) << "App::indexBoundaries host based " ;
        m_bnd = new BoundariesNPY(dpho); 
        m_bnd->setBoundaryNames(boundary_names); 
        m_bnd->indexBoundaries();     
    } 
    else
    {
        LOG(warning) << "App::indexBoundaries dpho NULL or no data " ;
    }


    TIMER("indexBoundariesHost"); 
}


void App::indexEvt()
{
   /*

       INTEROP mode GPU buffer access C:create R:read W:write
       ----------------------------------------------------------

                     OpenGL     OptiX              Thrust 

       gensteps       CR       R (gen/prop)       R (seeding)

       photons        CR       W (gen/prop)       W (seeding)
       sequence                W (gen/prop)
       phosel         CR                          W (indexing) 

       records        CR       W  
       recsel         CR                          W (indexing)


       OptiX has no business with phosel and recsel 
   */

    if(!m_evt) return ; 

    if(m_evt->isIndexed())
    {
        LOG(info) << "App::indexEvt" 
                  << " skip as already indexed "
                  ;
        return ; 
    }


#ifdef WITH_OPTIX 
    LOG(info) << "App::indexEvt WITH_OPTIX" ; 

    indexSequence();

    LOG(info) << "App::indexEvt WITH_OPTIX DONE" ; 
#endif

    indexBoundariesHost();

    TIMER("indexEvt"); 
}


void App::indexEvtOld()
{
    if(!m_evt) return ; 

    // TODO: wean this off use of Types, for the new way (GFlags..)
    Types* types = m_cache->getTypes();
    Typ* typ = m_cache->getTyp();

    NPY<float>* ox = m_evt->getPhotonData();


    if(ox && ox->hasData())
    {
        m_pho = new PhotonsNPY(ox);   // a detailed photon/record dumper : looks good for photon level debug 
        m_pho->setTypes(types);
        m_pho->setTyp(typ);

        m_hit = new HitsNPY(ox, m_ggeo->getSensorList());
        //m_hit->debugdump();
    }

    // hmm thus belongs in NumpyEvt rather than here
    NPY<short>* rx = m_evt->getRecordData();

    if(rx && rx->hasData())
    {
        m_rec = new RecordsNPY(rx, m_evt->getMaxRec(), m_evt->isFlat());
        m_rec->setTypes(types);
        m_rec->setTyp(typ);
        m_rec->setDomains(m_evt->getFDomain()) ;

        if(m_pho)
        {
            m_pho->setRecs(m_rec);
            //if(m_torchstep) m_torchstep->dump("App::indexEvtOld TorchStepNPY");

            m_pho->dump(0  ,  "App::indexEvtOld dpho 0");
            m_pho->dump(100,  "App::indexEvtOld dpho 100" );
            m_pho->dump(1000, "App::indexEvtOld dpho 1000" );

        }
        m_evt->setRecordsNPY(m_rec);
        m_evt->setPhotonsNPY(m_pho);
    }

    TIMER("indexEvtOld"); 
}




void App::prepareGUI()
{
    if(m_opticks->isCompute()) return ; 

    m_bookmarks->create(0);

#ifdef GUI_

    m_types = m_cache->getTypes();  // needed for each render
    m_photons = new Photons(m_types, m_boundaries, m_seqhis, m_seqmat ) ; // GUI jacket 
    m_scene->setPhotons(m_photons);

    m_gui = new GUI(m_ggeo) ;
    m_gui->setScene(m_scene);
    m_gui->setPhotons(m_photons);
    m_gui->setComposition(m_composition);
    m_gui->setBookmarks(m_bookmarks);
    m_gui->setStateGUI(new StateGUI(m_state));
    m_gui->setInteractor(m_interactor);   // status line
    
    m_gui->init(m_window);
    m_gui->setupHelpText( m_cfg->getDescString() );

    TimesTable* tt = m_evt ? m_evt->getTimesTable() : NULL ; 
    if(tt)
    {
        m_gui->setupStats(tt->getLines());
    }
    else
    {
        LOG(warning) << "App::prepareGUI NULL TimesTable " ; 
    }  

    Parameters* parameters = m_evt ? m_evt->getParameters() : m_parameters ; 

    m_gui->setupParams(parameters->getLines());

#endif

    TIMER("prepareGUI"); 
}


void App::renderGUI()
{
#ifdef GUI_
    m_gui->newframe();
    bool* show_gui_window = m_interactor->getGUIModeAddress();
    if(*show_gui_window)
    {
        m_gui->show(show_gui_window);
        if(m_photons)
        {
            if(m_boundaries)
            {
                m_composition->getPick().y = m_boundaries->getSelected() ;   //  1st boundary 
            }
            glm::ivec4& recsel = m_composition->getRecSelect();
            recsel.x = m_seqhis ? m_seqhis->getSelected() : 0 ; 
            recsel.y = m_seqmat ? m_seqmat->getSelected() : 0 ; 
            m_composition->setFlags(m_types->getFlags()); 
        }
        // maybe imgui edit selection within the composition imgui, rather than shovelling ?
        // BUT: composition feeds into shader uniforms which could be reused by multiple classes ?
    }

    bool* show_scrub_window = m_interactor->getScrubModeAddress();
    if(*show_scrub_window)
        m_gui->show_scrubber(show_scrub_window);

    m_gui->render();
#endif
}




void App::render()
{
    if(m_opticks->isCompute()) return ; 

    m_frame->viewport();
    m_frame->clear();

#ifdef WITH_OPTIX
    if(m_scene->isRaytracedRender() || m_scene->isCompositeRender())
    {
        if(m_opv) m_opv->render();
    }
#endif
    m_scene->render();
}



void App::renderLoop()
{
    if(m_opticks->isCompute()) return ; 

    if(hasOpt("noviz"))
    {
        LOG(info) << "App::renderLoop early exit due to --noviz/-V option " ; 
        return ;
    }
    LOG(info) << "enter runloop "; 

    //m_frame->toggleFullscreen(true); causing blankscreen then segv
    m_frame->hintVisible(true);
    m_frame->show();
    LOG(info) << "after frame.show() "; 

    unsigned int count ; 

    while (!glfwWindowShouldClose(m_window))
    {
        m_frame->listen(); 
#ifdef WITH_NPYSERVER
        if(m_server) m_server->poll_one();  
#endif
        count = m_composition->tick();

        if( m_composition->hasChanged() || m_interactor->hasChanged() || count == 1)  
        {
            render();
            renderGUI();

            glfwSwapBuffers(m_window);

            m_interactor->setChanged(false);  
            m_composition->setChanged(false);   // sets camera, view, trackball dirty status 
        }
    }
}



void App::cleanup()
{
#ifdef WITH_OPTIX
    if(m_ope) m_ope->cleanup();
#endif

#ifdef WITH_NPYSERVER
    if(m_server) m_server->stop();
#endif
#ifdef GUI_
    if(m_gui) m_gui->shutdown();
#endif
    if(m_frame) m_frame->exit();
}


bool App::hasOpt(const char* name)
{
    return m_fcfg->hasOpt(name);
}



#ifdef WITH_OPTIX
void App::prepareOptiX()
{
    LOG(info) << "App::prepareOptiX create OpEngine " ; 
    m_ope = new OpEngine(m_opticks, m_ggeo);
    m_ope->prepareOptiX();
}

void App::prepareOptiXViz()
{
    if(!m_ope) return ; 
    m_opv = new OpViz(m_ope, m_scene); 
}

void App::setupEventInEngine()
{
    if(!m_ope) return ; 
    m_ope->setEvent(m_evt);  // without this cannot index
}

void App::preparePropagator()
{
    if(!m_ope) return ; 
    m_ope->preparePropagator();
}

void App::seedPhotonsFromGensteps()
{
    if(!m_ope) return ; 
    m_ope->seedPhotonsFromGensteps();
}

void App::initRecords()
{
    if(!m_ope) return ; 
    m_ope->initRecords();
}

void App::propagate()
{
    if(hasOpt("nooptix|noevent|nopropagate")) 
    {
        LOG(warning) << "App::propagate skip due to --nooptix/--noevent/--nopropagate " ;
        return ;
    }
    if(!m_ope) return ; 
    m_ope->propagate();
}

void App::saveEvt()
{
    if(!m_ope) return ; 
    if(!m_opticks->isCompute()) 
    {
        Rdr::download(m_evt);
    }
    m_ope->saveEvt();
}

void App::indexSequence()
{
    if(!m_ope)
    {
        LOG(warning) << "App::indexSequence NULL OpEngine " ;
        return ; 
    }

    //m_evt->prepareForIndexing();  // stomps on prior recsel phosel buffers, causes CUDA error with Op indexing, but needed for G4 indexing  
    LOG(info) << "App::indexSequence evt shape " << m_evt->getShapeString() ;

    m_ope->indexSequence();
    LOG(info) << "App::indexSequence DONE" ;
}

#endif






