#pragma once

#include "stdlib.h"
#include "stdio.h"
#include "assert.h"
#include "string.h"
#include <vector>



class NumpyEvt ; 
class MultiViewNPY ;

// ggeo-
class GDrawable ;
class GMergedMesh ;
class GGeo ;
class GBoundaryLibMetadata ;
class GBuffer ; 

// oglrap-
class DynamicDefine ; 
class Renderer ; 
class Rdr ;
class Device ; 
class Composition ; 
class Photons ; 
class Colors ; 


#include "Configurable.hh"


class Scene : public Configurable {

   public:
        static const char* PHOTON ;
        static const char* AXIS ;
        static const char* GENSTEP ;
        static const char* GLOBAL ;
        static const char* RECORD ;
   public:
        static const char* TARGET ;
   public:
        static const char* REC_ ; 
        static const char* ALTREC_ ; 
        static const char* DEVREC_ ; 
   public:
        static const char* NORM_ ; 
        static const char* BBOX_ ; 
        static const char* NORM_BBOX_ ; 
   public:
        enum { MAX_INSTANCE_RENDERER = 5 };  
        static const char* INSTANCE0  ;
        static const char* INSTANCE1  ;
        static const char* INSTANCE2  ;
        static const char* INSTANCE3  ;
        static const char* INSTANCE4  ;
        static const char* BBOX0  ;
        static const char* BBOX1  ;
        static const char* BBOX2  ;
        static const char* BBOX3  ;
        static const char* BBOX4  ;
   public:
        typedef enum { REC, ALTREC, DEVREC, NUM_RECORD_STYLE } RecordStyle_t ;
        void setRecordStyle(Scene::RecordStyle_t style);
        Scene::RecordStyle_t getRecordStyle();
        static const char* getRecordStyleName(Scene::RecordStyle_t style);
        const char* getRecordStyleName();
        void nextPhotonStyle();
   public:
        // disabled styles after NUM_GEOMETRY_STYLE
        typedef enum { BBOX, NORM, NUM_GEOMETRY_STYLE, NORM_BBOX } GeometryStyle_t ;
        void setGeometryStyle(Scene::GeometryStyle_t style);
        void applyGeometryStyle();
        static const char* getGeometryStyleName(Scene::GeometryStyle_t style);
        const char* getGeometryStyleName();
        void nextGeometryStyle();
        void toggleGeometry();
   public:
        Scene(const char* shader_dir=NULL, const char* shader_incl_path=NULL, const char* shader_dynamic_dir=NULL );
   private:
        void init();
   public:
        void write(DynamicDefine* dd);
        void gui();
        void initRenderers();
   public:
        // Configurable
        std::vector<std::string> getTags();
        void set(const char* name, std::string& xyz);
        std::string get(const char* name);

   public:
        static bool accepts(const char* name);
        void configure(const char* name, const char* value_);
        void configure(const char* name, int value);

   public:
        void configureI(const char* name, std::vector<int> values);
        void setComposition(Composition* composition);
        void setNumpyEvt(NumpyEvt* evt);
        void setPhotons(Photons* photons);
   public:
        void setGeometry(GGeo* gg);
        void uploadGeometry(); 
   public:
        void uploadColorBuffer(GBuffer* colorbuffer);
   public:
        // target cannot live in Composition, as needs geometry 
        // to convert solid index into CenterExtent to give to Composition
        //
        void setTarget(unsigned int index=0, bool autocam=false); 
        unsigned int touch(int ix, int iy, float depth);
        void setTouch(unsigned int index); 
        unsigned int getTouch();
        void jump(); 

   public:
        void uploadEvt();
        void uploadAxis();
        void uploadSelection();

   private:
        void uploadRecordAttr(MultiViewNPY* attr);
   public:
        void render();
   public:
        unsigned int  getTarget(); 


   public:
        Renderer*     getGeometryRenderer();
        Renderer*     getInstanceRenderer(unsigned int i);
        unsigned int  getNumInstanceRenderer();
   public:
        Rdr*          getAxisRenderer();
        Rdr*          getGenstepRenderer();
        Rdr*          getPhotonRenderer();
        Rdr*          getRecordRenderer();
        Rdr*          getRecordRenderer(RecordStyle_t style);
        //GMergedMesh*  getGeometry();
        Composition*  getComposition();
        NumpyEvt*     getNumpyEvt();
        Photons*      getPhotons();
        bool*         getModeAddress(const char* name);
        const char*   getRecordTag();
        float         getTimeFraction();

   private:
        char*        m_shader_dir ; 
        char*        m_shader_dynamic_dir ; 
        char*        m_shader_incl_path ; 
        Device*      m_device ; 
        Colors*      m_colors ; 
   private:
        unsigned int m_num_instance_renderer ; 
        Renderer*    m_geometry_renderer ; 
        Renderer*    m_instance_renderer[MAX_INSTANCE_RENDERER] ; 
        Renderer*    m_bbox_renderer[MAX_INSTANCE_RENDERER] ; 
        Renderer*    m_global_renderer ; 
   private:
        Rdr*         m_axis_renderer ; 
        Rdr*         m_genstep_renderer ; 
        Rdr*         m_photon_renderer ; 
        Rdr*         m_record_renderer ; 
        Rdr*         m_altrecord_renderer ; 
        Rdr*         m_devrecord_renderer ; 
   private:
        NumpyEvt*    m_evt ;
        Photons*     m_photons ; 
        GGeo*        m_ggeo ;
        GMergedMesh* m_mesh0 ; 
        Composition* m_composition ;
        GBuffer*     m_colorbuffer ;
        unsigned int m_target ;
        unsigned int m_touch ;

