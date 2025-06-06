#pragma once
/**
SGLFW.h : Light touch OpenGL render loop and key handling 
===========================================================

Light touch encapsulation of OpenGL window and shader program, 
that means trying to hide boilerplate, but not making lots of 
decisions for user and getting complicated and inflexible like 
the old oglrap/Frame.hh oglrap/OpticksViz did. 

WASD : Navigation in 3D space 
-------------------------------

* FPS : first-person shooters
* https://learnopengl.com/Getting-started/Camera


* :google:`mouse interface to move around 3D environment`
* https://www.researchgate.net/publication/220863069_Comparison_of_3D_Navigation_Interfaces

For "first-person shooters", navigation typically uses a combination of mouse
and keyboard. The keyboard keys "WASD" are used for movement forward, a side
step (“strafe”) left, backward, and a side step right, respectively. The users
view direction is controlled by the mouse so that moving the mouse rotates the
view direction of the users head.

To change the view direction the user presses and
holds either of the side mouse buttons. When pressing and
holding either side button the cursor disappears as a secondary
indication that the view direction is tethered to the mouse
movement. Likewise, when the buttons are not pressed the
cursor is displayed so the user can then use it to select (and
manipulate) objects. This user interface implementation
enables a seamless transition between navigation and
manipulation without the need for designating a button or keys
solely for mode switching.

An important attribute to note about this interface is that
some movements can only be achieved by using the keyboard
and mouse buttons in conjunction. To turn while moving
forward or backward it is necessary to hold down the ‘W’ or
‘S’ key simultaneously with a mouse side button, and then
moving the mouse left or right to “steer”. Orbiting an object
can also be achieved by holding ‘A’ or ‘D’ simultaneously
with a side mouse button and moving the mouse to change the
view direction simultaneously while the user side steps. This
makes this user interface more powerful than ‘Click-to-Move’.
If done properly, one can keep a point of the virtual
environment fixed in the center of the view while moving
around it.
    
**/

#include <cassert>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#ifndef GLFW_TRUE
#define GLFW_TRUE true
#endif

#include "GL_CHECK.h"

#define GLEQ_IMPLEMENTATION
#include "gleq.h"

#include <glm/glm.hpp>
#include "NPU.hh"

#include "ssys.h"
#include "spath.h"

#include "SMesh.h"
#include "SGLM.h"

#include "SGLFW_Extras.h"
#include "SGLFW_Program.h"
#include "SGLFW_Mesh.h"
#include "SGLFW_GLEQ.h"

#ifdef WITH_CUDA_GL_INTEROP
#include "SGLFW_CUDA.h"
#endif


#include "SGLFW_Keys.h"

#include "SIMG_Frame.h"


struct SGLFW : public SCMD 
{
    static constexpr const char* HELP =  R"LITERAL(
SGLFW.h
========

::
                  

