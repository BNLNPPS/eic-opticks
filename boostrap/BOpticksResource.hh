#pragma once

#include <vector>
#include <map>
#include <string>

#include "BRAP_API_EXPORT.hh"
#include "BRAP_HEAD.hh"

class SLog ; 

class BOpticksKey ; 
class BPath ; 
class BResource ; 


class BRAP_API  BOpticksResource {
       
    private:
       static const char* G4ENV_RELPATH ; 
       static const char* OKDATA_RELPATH ;
    protected:
       static const char* InstallPathOKDATA() ;
       static const char* InstallPathG4ENV() ;
       static const char* InstallPath(const char* relpath) ;
   public:
       static const char* MakeSrcPath(const char* srcpath, const char* ext) ;
   public:
        BOpticksResource(const char* envprefix="OPTICKS_");
        virtual ~BOpticksResource();
        virtual void Summary(const char* msg="BOpticksResource::Summary");

        static std::string BuildDir(const char* proj);
        static std::string BuildProduct(const char* proj, const char* name);
        static std::string PTXPath(const char* name, const char* target="OptiXRap");

        static const char* OpticksDataDir();
        static const char* GeoCacheDir();
        static const char* ResourceDir();
        static const char* GenstepsDir();
        static const char* InstallCacheDir();
        static const char* PTXInstallPath();
        static const char* RNGInstallPath();
        static const char* OKCInstallPath();
   private:
        static std::string PTXPath(const char* name, const char* target, const char* prefix);
        static std::string PTXName(const char* name, const char* target);
        static const char* makeInstallPath( const char* prefix, const char* main, const char* sub );
   public:       
        const char* getInstallPrefix();
        const char* getInstallDir();

        const char* getOpticksDataDir();
        const char* getGeoCacheDir();
        const char* getInstallCacheDir();
        const char* getResourceDir();
        const char* getGenstepsDir();

        const char* getRNGInstallCacheDir();
        const char* getOKCInstallCacheDir();
        const char* getPTXInstallCacheDir();

        const char* getDebuggingTreedir(int argc, char** argv);
        std::string getPTXPath(const char* name, const char* target="OptiXRap");
   public:       
        const char* getDebuggingIDPATH();
        const char* getDebuggingIDFOLD();
   public:       
       std::string getIdPathPath(const char* rela, const char* relb=NULL, const char* relc=NULL, const char* reld=NULL ) const ; 
       std::string getGeoCachePath(const char* rela, const char* relb=NULL, const char* relc=NULL, const char* reld=NULL) const ;
       std::string getPropertyLibDir(const char* name) const ;
       std::string getInstallPath(const char* relpath) const ;
       const char* getIdPath() const ;
       const char* getIdFold() const ;  // parent directory of idpath containing g4_00.dae
       void setIdPathOverride(const char* idpath_tmp=NULL);  // used for test saves into non-standard locations

    public:
       const char* getSrcPath() const ;
       const char* getSrcDigest() const ;
       const char* getDAEPath() const ;
       const char* getGDMLPath() const ;
       const char* getGLTFPath() const ;
       const char* getMetaPath() const ;
       const char* getIdMapPath() const ;
    public:
       const char* getGLTFBase() const ;
       const char* getGLTFName() const ;
  private:
        void init();
        void initInstallPrefix();
        void initTopDownDirs();
        void initDebuggingIDPATH(); 
   protected:
        friend struct NSensorListTest ; 
        friend struct NSceneTest ; 
        friend struct HitsNPYTest ; 
        friend struct BOpticksResourceTest ; 
        // only use one setup route
        void setupViaSrc(const char* srcpath, const char* srcdigest);
        void setupViaID(const char* idpath );
        void setupViaKey();

        // unfortunately having 2 routes difficult to avoid, as IDPATH is 
        // more convenient in that a single path yields everything, whereas
        // the OpticksResource geokey stuff needs to go via Src
   private:
        void setSrcPath(const char* srcpath);
        void setSrcDigest(const char* srcdigest);
   protected:
        SLog*        m_log ; 
        bool         m_setup ; 
        BOpticksKey* m_key ; 
        BPath*       m_id ; 
        BResource*   m_res ;
 
        const char* m_envprefix ; 
        int         m_layout ; 
        const char* m_install_prefix ;   // from BOpticksResourceCMakeConfig header
        const char* m_opticksdata_dir ; 
        const char* m_geocache_dir ; 
        const char* m_resource_dir ; 
        const char* m_gensteps_dir ; 
        const char* m_installcache_dir ; 
        const char* m_rng_installcache_dir ; 
        const char* m_okc_installcache_dir ; 
        const char* m_ptx_installcache_dir ; 
   protected:
        const char* m_srcpath ; 
        const char* m_srcfold ; 
        const char* m_srcbase ; 
        const char* m_srcdigest ; 
        const char* m_idfold ; 
        const char* m_idfile ; 
        const char* m_idname ; 
        const char* m_idpath ; 
        const char* m_idpath_tmp ; 
   protected:
        const char* m_debugging_idpath ; 
        const char* m_debugging_idfold ; 
   protected:
       const char* m_daepath ;
       const char* m_gdmlpath ;
       const char* m_gltfpath ;
       const char* m_metapath ;
       const char* m_idmappath ;
};

#include "BRAP_TAIL.hh"

