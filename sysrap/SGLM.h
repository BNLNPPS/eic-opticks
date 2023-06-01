#pragma once
/**
SGLM : Header Only OpenGL viz projection maths 
=================================================

This is the starting point for a simple single header 
replacement of a boatload of classes used by okc. 

* Composition, View, Camera, ...

Using this aims to enable CSGOptiX to drop dependency
on okc, npy, brap and instead depend only on QUDARap, SysRap, CSG. 

* Note that animated interpolation between views available 
  in the old machinery is not yet implemented in the new workflow
  as that currently focusses on snap renders. 

* because of this incomplete nature have to keep SGLM reliance (not usage) 
  behind WITH_SGLM and retain the old Composition based version of CSGOptiX (and okc dependency) 

* note that the with Composition version of CSGOptiX also uses SGLM 
  to allow easy comparison of frames and matrices between the two approaches

* ALSO this needs to be tested by comparing rasterized and ray-traced renders
  in order to achieve full consistency. Usage with interactive changing of camera and view etc.. 
  and interactive flipping between rasterized and ray traced is the way consistency 
  of the projective and ray traced maths was achieved for okc/Composition. 

Normal inputs WH, CE, EYE, LOOK, UP are held in static variables with envvar defaults 
These can be changed with static methods before instanciating SGLM. 
NB it will usually be too late for setenv in code to influence SGLM 
as the static initialization would have happened already 
 
* https://learnopengl.com/Getting-started/Camera

TODO: WASD camera navigation, using a method intended to be called from the GLFW key callback 


TODO: provide persistency into ~16 quad4 for debugging view/cam/projection state 
      would be best to group that for clarity 


* hmm probably using nested structs makes sense, or just use SGLM namespace with 
* https://riptutorial.com/cplusplus/example/11914/nested-classes-structures
* https://www.geeksforgeeks.org/nested-structure-in-c-with-examples/

**/

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "scuda.h"
#include "sqat4.h"
#include "SCenterExtentFrame.h"
#include "SCAM.h"
#include "SBAS.h"
#include "sframe.h"

#include "SYSRAP_API_EXPORT.hh"


struct SYSRAP_API SGLM
{
    static SGLM* INSTANCE ; 
    static constexpr const char* kWH = "WH" ; 
    static constexpr const char* kCE = "CE" ; 
    static constexpr const char* kEYE = "EYE" ; 
    static constexpr const char* kLOOK = "LOOK" ; 
    static constexpr const char* kUP = "UP" ; 
    static constexpr const char* kZOOM = "ZOOM" ; 
    static constexpr const char* kTMIN = "TMIN" ; 
    static constexpr const char* kCAM = "CAM" ; 
    static constexpr const char* kNEARFAR = "NEARFAR" ; 
    static constexpr const char* kFOCAL = "FOCAL" ; 

    // static defaults, some can be overridden in the instance 
    static glm::ivec2 WH ; 

    static glm::vec4 CE ;    
    static glm::vec4 EYE ; 
    static glm::vec4 LOOK ; 
    static glm::vec4 UP ; 

    static float ZOOM ; 
    static float TMIN ; 
    static int   CAM ; 
    static int   NEARFAR ; 
    static int   FOCAL ; 

    // ~1.5 quad4

    static void SetWH( int width, int height ); 
    static void SetCE( float x, float y, float z, float extent ); 
    static void SetEYE( float x, float y, float z ); 
    static void SetLOOK( float x, float y, float z ); 
    static void SetUP( float x, float y, float z ); 
    static void SetZOOM( float v ); 
    static void SetTMIN( float v ); 
    static void SetCAM( const char* cam ); 
    static void SetNEARFAR( const char* nearfar ); 
    static void SetFOCAL( const char* focal ); 

    // querying of static defaults 
    static std::string DescInput() ; 
    static int Width() ; 
    static int Height() ; 
    static float Aspect(); 
    static const char* CAM_Label() ; 
    static const char* NEARFAR_Label() ; 
    static const char* FOCAL_Label() ; 

    template <typename T> static T ato_( const char* a );
    template <typename T> static void GetEVector(std::vector<T>& vec, const char* key, const char* fallback );
    template <typename T> static std::string Present(std::vector<T>& vec);

    static std::string Present(const glm::ivec2& v, int wid=6 );
    static std::string Present(const float v, int wid=10, int prec=3);
    static std::string Present(const glm::vec3& v, int wid=10, int prec=3);
    static std::string Present(const glm::vec4& v, int wid=10, int prec=3);
    static std::string Present(const float4& v,    int wid=10, int prec=3);
    static std::string Present(const glm::mat4& m, int wid=10, int prec=3);

    template<typename T> static std::string Present_(const glm::tmat4x4<T>& m, int wid=10, int prec=3);

    static void GetEVec(glm::vec3& v, const char* key, const char* fallback );
    static void GetEVec(glm::vec4& v, const char* key, const char* fallback );