   private:
        bool         m_global_mode ; 
        bool         m_instance_mode[MAX_INSTANCE_RENDERER] ; 
        bool         m_bbox_mode[MAX_INSTANCE_RENDERER] ; 
        bool         m_axis_mode ; 
        bool         m_genstep_mode ; 
        bool         m_photon_mode ; 
        bool         m_record_mode ; 
   private:
        RecordStyle_t   m_record_style ; 
        GeometryStyle_t m_geometry_style ; 
        bool            m_initialized ;  
        float           m_time_fraction ;  


};


inline Scene::Scene(const char* shader_dir, const char* shader_incl_path, const char* shader_dynamic_dir) 
            :
            m_shader_dir(shader_dir ? strdup(shader_dir): NULL ),
            m_shader_dynamic_dir(shader_dynamic_dir ? strdup(shader_dynamic_dir): NULL),
            m_shader_incl_path(shader_incl_path ? strdup(shader_incl_path): NULL),
            m_device(NULL),
            m_colors(NULL),
            m_num_instance_renderer(0),
            m_geometry_renderer(NULL),
            m_global_renderer(NULL),
            m_axis_renderer(NULL),
            m_genstep_renderer(NULL),
            m_photon_renderer(NULL),
            m_record_renderer(NULL),
            m_altrecord_renderer(NULL),
            m_devrecord_renderer(NULL),
            m_evt(NULL),
            m_photons(NULL),
            m_ggeo(NULL),
            m_mesh0(NULL),
            m_composition(NULL),
            m_colorbuffer(NULL),
            m_target(0),
            m_touch(0),
            m_global_mode(false),
            m_axis_mode(true),
            m_genstep_mode(true),
            m_photon_mode(true),
            m_record_mode(true),
            m_record_style(REC),
            m_geometry_style(BBOX),
            m_initialized(false),
            m_time_fraction(0.f)
{

    init();

    for(unsigned int i=0 ; i < MAX_INSTANCE_RENDERER ; i++ ) 
    {
        m_instance_renderer[i] = NULL ; 
        m_bbox_renderer[i] = NULL ; 
        m_instance_mode[i] = false ; 
        m_bbox_mode[i] = false ; 
    }
}




inline void Scene::setGeometry(GGeo* gg)
{
    m_ggeo = gg ;
}
inline unsigned int Scene::getNumInstanceRenderer()
{
    return m_num_instance_renderer ; 
}

inline float Scene::getTimeFraction()
{
    return m_time_fraction ; 
}

inline unsigned int Scene::getTarget()
{
    return m_target ;
}
inline unsigned int Scene::getTouch()
{
    return m_touch ;
}
inline void Scene::setTouch(unsigned int touch)
{
    m_touch = touch ; 
}



inline Renderer* Scene::getGeometryRenderer()
{
    return m_geometry_renderer ; 
}


inline Rdr* Scene::getAxisRenderer()
{
    return m_axis_renderer ; 
}
inline Rdr* Scene::getGenstepRenderer()
{
    return m_genstep_renderer ; 
}
inline Rdr* Scene::getPhotonRenderer()
{
    return m_photon_renderer ; 
}



inline NumpyEvt* Scene::getNumpyEvt()
{
    return m_evt ; 
}
inline Photons* Scene::getPhotons()
{
    return m_photons ; 
}




//inline GMergedMesh* Scene::getGeometry()
//{
//    return m_geometry ; 
//}


inline void Scene::setNumpyEvt(NumpyEvt* evt)
{
    m_evt = evt ; 
}
inline void Scene::setPhotons(Photons* photons)
{
    m_photons = photons ; 
}




inline void Scene::setRecordStyle(RecordStyle_t style)
{
    m_record_style = style ; 
}

inline Scene::RecordStyle_t Scene::getRecordStyle()
{
    return m_record_style ; 
}







inline void Scene::nextPhotonStyle()
{
    int next = (m_record_style + 1) % NUM_RECORD_STYLE ; 
    m_record_style = (RecordStyle_t)next ; 
}



inline void Scene::toggleGeometry()
{
    m_global_mode = !m_global_mode ; 
}

inline void Scene::nextGeometryStyle()
{
    int next = (m_geometry_style + 1) % NUM_GEOMETRY_STYLE ; 
    setGeometryStyle( (GeometryStyle_t)next );

    const char* stylename = getGeometryStyleName();
    printf("Scene::nextGeometryStyle : %s \n", stylename);
}

inline void Scene::setGeometryStyle(GeometryStyle_t style)
{
    m_geometry_style = style ; 
    applyGeometryStyle();
}

inline void Scene::applyGeometryStyle()
{
    bool imode ; 
    bool bmode ; 

    switch(m_geometry_style)
    {
      case BBOX:
             imode = false ; 
             bmode = true ; 
             break;
      case NORM:
             imode = true ;
             bmode = false ; 
             break;
      case NORM_BBOX:
             imode = true ; 
             bmode = true ; 
             break;
      case NUM_GEOMETRY_STYLE:
             assert(0);
             break;
   }

   for(unsigned int i=0 ; i < m_num_instance_renderer ; i++ ) 
   {
       m_instance_mode[i] = imode ; 
       m_bbox_mode[i] = bmode ; 
   }
}