                 Q  W 
                 | /
                 |/
             A---+---D
                /|
               / |
              S  E          


A
   (WASDQE) hold to change eyeshift, translate left 
B
   invokes SGLM::desc describing view parameters [--desc]
C
   toggle.cuda between rasterized and raytrace render 
   [currently only implemented for sysrap triangulated renderer]
D
   (WASDQE) hold to change eyeshift, translate right 
E
   (WASDQE) hold to change eyeshift, translate down 
F
   toggle.tmax : then change far by moving cursor vertically 
G
   -
H
   invokes SGLM::home returning to initial position with no lookrotation or eyeshift [--home]
   and dumps this help string [--help]
I
   --snap-local
J
   --snap-local-inverted
K
   save screen shot [--snap]
L
   save Y inverted screen shot [--snap-inverted]
M
   hop to the MOI envvar configured frame [not supported by all renderers]
N
   toggle.tmin : then change near by moving cursor vertically 
O
   switch camera between perspective and orthographic projections [--tcam]
P
   -
Q
   (WASDQE) hold to change eyeshift, translate up 

R
   hold down while arcball dragging mouse to change look rotation 
S
   (WASDQE) hold to change eyeshift, translate backwards 
T:toggle.tran
   no longer used : now use holding down WASDQE keys to change the eyeshift
U:toggle.norm
   for rasterized render toggle between wireframe and normal shading 
V
   [--traceyflip] invert vertical of the raytrace render, via Param.h signal to kernel 
W
   (WASDQE) hold to change eyeshift, translate forwards 
X
   [--rendertype] toggle between normal and zdepth shading 
Y
   experimental mouse control of eyerotation
Z:toggle.zoom
   change zoom scaling by moving cursor vertically 


0,1,2,3,4,5,6,7,8,9
   hop to frame "num" or default if no such frame
   
0,1,2,3,4,5,6,7,8,9 + SHIFT
   hop to frame "num + 10" using offset for SHIFT modifier

0,1,2,3,4,5,6,7,8,9 + ALT
   hop to frame "num + 20" using offset for ALT modifier
   (hop to default frame if there is no frame with the index) 

0,1,2,3,4,5,6,7,8,9 + SHIFT + ALT
   hop to frame "num + 30" using offset for SHIFT and ALT modifier
   (hop to default frame if there is no frame with the index) 

With all num_key frame selection is there is no frame with the index
then hop to the default frame.  


)LITERAL" ; 
    static constexpr const char* TITLE = "SGLFW" ; 

    SGLM& gm ; 
    int wanted_frame_idx ; 
    int wanted_snap ; 

    int width ; 
    int height ; 

    const char* title ; 
    GLFWwindow* window ; 

    int count ; 
    int renderlooplimit ; 
    bool exitloop ; 

    bool dump ; 
    int  _width ;  // on retina 2x width 
    int  _height ;
    SIMG_Frame*  sif ; 
    SIMG_Frame*  sid ; 

    // getStartPos
    double _start_x ; 
    double _start_y ; 

    glm::vec2 start_ndc ;  // from key_pressed
    glm::vec2 move_ndc ;   // from cursor_moved
    glm::vec4 drag ; 

    SGLFW_Toggle toggle = {} ; 
    SGLFW_Keys keys = {} ; 



    bool renderloop_proceed(); 
    void renderloop_exit(); 
    void renderloop_head(); 
    void renderloop_tail(); 

    void handle_event(GLEQevent& event); 

    void window_refresh();
    void key_pressed(unsigned key); 
    void numkey_pressed(unsigned num, unsigned modifiers); 

    void set_wanted_frame_idx(int _idx); 
    int  get_wanted_frame_idx() const ;



    void snap(int w); 
    void set_wanted_snap(int w); 
    int get_wanted_snap() const ;

    void download_pixels(); 
    void download_depth(); 
    void init_img_frame(); 
    void writeJPG(const char* path) const ; 
    void snap_local(bool yflip); 

    void key_repeated(unsigned key); 
    void key_released(unsigned key); 

    void button_pressed(unsigned button, unsigned mods); 
    void button_released(unsigned button, unsigned mods); 


    void cursor_moved(int ix, int iy); 
    void cursor_moved_action(); 

    int command(const char* cmd); 
    static void Help(); 
    void home(); 
    void _desc(); 
    void tcam(); 
    void traceyflip(); 
    void rendertype(); 
    static std::string FormCommand(const char* token, float value); 

    void getWindowSize();
    std::string descWindowSize() const;

    void setCursorPos(float ndc_x, float ndc_y);
    void getStartPos(); 
    std::string descDrag() const;
    std::string descStartPos() const;  

    SGLFW(SGLM& gm ); 
    virtual ~SGLFW(); 
    static void Error_callback(int error, const char* description); 
    void init(); 

}; 