    template <typename T> static T EValue(const char* key, const char* fallback );
    static glm::ivec2 EVec2i(const char* key, const char* fallback); 
    static glm::vec3 EVec3(const char* key, const char* fallback); 
    static glm::vec4 EVec4(const char* key, const char* fallback, float missing=1.f ); 

    template<typename T> static glm::tmat4x4<T> DemoMatrix(T scale); 


    // member methods
    std::string descInput() const ; 

    SGLM(); 
    void initCE(); 

    sframe fr ;    // CAUTION: SEvt also holds an SFrame for input photon targetting 
    //glm::vec4 ce ;

    // modes
    int  cam ; 
    int  nearfar ;
    int  focal ;  

    // manual input matrices
    //const qat4* m2w ; 
    //const qat4* w2m ; 
    bool rtp_tangential ; 

    // ~1.5+3 quad4

 
    // derived matrices depending on ce 
    glm::mat4 model2world ; 
    glm::mat4 world2model ; 
    int updateModelMatrix_branch ; 

    float nearfar_manual ; 
    float focal_manual ; 
    float near ; 
    float far ; 

    // ~1.5+3+3 quad4

    std::vector<std::string> log ; 

    float tmin_abs() const ; 
    void set_frame( const sframe& fr ); 
    //void set_ce(  float x, float y, float z, float w ); 
    //void set_m2w( const qat4* m2w_, const qat4* w2m_ ); 
    void set_rtp_tangential( bool rtp_tangential_ ); 

    void updateModelMatrix();  // depends on ce, unless valid m2w and w2m matrices provided

    void set_near( float near_ ); 
    void set_far( float far_ ); 
    void set_near_abs( float near_abs_ ); 
    void set_far_abs(  float far_abs_ ); 

    float get_near() const ;  
    float get_far() const ;  
    float get_near_abs() const ;  
    float get_far_abs() const ;  

    std::string descBasis() const ; 

    // world frame View converted from static model frame
    glm::vec3 eye ;  
    glm::vec3 look ; 
    glm::vec3 up ; 
    glm::vec3 gaze ; 

    // 1.5 +3+3+1

    void updateELU();   // depends on CE and EYE, LOOK, UP 
    std::string descELU() const ; 

    // results from updateEyeSpace

    glm::vec3 forward_ax ; 
    glm::vec3 right_ax ; 
    glm::vec3 top_ax ; 
    glm::mat4 rot_ax ;  
    glm::mat4 world2camera ; 
    glm::mat4 camera2world ; 

    // 1.5 +3+3+1+4   ~13 quad4 



    void updateEyeSpace(); 
    std::string descEyeSpace() const ; 

    // results from updateEyeBasis
    glm::vec3 u ; 
    glm::vec3 v ; 
    glm::vec3 w ; 
    glm::vec3 e ; 
    void updateEyeBasis(); 
    static std::string DescEyeBasis( const glm::vec3& E, const glm::vec3& U, const glm::vec3& V, const glm::vec3& W ); 
    std::string descEyeBasis() const ; 


    glm::mat4 projection ; 
    glm::mat4 world2clip ; 

    // ~16 quad4 

    float getGazeLength() const ; 

    void updateProjection(); 
    std::string descProjection() const ; 


    void set_nearfar_mode(const char* mode); 
    void set_focal_mode(const char* mode); 

    const char* get_nearfar_mode() const ; 
    const char* get_focal_mode() const ; 

    void set_nearfar_manual(float nearfar_manual_); 
    void set_focal_manual(float focal_manual_); 

    float get_nearfar_basis() const ; 
    float get_focal_basis() const ; 


    std::string desc() const ; 
    std::string descLog() const ; 

    void addlog( const char* label, float value       ) ; 
    void addlog( const char* label, const char* value ) ; 
    void dump() const ; 
    void update(); 
};

SGLM* SGLM::INSTANCE = nullptr ; 

glm::ivec2 SGLM::WH = EVec2i(kWH,"1920,1080") ; 
glm::vec4  SGLM::CE = EVec4(kCE,"0,0,0,100") ; 
glm::vec4  SGLM::EYE  = EVec4(kEYE, "-1,-1,0,1") ; 
glm::vec4  SGLM::LOOK = EVec4(kLOOK, "0,0,0,1") ; 
glm::vec4  SGLM::UP  =  EVec4(kUP,   "0,0,1,0") ; 
float      SGLM::ZOOM = EValue<float>(kZOOM, "1"); 
float      SGLM::TMIN = EValue<float>(kTMIN, "0.1"); 
int        SGLM::CAM  = SCAM::EGet(kCAM, "perspective") ; 
int        SGLM::NEARFAR = SBAS::EGet(kNEARFAR, "gazelength") ; 
int        SGLM::FOCAL   = SBAS::EGet(kFOCAL,   "gazelength") ; 

