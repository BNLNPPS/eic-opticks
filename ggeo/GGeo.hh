#pragma once

#include <map>
#include <vector>
#include <unordered_set>
#include <iterator>

#include <glm/fwd.hpp>

// npy-
#include "NConfigurable.hpp"

class NLookup ; 
class TorchStepNPY ; 

// okc-
class Opticks ; 
class OpticksEvent ; 
class OpticksColors ; 
class OpticksFlags ; 
class OpticksResource ; 
class OpticksAttrSeq ; 
class Composition ; 

// ggeo-
#include "GVector.hh"
template <typename T> class GDomain ; 
template <typename T> class GPropertyMap ; 
template <typename T> class GProperty ; 

class GMesh ; 
class GSolid ; 
class GNode ; 
class GMaterial ; 
class GSkinSurface ; 
class GBorderSurface ; 

class GMeshLib ; 
class GNodeLib ; 
class GGeoLib ;
class GBndLib ;
class GMaterialLib ;
class GSurfaceLib ;
class GScintillatorLib ;
class GSourceLib ;
class GPmtLib ; 


class GTreeCheck ;
class GColorizer ; 

class GItemIndex ; 
class GItemList ; 
class GMergedMesh ;

// GLTF handling 
class NScene ; 
class GScene ; 

#include "GGeoBase.hh"

#include "GGEO_API_EXPORT.hh"
#include "GGEO_HEAD.hh"

/*
GGeo
=====

In the beginning GGeo was intended to be  a dumb substrate 
from which the geometry model is created eg by AssimpGGeo::convert 
However it grew to be somewhat monolithic.

When possible break pieces off the monolith.

*/

class GGEO_API GGeo : public GGeoBase, public NConfigurable {
    public:
        friend class  AssimpGGeo ; 
        friend struct GSceneTest ; 
    public:
        static const char* CATHODE_MATERIAL ; 
    public:
        // see GGeoCfg.hh
        static const char* PICKFACE ;   
        static const char* PREFIX ;
    public:
        // GGeoBase interface
        const char*       getIdentifier();
        GScintillatorLib* getScintillatorLib() ; 
        GSourceLib*       getSourceLib() ; 
        GBndLib*          getBndLib() ; 
        GGeoLib*          getGeoLib() ; 
    public:
        const char* getPrefix();
        void configure(const char* name, const char* value);
        std::vector<std::string> getTags();
        void set(const char* name, std::string& s);
        std::string get(const char* name);
    public:
        typedef int (*GLoaderImpFunctionPtr)(GGeo*);
        void setLoaderImp(GLoaderImpFunctionPtr imp);
        void setMeshVerbosity(unsigned int verbosity);
        unsigned int getMeshVerbosity();
    public:
        typedef GMesh* (*GJoinImpFunctionPtr)(GMesh*, Opticks*);
        void setMeshJoinImp(GJoinImpFunctionPtr imp);
        void setMeshJoinCfg(const char* config);
        bool shouldMeshJoin(GMesh* mesh);
        GMesh* invokeMeshJoin(GMesh* mesh);    // used from AssimpGGeo::convertMeshes immediately after GMesh birth and deduping
    public:
        typedef std::map<unsigned int, std::string> Index_t ;

    public:
        GGeo(Opticks* opticks); 
    public:
        const char* getIdPath();
        bool isValid();
    public:
        Composition* getComposition();
        void setComposition(Composition* composition);
    public:
        void loadGeometry(); 
        void loadFromCache();
        void loadFromG4DAE();  // AssimpGGeo::load
    private: 
        void loadAnalyticFromGLTF();
        void loadAnalyticFromCache();

        void afterConvertMaterials();
        //void createSurLib();
    public:
        // post-load setup
        void setupLookup();
        void setupColors();
        void setupTyp();
    public:
        // configureGeometry stage additions
    public:
        void prepareMaterialLib();
        void prepareSurfaceLib();
        void prepareScintillatorLib();
        void prepareMeshes();
        void prepareVertexColors();
    public:

        void setCathode(GMaterial* cathode);
        GMaterial* getCathode();  
        unsigned int getMaterialLine(const char* shortname);