inline bool SGLFW::renderloop_proceed()
{
    return !glfwWindowShouldClose(window) && !exitloop ; 
}
inline void SGLFW::renderloop_exit()
{
    std::cout << "SGLFW::renderloop_exit" << std::endl; 
    glfwSetWindowShouldClose(window, true);
}
inline void SGLFW::renderloop_head()
{
    dump = count % 100000 == 0 ; 

    getWindowSize();
    glViewport(0, 0, _width, _height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(dump) std::cout << "SGLFW::renderloop_head" << " gl.count " << count << std::endl ;

    if(count == 0 ) home(); 
}


inline void SGLFW::renderloop_tail()
{
    glfwSwapBuffers(window);
    glfwPollEvents();

    GLEQevent event;
    while (gleqNextEvent(&event))
    {
        handle_event(event);
        gleqFreeEvent(&event);
    }

    exitloop = renderlooplimit > 0 && count++ > renderlooplimit ;
}



/**
SGLFW::handle_event
--------------------

See oglrap/Frame::handle_event

**/

inline void SGLFW::handle_event(GLEQevent& event)
{
    //std::cout << "SGLFW::handle_event " << SGLFW_GLEQ::Name(event.type) << std::endl; 
    switch(event.type)
    {
        case GLEQ_KEY_PRESSED:   key_pressed( event.keyboard.key)       ; break ; 
        case GLEQ_KEY_REPEATED:  key_repeated(event.keyboard.key)       ; break ;
        case GLEQ_KEY_RELEASED:  key_released(event.keyboard.key)       ; break ;
        case GLEQ_BUTTON_PRESSED:  button_pressed(  event.mouse.button, event.mouse.mods)  ; break ; 
        case GLEQ_BUTTON_RELEASED: button_released( event.mouse.button, event.mouse.mods)  ; break ; 
        case GLEQ_CURSOR_MOVED:    cursor_moved(event.pos.x, event.pos.y) ; break ;
        case GLEQ_WINDOW_REFRESH:  window_refresh()                       ; break ; 
        default:                                                          ; break ; 
    }
}



/**
SGLFW::window_refresh
----------------------

By observation this event fires only in initialization

**/

inline void SGLFW::window_refresh()
{
    home(); 
}


/**
SGLFW::key_pressed
--------------------

HMM:dont like the arbitrary split between here and SGLM.h for interaction control 
Maybe remove the toggle or do that in SGLM ?

**/


inline void SGLFW::key_pressed(unsigned key)
{
    keys.key_pressed(key); 
    unsigned modifiers = keys.modifiers() ;

    getStartPos(); 
    std::cout 
        << descStartPos() 
        << descWindowSize() 
        << std::endl
        ; 

    switch(key)
    {
        case GLFW_KEY_0:
        case GLFW_KEY_1:
        case GLFW_KEY_2:
        case GLFW_KEY_3:
        case GLFW_KEY_4:
        case GLFW_KEY_5:
        case GLFW_KEY_6:
        case GLFW_KEY_7:
        case GLFW_KEY_8:
        case GLFW_KEY_9:
                              numkey_pressed(key - GLFW_KEY_0, modifiers) ; break ; 
        case GLFW_KEY_M:
                              set_wanted_frame_idx(-2)              ; break ;   // MOI target 
        case GLFW_KEY_Z:      toggle.zoom = !toggle.zoom            ; break ; 
        case GLFW_KEY_N:      toggle.tmin = !toggle.tmin            ; break ; 
        case GLFW_KEY_F:      toggle.tmax = !toggle.tmax            ; break ; 
        case GLFW_KEY_R:      toggle.lrot = !toggle.lrot            ; break ; 
        case GLFW_KEY_C:      toggle.cuda = !toggle.cuda            ; break ; 
        case GLFW_KEY_U:      toggle.norm = !toggle.norm            ; break ; 
        case GLFW_KEY_T:      toggle.tran = !toggle.tran            ; break ; 
        case GLFW_KEY_B:      command("--desc")                     ; break ; 
        case GLFW_KEY_H:      command("--home") ; command("--help") ; break ; 
        case GLFW_KEY_O:      command("--tcam")                     ; break ;  
        case GLFW_KEY_I:      command("--snap-local")               ; break ;  
        case GLFW_KEY_J:      command("--snap-local-inverted")      ; break ;  
        case GLFW_KEY_K:      command("--snap")                     ; break ;  
        case GLFW_KEY_L:      command("--snap-inverted")            ; break ;  
        case GLFW_KEY_V:      command("--traceyflip")               ; break ;  
        case GLFW_KEY_X:      command("--rendertype")               ; break ;  
        case GLFW_KEY_ESCAPE: command("--exit")                     ; break ;  
    }
   
    //if(modifiers > 0) gm.key_pressed_action(modifiers) ; // not triggered repeatedly enough for navigation

    std::cout << toggle.desc() << std::endl ; 
}

inline void SGLFW::numkey_pressed(unsigned _num, unsigned modifiers)
{
    bool with_shift = SGLM_Modifiers::IsShift(modifiers) ;
    bool with_control = SGLM_Modifiers::IsControl(modifiers) ;
    bool with_alt = SGLM_Modifiers::IsAlt(modifiers) ;
    bool with_super = SGLM_Modifiers::IsSuper(modifiers) ;

    unsigned offset = 0 ; 
    if(         with_shift && !with_alt)  offset = 10 ; 
    else if(   !with_shift &&  with_alt)  offset = 20 ;
    else if(    with_shift &&  with_alt)  offset = 30 ;

    unsigned num = _num + offset ; 

    std::cout 
        << "SGLFW::numkey_pressed"
        << " _num " << _num 
        << " modifiers " << modifiers 
        << " SGLM_Modifiers::Desc(modifiers) " << SGLM_Modifiers::Desc(modifiers)
        << " offset " << offset 
        << " num " << num 
        << " with_shift " << with_shift
        << " with_control " << with_control
        << " with_alt " << with_alt
        << " with_super " << with_super
        << "\n" 
        ; 

    set_wanted_frame_idx(num); 
}
inline void SGLFW::set_wanted_frame_idx(int _idx){ wanted_frame_idx = _idx ; }
inline int  SGLFW::get_wanted_frame_idx() const { return wanted_frame_idx ; }


inline void SGLFW::snap(int w){ set_wanted_snap(w); }

inline void SGLFW::set_wanted_snap(int w){ wanted_snap = w ; }
inline int SGLFW::get_wanted_snap() const { return wanted_snap ; }


/**
SGLFW::download_pixels
--------------------------

After oglrap Pix

https://www.khronos.org/opengl/wiki/GLAPI/glPixelStore

**/

inline void SGLFW::download_pixels()
{
    if(sif == nullptr) init_img_frame();  

    assert( _width > 0 && _width == sif->width ) ; 
    assert( _height > 0 && _height == sif->height ) ; 

    glPixelStorei(GL_PACK_ALIGNMENT,1);   // byte aligned output
    glReadPixels(0,0,_width,_height,GL_RGBA, GL_UNSIGNED_BYTE, sif->pixels );

    if(sid != nullptr) download_depth(); 
} 


/**
SGLFW::download_depth
----------------------

Depth component "native" type is GL_FLOAT in range 0. to 1. 
when type is not GL_FLOAT then the read values are scaled depending 
on the type, eg by 255. for type of GL_UNSIGNED_BYTE

* https://registry.khronos.org/OpenGL-Refpages/gl4/html/glReadPixels.xhtml 

**/

inline void SGLFW::download_depth()
{
    assert(sid); 
    GLenum format = GL_DEPTH_COMPONENT ;
    GLenum type = GL_UNSIGNED_BYTE ;
    glPixelStorei(GL_PACK_ALIGNMENT,1);   // byte aligned output
    glReadPixels(0,0,_width,_height, format, type, sid->pixels ); 
}


inline void SGLFW::init_img_frame()
{
    assert( _width > 0 ); 
    assert( _height > 0 ); 
    int channels = 4 ; 

    sif = new SIMG_Frame(_width, _height, channels ) ; 

    bool DEPTH  = ssys::getenvbool("SGLFW__DEPTH"); 
    if(DEPTH)
    {
        sid = new SIMG_Frame(_width, _height, 1 );
    }
}
inline void SGLFW::writeJPG(const char* path) const 
{
    std::cout << "SGLFW::writeJPG [" << ( path ? path : "-" ) << "]\n" ; 
    assert(sif); 
    sif->writeJPG(path);

    if(sid)
    {
        const char* dpath = sstr::ReplaceEnd( path, ".jpg", "_depth.jpg" ); 
        std::cout << "SGLFW::writeJPG [" << ( dpath ? dpath : "-" ) << "]\n" ; 
        sid->writeJPG(dpath); 

        const char* npath = sstr::ReplaceEnd( path, ".jpg", "_depth.npy" ); 
        std::cout << "SGLFW::writeJPG [" << ( npath ? npath : "-" ) << "]\n" ; 
        sid->writeNPY(npath); 
    }
}


/**
SGLFW::snap_local
-----------------

Default stem of nullptr leads to use of current datetime formatted 
string of form sstamp::DEFAULT_TIME_FMT 

CAUTION : this is currently NOT used from::

     ~/o/cx.sh 
     CSGOptiXRenderInteractiveTest
     CSGOptiX::snap

It is used from::

   sysrap/tests/SGLFW_SOPTIX_Scene_test.cc

**/

inline void SGLFW::snap_local(bool yflip)
{
    download_pixels(); 
    if(yflip)
    { 
        sif->flipVertical();
        if(sid) sid->flipVertical(); 
    }

    const char* stem = ssys::getenvvar("SGLFW__snap_local_STEM", nullptr ); 
    int index = 0 ; 
    const char* ext = ".jpg" ;  
    bool unique = true ;
    const char* path = spath::DefaultOutputPath(stem, index, ext, unique); 
    spath::MakeDirsForFile(path);

    writeJPG(path);
}


inline void SGLFW::key_repeated(unsigned key)
{
    //std::cout << "SGLFW::key_repeated " << key << "\n" ; 
    keys.key_pressed(key); 
    unsigned modifiers = keys.modifiers() ;
    if(modifiers == 0) return ;  
    gm.key_pressed_action(modifiers) ; 
    gm.update(); 
}

inline void SGLFW::key_released(unsigned key)
{
    keys.key_released(key); 
}

inline void SGLFW::button_pressed(unsigned button, unsigned mods)
{
    std::cout << "SGLFW::button_pressed " << button << " " << mods << "\n" ; 
}
inline void SGLFW::button_released(unsigned button, unsigned mods)
{
    std::cout << "SGLFW::button_released " << button << " " << mods << "\n" ; 
}





inline int SGLFW::command(const char* cmd)
{
    if(strcmp(cmd, "--exit") == 0) renderloop_exit(); 
    if(strcmp(cmd, "--help") == 0) Help(); 
    if(strcmp(cmd, "--home") == 0) home(); 
    if(strcmp(cmd, "--desc") == 0) _desc(); 
    if(strcmp(cmd, "--tcam") == 0) tcam(); 
    if(strcmp(cmd, "--snap") == 0) snap(1); 
    if(strcmp(cmd, "--snap-inverted") == 0) snap(2); 
    if(strcmp(cmd, "--snap-local") == 0) snap_local(false); 
    if(strcmp(cmd, "--snap-local-inverted") == 0) snap_local(true); 
    if(strcmp(cmd, "--traceyflip") == 0) traceyflip(); 
    if(strcmp(cmd, "--rendertype") == 0) rendertype(); 
    return 0 ;  
}

inline void SGLFW::Help() 
{
    std::cout << HELP ; 
}

/**
SGLFW::home
-------------

1. center cursor position 
2. zero the eyeshift and rotations, set zoom to 1  

WIP: invoking doing this from ctor at tail of SGLFW::init flaky 
     so try from renderloop_head for count==0  

**/

inline void SGLFW::home()
{
    setCursorPos(0.f,0.f); 
    gm.command("--home"); 
} 
inline void SGLFW::_desc()
{
    gm.command("--desc"); 
}
inline void SGLFW::tcam()
{
    gm.command("--tcam"); 
}
inline void SGLFW::traceyflip()
{
    gm.command("--traceyflip"); 
}
inline void SGLFW::rendertype()
{
    gm.command("--rendertype"); 
}





inline std::string SGLFW::FormCommand(const char* token, float value)  // static
{
    std::stringstream ss ; 
    ss << token << " " << value ; 
    std::string str = ss.str();
    return str ;
}




/**
SGLFW::getWindowSize
---------------------

eg on macOS with retina screen : SGLFW::descWindowSize wh[1024, 768] _wh[2048,1536]

**/
inline void SGLFW::getWindowSize()
{
    glfwGetWindowSize(window, &width, &height);
    glfwGetFramebufferSize(window, &_width, &_height);
}
inline std::string SGLFW::descWindowSize() const
{
    std::stringstream ss ; 
    ss << "SGLFW::descWindowSize"
       << " wh["
       << std::setw(4) << width
       << ","
       << std::setw(4) << height
       << "]"
       << " _wh["
       << std::setw(4) << _width 
       << ","
       << std::setw(4) << _height
       << "]"
       ;
    std::string str = ss.str();
    return str ;
}


inline void SGLFW::setCursorPos(float ndc_x, float ndc_y )
{
    float x = (1.f + ndc_x)*width/2.f ; 
    float y = (1.f - ndc_y)*height/2.f ; 
    glfwSetCursorPos(window, x, y );
}


/**
SGLFW::getStartPos
----------------------


::

    TL:cursor(0.,0.) ndc(-1.,1.)
    +-----------------+
    |                 |
    |                 |
    |        + CENTER: cursor(width/2,height/2) ndc(0.,0.)
    |                 |
    |                 |
    +-----------------+ BR:cursor(width, height) ndc(1.,-1.)


**/

inline void SGLFW::getStartPos()
{
    glfwGetCursorPos(window, &_start_x, &_start_y );

    start_ndc.x = 2.f*_start_x/width - 1.f ; 
    start_ndc.y = 1.f - 2.f*_start_y/height ; 
}

/**
SGLFW::cursor_moved
-----------------------

Follow old approach from Frame::cursor_moved_just_move

1. convert pixel positions into ndc (x,y) [-1:1, -1:1]


As cursor_moved gets called repeatedly during mouse
movements the drag.z drag.w tend to be small.

To combat this for local rotation control via quaternion
use the abolute start position (from key_pressed) 
and current position from cursor_moved. 

**/
inline void SGLFW::cursor_moved(int ix, int iy)
{
    move_ndc.x  = 2.f*float(ix)/width - 1.f ;   
    move_ndc.y  = 1.f - 2.f*float(iy)/height ; 

    float dx = move_ndc.x - drag.x ; 
    float dy = move_ndc.y - drag.y ;   // delta with the prior call to cursor_moved

    drag.x = move_ndc.x ; 
    drag.y = move_ndc.y ; 
    drag.z = dx ; 
    drag.w = dy ; 

    cursor_moved_action(); 
}

inline void SGLFW::cursor_moved_action()
{
    unsigned modifiers = keys.modifiers() ;
    float dy = drag.w ; 
    if(toggle.zoom)
    {
        std::string cmd = FormCommand("--inc-zoom", dy*100 ); 
        gm.command(cmd.c_str()) ;
    } 
    else if(toggle.tmin)
    {
        std::string cmd = FormCommand("--inc-tmin", dy ); 
        gm.command(cmd.c_str()) ;
    }
    else if(toggle.tmax)
    {
        std::string cmd = FormCommand("--inc-tmax", dy*100 ); 
        gm.command(cmd.c_str()) ;
    }
    /*
    else if(toggle.lrot)
    {
        //std::cout << "SGLFW::cursor_moved_action.lrot " << std::endl ; 
        gm.setLookRotation(start_ndc, move_ndc); 
        gm.update(); 
    }
    else if(toggle.tran)
    {
        gm.setEyeShift(start_ndc, move_ndc, modifiers );
        gm.update(); 
    }
    */
    else
    {
        gm.cursor_moved_action(start_ndc, move_ndc, modifiers );
        gm.update(); 
    }
}

inline std::string SGLFW::descDrag() const
{
    std::stringstream ss ; 
    ss
        << " (" 
        << std::setw(10) << std::fixed << std::setprecision(3) << drag.x 
        << ","
        << std::setw(10) << std::fixed << std::setprecision(3) << drag.y
        << ","
        << std::setw(10) << std::fixed << std::setprecision(3) << drag.z
        << ","
        << std::setw(10) << std::fixed << std::setprecision(3) << drag.w
        << ")" 
        ;
    std::string str = ss.str();
    return str ;
}


inline std::string SGLFW::descStartPos() const
{
    std::stringstream ss ; 
    ss << "SGLFW::descCursorPos"
       << "["
       << std::setw(7) << std::fixed << std::setprecision(2) << _start_x 
       << ","
       << std::setw(7) << std::fixed << std::setprecision(2) << _start_y
       << "]"
       << " ndc["
       << std::setw(7) << std::fixed << std::setprecision(2) << start_ndc.x
       << ","
       << std::setw(7) << std::fixed << std::setprecision(2) << start_ndc.y
       << "]"
       ;
    std::string str = ss.str();
    return str ;
}

/**
SGLFW::SGLFW
-------------

 -1:corresponds to frame formed from entire CE center-extent  

**/

inline SGLFW::SGLFW(SGLM& _gm )
    :
    gm(_gm),
    wanted_frame_idx(ssys::getenvint("SGLFW_FRAME", -2)),   
    wanted_snap(0),
    width(gm.Width()),
    height(gm.Height()),
    title(TITLE),
    window(nullptr),
    count(0),
    renderlooplimit(ssys::getenvint("SGLFW__renderlooplimit",1000000)), 
    exitloop(false),
    dump(false),
    _width(0),
    _height(0),
    sif(nullptr),
    sid(nullptr),
    _start_x(0.),
    _start_y(0.),
    start_ndc(0.f,0.f),
    move_ndc(0.f,0.f),
    drag(0.f,0.f,0.f,0.f)
{
    init(); 
}

inline SGLFW::~SGLFW()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

inline void SGLFW::Error_callback(int error, const char* description) // static
{
    fprintf(stderr, "SGLFW::Error_callback: %s\n", description);
}

/**
SGLFW::init
-------------

1. OpenGL initialize
2. create window


Example responses::

     Renderer: NVIDIA GeForce GT 750M OpenGL Engine
     OpenGL version supported 4.1 NVIDIA-10.33.0 387.10.10.10.40.105

     Renderer: TITAN RTX/PCIe/SSE2
     OpenGL version supported 4.1.0 NVIDIA 418.56

**/

inline void SGLFW::init()
{
    glfwSetErrorCallback(SGLFW::Error_callback);
    if (!glfwInit()) exit(EXIT_FAILURE);

    gleqInit();

    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4); 
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 1);  // also used 6 here 
    //glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // remove stuff deprecated in requested release
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);   
    // https://learnopengl.com/In-Practice/Debugging Debug output is core since OpenGL version 4.3,   

    // HMM: using fullscreen mode with resolution less than display changes display resolution 
    GLFWmonitor* monitor = gm.fullscreen ? glfwGetPrimaryMonitor() : nullptr ;   // nullptr for windowed mode
    GLFWwindow* share = nullptr ;     // window whose context to share resources with, or NULL to not share resources

    window = glfwCreateWindow(width, height, title, monitor, share);
    if (!window)
    {   
        glfwTerminate();
        exit(EXIT_FAILURE);
    }   
    //glfwSetKeyCallback(window, SGLFW::key_callback);  // using gleq event for key callbacks not this manual approach 
    glfwMakeContextCurrent(window);

    gleqTrackWindow(window);  // replaces callbacks, see https://github.com/glfw/gleq
    
    glewExperimental = GL_TRUE;
    glewInit (); 

    

    GLenum err0 = glGetError() ; 
    GLenum err1 = glGetError() ; 
    bool err0_expected = err0 == GL_INVALID_ENUM ; // long-standing glew bug apparently 
    bool err1_expected = err1 == GL_NO_ERROR ; 
    if(!err0_expected) printf("//SGLFW::init UNEXPECTED err0 %d \n", err0 ); 
    if(!err1_expected) printf("//SGLFW::init UNEXPECTED err1 %d \n", err1 ); 
    //assert( err0_expected );  
    //assert( err1_expected );  

    const GLubyte* renderer = glGetString (GL_RENDERER);
    const GLubyte* version = glGetString (GL_VERSION);
    printf("//SGLFW::init GL_RENDERER [%s] \n", renderer );
    printf("//SGLFW::init GL_VERSION [%s] \n", version );


    //  https://learnopengl.com/Advanced-OpenGL/Depth-testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);  


    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE); // otherwise gl_PointSize setting ignored, setting in geom not vert shader used when present 

    int interval = 1 ; // The minimum number of screen updates to wait for until the buffers are swapped by glfwSwapBuffers.
    glfwSwapInterval(interval);

    //home(); // maybe too soon to do this, as flaky where the cursor starts  
}