void SGLM::SetWH( int width, int height ){ WH.x = width ; WH.y = height ; }
void SGLM::SetCE(  float x, float y, float z, float w){ CE.x = x ; CE.y = y ; CE.z = z ;  CE.w = w ; }
void SGLM::SetEYE( float x, float y, float z){ EYE.x = x  ; EYE.y = y  ; EYE.z = z  ;  EYE.w = 1.f ; }
void SGLM::SetLOOK(float x, float y, float z){ LOOK.x = x ; LOOK.y = y ; LOOK.z = z ;  LOOK.w = 1.f ; }
void SGLM::SetUP(  float x, float y, float z){ UP.x = x   ; UP.y = y   ; UP.z = z   ;  UP.w = 1.f ; }
void SGLM::SetZOOM( float v ){ ZOOM = v ; }
void SGLM::SetTMIN( float v ){ TMIN = v ; }
void SGLM::SetCAM( const char* cam ){ CAM = SCAM::Type(cam) ; }
void SGLM::SetNEARFAR( const char* nearfar ){ NEARFAR = SBAS::Type(nearfar) ; }
void SGLM::SetFOCAL( const char* focal ){ FOCAL = SBAS::Type(focal) ; }

int SGLM::Width(){  return WH.x ; }
int SGLM::Height(){ return WH.y ; }
float SGLM::Aspect() { return float(WH.x)/float(WH.y) ; } 
const char* SGLM::CAM_Label(){ return SCAM::Name(CAM) ; }
const char* SGLM::NEARFAR_Label(){ return SBAS::Name(NEARFAR) ; }
const char* SGLM::FOCAL_Label(){   return SBAS::Name(FOCAL) ; }

SGLM::SGLM() 
    :
    cam(CAM),
    nearfar(NEARFAR),
    focal(FOCAL),
    //m2w(nullptr),
    //w2m(nullptr),
    rtp_tangential(false),
    model2world(1.f), 
    world2model(1.f),
    updateModelMatrix_branch(-1), 

    nearfar_manual(0.f),
    focal_manual(0.f),

    near(0.1f),   // units of get_nearfar_basis
    far(5.f),     // units of get_nearfar_basis

    forward_ax(0.f,0.f,0.f),
    right_ax(0.f,0.f,0.f),
    top_ax(0.f,0.f,0.f),
    rot_ax(1.f),
    projection(1.f),
    world2clip(1.f)
{
    initCE(); 
    addlog("SGLM::SGLM", "ctor"); 
    update(); 
    INSTANCE = this ; 
}

void SGLM::initCE()
{
    fr.ce.x = CE.x ; 
    fr.ce.y = CE.y ; 
    fr.ce.z = CE.z ; 
    fr.ce.w = CE.w ; 
}

void SGLM::update()  
{
    addlog("SGLM::update", "["); 
    updateModelMatrix(); 
    updateELU(); 
    updateEyeSpace(); 
    updateEyeBasis(); 
    updateProjection(); 
    addlog("SGLM::update", "]"); 
}

std::string SGLM::desc() const 
{
    std::stringstream ss ; 
    ss << DescInput() << std::endl ; 
    ss << descInput() << std::endl ; 
    ss << descELU() << std::endl ; 
    ss << descEyeSpace() << std::endl ; 
    ss << descEyeBasis() << std::endl ; 
    ss << descProjection() << std::endl ; 
    ss << descBasis() << std::endl ; 
    std::string s = ss.str(); 
    return s ; 
}

void SGLM::dump() const 
{
    std::cout << desc() << std::endl ; 
}

std::string SGLM::DescInput() // static
{
    std::stringstream ss ; 
    ss << "SGLM::DescInput" << std::endl ; 
    ss << std::setw(15) << "SGLM::CAM"  << " " << SGLM::CAM << std::endl ; 
    ss << std::setw(15) << kCAM << " " << CAM_Label() << std::endl ; 
    ss << std::setw(15) << kNEARFAR << " " << NEARFAR_Label() << std::endl ; 
    ss << std::setw(15) << kFOCAL   << " " << FOCAL_Label() << std::endl ; 
    ss << std::setw(15) << kWH    << Present( WH )   << " Aspect " << Aspect() << std::endl ; 
    ss << std::setw(15) << kCE    << Present( CE )   << std::endl ; 
    ss << std::setw(15) << kEYE   << Present( EYE )  << std::endl ; 
    ss << std::setw(15) << kLOOK  << Present( LOOK ) << std::endl ; 
    ss << std::setw(15) << kUP    << Present( UP )   << std::endl ; 
    ss << std::setw(15) << kZOOM  << Present( ZOOM ) << std::endl ; 
    ss << std::endl ; 
    std::string s = ss.str(); 
    return s ; 
}