   private:
        void init(); 
        //void loadMergedMeshes(const char* idpath);
        //void removeMergedMeshes(const char* idpath);
    public:
        void save();
        void saveAnalytic();
        void anaEvent(OpticksEvent* evt);
    private:
        //void saveMergedMeshes(const char* idpath);
    public:
        // pass thru to geolib
        GMergedMesh* makeMergedMesh(unsigned int index, GNode* base, GNode* root, unsigned verbosity );
        unsigned int getNumMergedMesh();
        GMergedMesh* getMergedMesh(unsigned int index);
    public:
        // these are operational from cache
        // target 0 : all geometry of the mesh, >0 : specific volumes
        glm::vec4 getCenterExtent(unsigned int target, unsigned int merged_mesh_index=0u );
        void dumpTree(const char* msg="GGeo::dumpTree");  
        void dumpVolume(unsigned int index, const char* msg="GGeo::dumpVolume");  
        void dumpNodeInfo(unsigned int mmindex, const char* msg="GGeo::dumpNodeInfo" );
        void dumpStats(const char* msg="GGeo::dumpStats");

        // merged mesh buffer offsets and counts
        //
        //     .x  prior faces offset    
        //     .y  prior verts offset  
        //     .z  index faces count
        //     .w  index verts count
        //
        glm::ivec4 getNodeOffsetCount(unsigned int index);
        glm::vec4 getFaceCenterExtent(unsigned int face_index, unsigned int solid_index, unsigned int mergedmesh_index=0 );
        glm::vec4 getFaceRangeCenterExtent(unsigned int face_index0, unsigned int face_index1, unsigned int solid_index, unsigned int mergedmesh_index=0 );

    private:
        glm::mat4 getTransform(int index);  //TRYING TO MOVE TO HUB 
    public:
        bool isLoaded();

    public:
        // via GNodeLib
        unsigned int getNumSolids();
        void add(GSolid*    solid);
        GNode* getNode(unsigned index); 
        GSolid* getSolid(unsigned int index);  
        GSolid* getSolidSimple(unsigned int index);  
        //GSolid* getSolidAnalytic(unsigned idx);

        const char* getPVName(unsigned int index);
        const char* getLVName(unsigned int index);

    private:
       // void _add(GMaterial* material);
    public:
        void add(GMaterial* material);
        void add(GSkinSurface*  surface);
        void add(GBorderSurface*  surface);

        void close();

        void addToIndex(GPropertyMap<float>* obj);
        void dumpIndex(const char* msg="GGeo::dumpIndex");

    public:
        void addRaw(GMaterial* material);
        void addRaw(GSkinSurface* surface);
        void addRaw(GBorderSurface*  surface);
      
    public:
        // via meshlib
        GMeshLib*          getMeshLib();  // unplaced meshes
        unsigned           getNumMeshes();
        GItemIndex*        getMeshIndex(); 
        GMesh*             getMesh(unsigned index);  
        void               add(GMesh*    mesh);
    public:
        void dumpRaw(const char* msg="GGeo::dumpRaw");
        void dumpRawMaterialProperties(const char* msg="GGeo::dumpRawMaterialProperties");
        void dumpRawSkinSurface(const char* name=NULL);
        void dumpRawBorderSurface(const char* name=NULL);
    public:
        void traverse(const char* msg="GGeo::traverse");
    private:
        void traverse(GNode* node, unsigned int depth);
 
    public:
        unsigned int getNumMaterials();
        unsigned int getNumSkinSurfaces();
        unsigned int getNumBorderSurfaces();
    public:
        unsigned int getNumRawMaterials();
        unsigned int getNumRawSkinSurfaces();
        unsigned int getNumRawBorderSurfaces();
    public:
        GScene*            getScene();
        GNodeLib*          getNodeLib();
        GMaterialLib*      getMaterialLib();
        GSurfaceLib*       getSurfaceLib();

        GPmtLib*           getPmtLib(); 
        NLookup*           getLookup(); 
    public:
        void  setLookup(NLookup* lookup);
    public:
        GColorizer*        getColorizer();
        OpticksColors*     getColors();
        OpticksFlags*      getFlags(); 
        OpticksResource*   getResource();
        OpticksAttrSeq*    getFlagNames(); 
        Opticks*           getOpticks();
    public:
        GMaterial* getMaterial(unsigned int index);  
        GSkinSurface* getSkinSurface(unsigned int index);  
        GBorderSurface* getBorderSurface(unsigned int index);  
    public:
        void findScintillatorMaterials(const char* props);
        void dumpScintillatorMaterials(const char* msg="GGeo::dumpScintillatorMaterials");
        unsigned int getNumScintillatorMaterials();
        GMaterial* getScintillatorMaterial(unsigned int index);
    public:
        void findCathodeMaterials(const char* props);
        void dumpCathodeMaterials(const char* msg="GGeo::dumpCathodeMaterials");
        unsigned int getNumCathodeMaterials();
        GMaterial* getCathodeMaterial(unsigned int index);
    public:
        std::vector<GMaterial*> getRawMaterialsWithProperties(const char* props, const char* delim);
    public:
        GPropertyMap<float>* findRawMaterial(const char* shortname);
        GProperty<float>*    findRawMaterialProperty(const char* shortname, const char* propname);
    public:

    public:
        gfloat3* getLow();
        gfloat3* getHigh();
        void setLow(const gfloat3& low);
        void setHigh(const gfloat3& high);
        void updateBounds(GNode* node); 

    public:
        void addCathodeLV(const char* lv);
        void dumpCathodeLV(const char* msg="GGeo::dumpCathodeLV");
        const char* getCathodeLV(unsigned int index);
        unsigned int getNumCathodeLV();
    public:
        GSkinSurface* findSkinSurface(const char* lv);  
        GBorderSurface* findBorderSurface(const char* pv1, const char* pv2);  

    public:
        std::map<unsigned int, unsigned int>& getMeshUsage();
        std::map<unsigned int, std::vector<unsigned int> >& getMeshNodes();
        void countMeshUsage(unsigned int meshIndex, unsigned int nodeIndex, const char* lv, const char* pv);
        void reportMeshUsage(const char* msg="GGeo::reportMeshUsage");

#if 0
    TODO: see if this can be reinstated
    public:
        void materialConsistencyCheck();
        unsigned int materialConsistencyCheck(GSolid* solid);
#endif

    public:
        void Summary(const char* msg="GGeo::Summary");
        void Details(const char* msg="GGeo::Details");

    public:
        GTreeCheck* getTreeCheck();
    public:
        void setPickFace(std::string pickface);
        void setPickFace(const glm::ivec4& pickface);
        void setFaceTarget(unsigned int face_index, unsigned int solid_index, unsigned int mesh_index);
        void setFaceRangeTarget(unsigned int face_index0, unsigned int face_index1, unsigned int solid_index, unsigned int mesh_index);
        glm::ivec4& getPickFace(); 
    private:
        Opticks*                      m_ok ;  
        bool                          m_analytic ; 
        int                           m_gltf ; 
        Composition*                  m_composition ; 
        GTreeCheck*                   m_treecheck ; 
        bool                          m_loaded ;  



        std::vector<GMaterial*>       m_materials ; 
        std::vector<GSkinSurface*>    m_skin_surfaces ; 
        std::vector<GBorderSurface*>  m_border_surfaces ; 

        std::vector<GSolid*>           m_sensitive_solids ; 
        std::unordered_set<std::string> m_cathode_lv ; 

        // _raw mainly for debug
        std::vector<GMaterial*>       m_materials_raw ; 
        std::vector<GSkinSurface*>    m_skin_surfaces_raw ; 
        std::vector<GBorderSurface*>  m_border_surfaces_raw ; 

        std::vector<GMaterial*>       m_scintillators_raw ; 
        std::vector<GMaterial*>       m_cathodes_raw ; 

        NLookup*                      m_lookup ; 

        GMeshLib*                     m_meshlib ; 
        GGeoLib*                      m_geolib ; 

        GNodeLib*                     m_nodelib ; 

        GBndLib*                      m_bndlib ; 
        GMaterialLib*                 m_materiallib ; 
        GSurfaceLib*                  m_surfacelib ; 
        GScintillatorLib*             m_scintillatorlib ; 
        GSourceLib*                   m_sourcelib ; 
        GPmtLib*                      m_pmtlib ; 

        GColorizer*                   m_colorizer ; 

        gfloat3*                      m_low ; 
        gfloat3*                      m_high ; 

        // maybe into GGeoLib ? 

        std::map<unsigned int, unsigned int>                  m_mesh_usage ; 
        std::map<unsigned int, std::vector<unsigned int> >    m_mesh_nodes ; 

    private:

        Index_t                            m_index ; 
        unsigned int                       m_sensitive_count ;  
        GMaterial*                         m_cathode ; 
        const char*                        m_join_cfg ; 
        GJoinImpFunctionPtr                m_join_imp ;  
        GLoaderImpFunctionPtr              m_loader_imp ;  
        unsigned int                       m_mesh_verbosity ; 

    private:
        // glTF route 
        GScene*                            m_gscene ; 



};

#include "GGEO_TAIL.hh"