std::string SGLM::descInput() const 
{
    std::stringstream ss ; 
    ss << "SGLM::descInput" << std::endl ; 
    ss << std::setw(25) << " sglm.fr.ce "  << Present( fr.ce )   << std::endl ; 
    ss << std::setw(25) << " sglm.cam " << cam << std::endl ; 
    ss << std::setw(25) << " SCAM::Name(sglm.cam) " << SCAM::Name(cam) << std::endl ; 
    std::string s = ss.str(); 
    return s ; 
}

float SGLM::tmin_abs() const
{
    // TMIN: is "tmin_model" from TMIN envvar with default of 0.1 (units of extent) 
    return fr.ce.w*TMIN ; 
}

void SGLM::set_frame( const sframe& fr_ )
{
    fr = fr_ ; 
    //set_ce(fr.ce.x, fr.ce.y, fr.ce.z, fr.ce.w ); 
    //set_m2w(fr.m2w, fr.w2m);
    update();  
    set_near_abs(tmin_abs()) ;
    update();
}

/*
void SGLM::set_ce( float x, float y, float z, float w )
{
    ce.x = x ; 
    ce.y = y ; 
    ce.z = z ; 
    ce.w = w ; 
    addlog("set_ce", ce.w);
}
*/


void SGLM::set_rtp_tangential(bool rtp_tangential_ )
{
    rtp_tangential = rtp_tangential_ ; 
    addlog("set_rtp_tangential", rtp_tangential );
}

/*
void SGLM::set_m2w( const qat4* m2w_, const qat4* w2m_ )
{
    m2w = m2w_ ; 
    w2m = w2m_ ; 
    addlog("set_m2w", "and w2m");
}
*/


/**
SGLM::updateModelMatrix
------------------------

Called by SGLM::update. The matrices are set based on center--extent OR directly from the set_m2w matrices 

**/

void SGLM::updateModelMatrix()
{
    updateModelMatrix_branch = 0 ; 

    bool m2w_valid = fr.m2w.is_zero() == false ;
    bool w2m_valid = fr.w2m.is_zero() == false ;

    if( m2w_valid && w2m_valid )
    {
        updateModelMatrix_branch = 1 ; 
        model2world = glm::make_mat4x4<float>(fr.m2w.cdata());
        world2model = glm::make_mat4x4<float>(fr.w2m.cdata());
    }
    else if( rtp_tangential )
    {
        updateModelMatrix_branch = 2 ; 
        SCenterExtentFrame<double> cef( fr.ce.x, fr.ce.y, fr.ce.z, fr.ce.w, rtp_tangential );
        model2world = cef.model2world ;
        world2model = cef.world2model ;
    }
    else
    {
        updateModelMatrix_branch = 3 ; 
        glm::vec3 tr(fr.ce.x, fr.ce.y, fr.ce.z) ;  
        glm::vec3 sc(fr.ce.w, fr.ce.w, fr.ce.w) ; 
        glm::vec3 isc(1.f/fr.ce.w, 1.f/fr.ce.w, 1.f/fr.ce.w) ; 

        model2world = glm::scale(glm::translate(glm::mat4(1.0), tr), sc);
        world2model = glm::translate( glm::scale(glm::mat4(1.0), isc), -tr); 
    }
    addlog("updateModelMatrix", updateModelMatrix_branch );
}


void SGLM::addlog( const char* label, float value )
{
    std::stringstream ss ;  
    ss << std::setw(25) << label << " : " << std::setw(10) << std::fixed << std::setprecision(3) << value ;  
    std::string s = ss.str(); 
    log.push_back(s); 
}

void SGLM::addlog( const char* label, const char* value )
{
    std::stringstream ss ;  
    ss << std::setw(25) << label << " : " << value ;  
    std::string s = ss.str(); 
    log.push_back(s); 
}



void SGLM::set_nearfar_mode(const char* mode){    addlog("set_nearfar_mode",  mode) ; nearfar = SBAS::Type(mode) ; }
void SGLM::set_focal_mode(  const char* mode){    addlog("set_focal_mode",    mode) ; focal   = SBAS::Type(mode) ; }

const char* SGLM::get_nearfar_mode() const { return SBAS::Name(nearfar) ; } 
const char* SGLM::get_focal_mode() const {   return SBAS::Name(focal) ; } 

void SGLM::set_nearfar_manual(float nearfar_manual_ ){ addlog("set_nearfar_manual", nearfar_manual_ ) ; nearfar_manual = nearfar_manual_  ; }
void SGLM::set_focal_manual(float focal_manual_ ){     addlog("set_focal_manual", focal_manual_ )     ; focal_manual = focal_manual_  ; }


float SGLM::get_nearfar_basis() const 
{ 
    float basis = 0.f ; 
    switch(nearfar)
    {
        case BAS_MANUAL:     basis = nearfar_manual      ; break ; 
        case BAS_EXTENT:     basis = fr.ce.w             ; break ; 
        case BAS_GAZELENGTH: basis = getGazeLength()     ; break ;  // only available after updateELU
        case BAS_NEARABS:    assert(0)                   ; break ;  // this mode only valud for get_focal_basis 
    }
    return basis ;  
}

/**
SGLM::get_focal_basis
----------------------

BAS_NEARABS problematic as 

**/

float SGLM::get_focal_basis() const 
{ 
    float basis = 0.f ; 
    switch(focal)
    {
        case BAS_MANUAL:     basis = focal_manual        ; break ; 
        case BAS_EXTENT:     basis = fr.ce.w             ; break ; 
        case BAS_GAZELENGTH: basis = getGazeLength()     ; break ;  // only available after updateELU
        case BAS_NEARABS:    basis = get_near_abs()      ; break ; 
    }
    return basis ;  
}




void SGLM::set_near( float near_ ){ near = near_ ; addlog("set_near", near); }
void SGLM::set_far(  float far_ ){  far = far_   ; addlog("set_far", far);   }

float SGLM::get_near() const  { return near ; }  
float SGLM::get_far()  const  { return far  ; }  

// CAUTION: depends on get_nearfar_basis
void SGLM::set_near_abs( float near_abs_ ){ addlog("set_near_abs", near_abs_) ; set_near( near_abs_/get_nearfar_basis() ) ; }
void SGLM::set_far_abs(  float far_abs_ ){  addlog("set_far_abs", far_abs_)   ; set_far(  far_abs_/get_nearfar_basis()  ) ; }

float SGLM::get_near_abs() const { return near*get_nearfar_basis() ; }
float SGLM::get_far_abs() const { return   far*get_nearfar_basis() ; }


std::string SGLM::descBasis() const 
{
    int wid = 25 ; 
    std::stringstream ss ; 
    ss << "SGLM::descBasis" << std::endl ; 
    ss << std::setw(wid) << " sglm.get_nearfar_mode " << get_nearfar_mode()  << std::endl ; 
    ss << std::setw(wid) << " sglm.nearfar_manual "   << Present( nearfar_manual ) << std::endl ; 
    ss << std::setw(wid) << " sglm.fr.ce.w  "     << Present( fr.ce.w )  << std::endl ; 
    ss << std::setw(wid) << " sglm.getGazeLength  " << Present( getGazeLength() ) << std::endl ; 
    ss << std::setw(wid) << " sglm.get_nearfar_basis " << Present( get_nearfar_basis() ) << std::endl ; 
    ss << std::endl ; 
    ss << std::setw(wid) << " sglm.near  "     << Present( near )  << " (units of nearfar basis) " << std::endl ; 
    ss << std::setw(wid) << " sglm.far   "     << Present( far )   << " (units of nearfar basis) " << std::endl ; 
    ss << std::setw(wid) << " sglm.get_near_abs  " << Present( get_near_abs() ) << " near*get_nearfar_basis() " << std::endl ; 
    ss << std::setw(wid) << " sglm.get_far_abs  " << Present( get_far_abs() )   << " far*get_nearfar_basis() " << std::endl ; 
    ss << std::endl ; 
    ss << std::setw(wid) << " sglm.get_focal_mode " << get_focal_mode()  << std::endl ; 
    ss << std::setw(wid) << " sglm.get_focal_basis " << Present( get_focal_basis() ) << std::endl ; 
    ss << std::endl ; 
    std::string s = ss.str(); 
    return s ; 
}

void SGLM::updateELU() // eye, look, up, gaze into world frame 
{
    eye  = glm::vec3( model2world * EYE ) ; 
    look = glm::vec3( model2world * LOOK ) ; 
    up   = glm::vec3( model2world * UP ) ; 
    gaze = glm::vec3( model2world * (LOOK - EYE) ) ;    
}

std::string SGLM::descLog() const
{
    std::stringstream ss ; 
    for(unsigned i=0 ; i < log.size() ; i++) ss << log[i] << std::endl ; 
    std::string s = ss.str(); 
    return s ; 
}
 
std::string SGLM::descELU() const 
{
    std::stringstream ss ; 
    ss << "SGLM::descELU" << std::endl ; 
    ss << " sglm.model2world \n" << Present( model2world ) << std::endl ; 
    ss << " sglm.world2model \n" << Present( world2model ) << std::endl ; 
    ss << " sglm.updateModelMatrix_branch " << updateModelMatrix_branch << std::endl ; 
    ss << std::endl ; 
    ss << std::setw(15) << " sglm.EYE "  << Present( EYE )  << std::endl ; 
    ss << std::setw(15) << " sglm.LOOK " << Present( LOOK ) << std::endl ; 
    ss << std::setw(15) << " sglm.UP "   << Present( UP )   << std::endl ; 
    ss << std::setw(15) << " sglm.GAZE " << Present( LOOK-EYE ) << std::endl ; 
    ss << std::endl ; 
    ss << std::setw(15) << " sglm.eye "  << Present( eye )  << std::endl ; 
    ss << std::setw(15) << " sglm.look " << Present( look ) << std::endl ; 
    ss << std::setw(15) << " sglm.up "   << Present( up )   << std::endl ; 
    ss << std::setw(15) << " sglm.gaze " << Present( gaze ) << std::endl ; 
    ss << std::endl ; 
    std::string s = ss.str(); 
    return s ; 
}

/**
SGLM::updateEyeSpace
---------------------

Normalized eye space oriented via gaze and up directions.  

        +Y    -Z
     top_ax  forward_ax  
         |  /
         | /
         |/    
         +----- right_ax  (+X)
        .
       .
      .
    -forward_ax
    +Z 


world2camera
    first translates a world frame point to the eye point, 
    making eye point the origin then rotates to get into camera frame
    using the OpenGL eye space convention : -Z is forward, +X to right, +Y up

**/

void SGLM::updateEyeSpace()
{
    forward_ax = glm::normalize(gaze);
    right_ax   = glm::normalize(glm::cross(forward_ax,up)); 
    top_ax     = glm::normalize(glm::cross(right_ax,forward_ax));

    // OpenGL eye space convention : -Z is forward, +X to right, +Y up
    rot_ax[0] = glm::vec4( right_ax, 0.f );  
    rot_ax[1] = glm::vec4( top_ax  , 0.f );  
    rot_ax[2] = glm::vec4( -forward_ax, 0.f );  
    rot_ax[3] = glm::vec4( 0.f, 0.f, 0.f, 1.f ); 

    glm::mat4 ti(glm::translate(glm::vec3(eye)));  // origin to eye
    glm::mat4 t(glm::translate(glm::vec3(-eye)));  // eye to origin 

    world2camera = glm::transpose(rot_ax) * t  ;
    camera2world = ti * rot_ax ;
}

std::string SGLM::descEyeSpace() const 
{
    std::stringstream ss ; 
    ss << "SGLM::descEyeSpace" << std::endl ; 
    ss << std::setw(15) << "sglm.forward_ax" << Present(forward_ax) << std::endl ;  
    ss << std::setw(15) << "sglm.right_ax"   << Present(right_ax) << std::endl ;  
    ss << std::setw(15) << "sglm.top_ax"     << Present(top_ax) << std::endl ;  
    ss << std::endl ; 

    ss << " sglm.world2camera \n" << Present( world2camera ) << std::endl ; 
    ss << " sglm.camera2world \n" << Present( camera2world ) << std::endl ; 
    ss << std::endl ; 

    std::string s = ss.str(); 
    return s ; 
}


float SGLM::getGazeLength() const { return glm::length(gaze) ; }   // must be after updateELU 

void SGLM::updateProjection()
{
    float fsc = get_focal_basis() ;
    float fscz = fsc/ZOOM  ; 

    float aspect = Aspect(); 
    float left   = -aspect*fscz ;
    float right  =  aspect*fscz ;
    float bottom = -fscz ;
    float top    =  fscz ;

    float near_abs   = get_near_abs() ; 
    float far_abs    = get_far_abs()  ; 

    projection = glm::frustum( left, right, bottom, top, near_abs, far_abs );
    world2clip = projection * world2camera ;  //  ModelViewProjection :  no look rotation or trackballing   
}




std::string SGLM::descProjection() const 
{
    float fsc = get_focal_basis() ;
    float fscz = fsc/ZOOM  ; 
    float aspect = Aspect(); 
    float left   = -aspect*fscz ;
    float right  =  aspect*fscz ;
    float bottom = -fscz ;
    float top    =  fscz ;
    float near_abs   = get_near_abs() ; 
    float far_abs    = get_far_abs()  ; 

    int wid = 25 ; 
    std::stringstream ss ;
    ss << "SGLM::descProjection" << std::endl ; 
    ss << std::setw(wid) << "Aspect" << Present(aspect) << std::endl ;  
    ss << std::setw(wid) << "get_focal_basis" << Present(fsc) << std::endl ;  
    ss << std::setw(wid) << "get_focal_basis/ZOOM" << Present(fscz) << std::endl ;  
    ss << std::setw(wid) << "ZOOM" << Present(ZOOM) << std::endl ;  
    ss << std::setw(wid) << "left"   << Present(left) << std::endl ;  
    ss << std::setw(wid) << "right"  << Present(right) << std::endl ;  
    ss << std::setw(wid) << "top"    << Present(top) << std::endl ;  
    ss << std::setw(wid) << "bottom" << Present(bottom) << std::endl ;  
    ss << std::setw(wid) << "get_near_abs" << Present(near_abs) << std::endl ;  
    ss << std::setw(wid) << "get_far_abs" << Present(far_abs) << std::endl ;  

    ss << std::setw(wid) << "near" << Present(near) << std::endl ;  
    ss << std::setw(wid) << "far"  << Present(far) << std::endl ;  
    ss << std::setw(wid) << "sglm.projection\n" << Present(projection) << std::endl ; 
    ss << std::setw(wid) << "sglm.world2clip\n" << Present(world2clip) << std::endl ; 
    std::string s = ss.str(); 
    return s ; 
}


/**
SGLM::updateEyeBasis
----------------------

NB this uses the *camera2world* matrix obtained from SGLM::updateEyeSpace
with the same OpenGL eye frame convention to yield a scaled set of basis
vectors.  

**/

void SGLM::updateEyeBasis()
{
    glm::vec4 rht( 1., 0., 0., 0.);  // +X
    glm::vec4 top( 0., 1., 0., 0.);  // +Y
    glm::vec4 gaz( 0., 0.,-1., 0.);  // -Z
    glm::vec4 ori( 0., 0., 0., 1.); 

    float aspect = Aspect() ; 
    float fsc = get_focal_basis() ; 
    float fscz = fsc/ZOOM  ; 
    float gazlen = getGazeLength() ;  // HMM: maybe get_nearfar_basis for consistency

    u = glm::vec3( camera2world * rht ) * fscz * aspect ;  
    v = glm::vec3( camera2world * top ) * fscz  ;  
    w = glm::vec3( camera2world * gaz ) * gazlen ;    
    e = glm::vec3( camera2world * ori );   
}

std::string SGLM::descEyeBasis() const 
{
    std::stringstream ss ;
    ss << "SGLM::descEyeBasis : camera frame basis vectors transformed into world and focal plane scaled " << std::endl ; 

    int wid = 25 ; 
    float aspect = Aspect() ; 
    float fsc = get_focal_basis() ;
    float fscz = fsc/ZOOM ;  
    float gazlen = getGazeLength() ; 

    ss << std::setw(wid) << "aspect" << Present(aspect) << std::endl ;
    ss << std::setw(wid) << "near " << Present(near) << std::endl ;
    ss << std::setw(wid) << "ZOOM " << Present(ZOOM) << std::endl ;
    ss << std::setw(wid) << "get_focal_basis"      << Present(fsc) << std::endl ;
    ss << std::setw(wid) << "get_focal_basis/ZOOM" << Present(fscz) << std::endl ;
    ss << std::setw(wid) << "getGazeLength " << Present(gazlen) << std::endl ;

    ss << std::setw(wid) << "sglm.e " << Present(e) << " glm::vec3( camera2world * ori ) " << std::endl ; 
    ss << std::setw(wid) << "sglm.u " << Present(u) << " glm::vec3( camera2world * rht ) * fsc * aspect " << std::endl ; 
    ss << std::setw(wid) << "sglm.v " << Present(v) << " glm::vec3( camera2world * top ) * fsc  " << std::endl ; 
    ss << std::setw(wid) << "sglm.w " << Present(w) << " glm::vec3( camera2world * gaz ) * gazlen  " << std::endl ; 
    std::string s = ss.str(); 
    return s ; 
}

std::string SGLM::DescEyeBasis( const glm::vec3& E, const glm::vec3& U, const glm::vec3& V, const glm::vec3& W )
{
    int wid = 15 ; 
    std::stringstream ss ;
    ss << "SGLM::DescEyeBasis E,U,V,W " << std::endl ; 
    ss << std::setw(wid) << "E " << Present(E) << std::endl ; 
    ss << std::setw(wid) << "U " << Present(U) << std::endl ; 
    ss << std::setw(wid) << "V " << Present(V) << std::endl ; 
    ss << std::setw(wid) << "W " << Present(W) << std::endl ; 
    std::string s = ss.str(); 
    return s ; 
}








template <typename T>
inline T SGLM::ato_( const char* a )   // static 
{
    std::string s(a);
    std::istringstream iss(s);
    T v ; 
    iss >> v ; 
    return v ; 
}

template float    SGLM::ato_<float>( const char* a ); 
template unsigned SGLM::ato_<unsigned>( const char* a ); 
template int      SGLM::ato_<int>( const char* a ); 


 

template <typename T>
inline void SGLM::GetEVector(std::vector<T>& vec, const char* key, const char* fallback )  // static 
{
    const char* sval = getenv(key); 
    std::stringstream ss(sval ? sval : fallback); 
    std::string s ; 
    while(getline(ss, s, ',')) vec.push_back(ato_<T>(s.c_str()));   
}  

template void SGLM::GetEVector(std::vector<float>& vec, const char* key, const char* fallback ) ; 
template void SGLM::GetEVector(std::vector<int>& vec, const char* key, const char* fallback ) ; 
template void SGLM::GetEVector(std::vector<unsigned>& vec, const char* key, const char* fallback ) ; 

template <typename T>
inline std::string SGLM::Present(std::vector<T>& vec) // static 
{
    std::stringstream ss ;
    for(unsigned i=0 ; i < vec.size() ; i++) ss << vec[i] << " " ;
    return ss.str();
}




inline void SGLM::GetEVec(glm::vec3& v, const char* key, const char* fallback ) // static 
{
    std::vector<float> vec ; 
    SGLM::GetEVector<float>(vec, key, fallback); 
    std::cout << key << " " << Present(vec) << std::endl ; 
    assert( vec.size() == 3 );  
    for(int i=0 ; i < 3 ; i++) v[i] = vec[i] ; 
}

inline void SGLM::GetEVec(glm::vec4& v, const char* key, const char* fallback ) // static 
{
    std::vector<float> vec ; 
    SGLM::GetEVector<float>(vec, key, fallback); 
    std::cout << key << " " << Present(vec) << std::endl ; 
    assert( vec.size() == 4 );  
    for(int i=0 ; i < 4 ; i++) v[i] = vec[i] ; 
}

inline glm::vec4 SGLM::EVec4(const char* key, const char* fallback, float missing) // static
{
    std::vector<float> vec ; 
    SGLM::GetEVector<float>(vec, key, fallback); 
    glm::vec4 v ; 
    for(int i=0 ; i < 4 ; i++) v[i] = i < int(vec.size()) ? vec[i] : missing   ; 
    return v ; 
}

template <typename T>
inline T SGLM::EValue( const char* key, const char* fallback )  // static 
{
    const char* sval = getenv(key); 
    std::string s(sval ? sval : fallback); 
    T value = ato_<T>(s.c_str());
    return value ;    
} 

inline glm::ivec2 SGLM::EVec2i(const char* key, const char* fallback ) // static
{
    std::vector<int> vec ; 
    SGLM::GetEVector<int>(vec, key, fallback); 
    glm::ivec2 v ; 
    for(int i=0 ; i < 2 ; i++) v[i] = i < int(vec.size()) ? vec[i] : 0  ; 
    return v ; 
}

inline glm::vec3 SGLM::EVec3(const char* key, const char* fallback ) // static
{
    std::vector<float> vec ; 
    SGLM::GetEVector<float>(vec, key, fallback); 
    glm::vec3 v ; 
    for(int i=0 ; i < 3 ; i++) v[i] = i < int(vec.size()) ? vec[i] : 0.f  ; 
    return v ; 
}
inline std::string SGLM::Present(const glm::ivec2& v, int wid )
{
    std::stringstream ss ; 
    ss << std::setw(wid) << v.x << " " ; 
    ss << std::setw(wid) << v.y << " " ; 
    std::string s = ss.str(); 
    return s; 
}

inline std::string SGLM::Present(const float v, int wid, int prec)
{
    std::stringstream ss ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v ; 
    std::string s = ss.str(); 
    return s; 
}

inline std::string SGLM::Present(const glm::vec3& v, int wid, int prec)
{
    std::stringstream ss ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.x << " " ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.y << " " ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.z << " " ; 
    std::string s = ss.str(); 
    return s; 
}


inline std::string SGLM::Present(const glm::vec4& v, int wid, int prec)
{
    std::stringstream ss ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.x << " " ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.y << " " ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.z << " " ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.w << " " ; 
    std::string s = ss.str(); 
    return s; 
}

inline std::string SGLM::Present(const float4& v, int wid, int prec)
{
    std::stringstream ss ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.x << " " ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.y << " " ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.z << " " ; 
    ss << std::fixed << std::setw(wid) << std::setprecision(prec) << v.w << " " ; 
    std::string s = ss.str(); 
    return s; 
}





inline std::string SGLM::Present(const glm::mat4& m, int wid, int prec)
{
    std::stringstream ss ;
    for (int j=0; j<4; j++)
    {
        for (int i=0; i<4; i++) ss << std::fixed << std::setprecision(prec) << std::setw(wid) << m[i][j] ;
        ss << std::endl ;
    }
    return ss.str();
}


template<typename T>
inline std::string SGLM::Present_(const glm::tmat4x4<T>& m, int wid, int prec)
{
    std::stringstream ss ;
    for (int j=0; j<4; j++)
    {
        for (int i=0; i<4; i++) ss << std::fixed << std::setprecision(prec) << std::setw(wid) << m[i][j] ;
        ss << std::endl ;
    }
    return ss.str();
}


template<typename T>
inline glm::tmat4x4<T> SGLM::DemoMatrix(T scale)  // static
{
    std::array<T, 16> demo = {{
        T(1.)*scale,   T(2.)*scale,   T(3.)*scale,   T(4.)*scale ,
        T(5.)*scale,   T(6.)*scale,   T(7.)*scale,   T(8.)*scale ,
        T(9.)*scale,   T(10.)*scale,  T(11.)*scale,  T(12.)*scale ,
        T(13.)*scale,  T(14.)*scale,  T(15.)*scale,  T(16.)*scale 
      }} ;
    return glm::make_mat4x4<T>(demo.data()) ;
}



