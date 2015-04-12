#include <stdlib.h>  //exit()
#include <stdio.h>

// oglrap-
//  Frame include brings in GL/glew.h GLFW/glfw3.h gleq.h
#include "Frame.hh"
#include "FrameCfg.hh"
#include "Composition.hh"
#include "CompositionCfg.hh"
#include "Geometry.hh"
#include "Renderer.hh"
#include "RendererCfg.hh"
#include "Interactor.hh"
#include "InteractorCfg.hh"
#include "Camera.hh"
#include "CameraCfg.hh"
#include "View.hh"
#include "ViewCfg.hh"
#include "Trackball.hh"
#include "TrackballCfg.hh"

// numpyserver-
#include "numpydelegate.hpp"
#include "numpydelegateCfg.hpp"
#include "numpyserver.hpp"


int main(int argc, char** argv)
{
    Frame frame ;
    Composition composition ;
    Interactor interactor ; 
    Renderer renderer ; 
    Geometry geometry ; 

    numpydelegate delegate ; 

    frame.setInteractor(&interactor);    // GLFW key and mouse events from frame to interactor
    interactor.setup(composition.getCamera(), composition.getView(), composition.getTrackball());  // interactor changes camera, view, trackball 
    renderer.setComposition(&composition);


    FrameCfg<Frame>* framecfg = new FrameCfg<Frame>("frame", &frame, false);

    Cfg cfg("unbrella", false) ;  // collect other Cfg objects
    cfg.add(framecfg);
    cfg.add(new numpydelegateCfg<numpydelegate>("numpydelegate", &delegate, false));
    cfg.add(new RendererCfg<Renderer>("renderer", &renderer, true));
    cfg.add(new CompositionCfg<Composition>("composition", &composition, true));
    cfg.add(new CameraCfg<Camera>("camera", composition.getCamera(), true));
    cfg.add(new ViewCfg<View>(    "view",   composition.getView(),   true));
    cfg.add(new TrackballCfg<Trackball>( "trackball",   composition.getTrackball(),   true));
    cfg.add(new InteractorCfg<Interactor>( "interactor",  &interactor,   true));

    cfg.commandline(argc, argv);
    delegate.liveConnect(&cfg);    

    // hmm these below elswhere, as are needed for non-GUI apps too
    if(framecfg->isHelp())  std::cout << cfg.getDesc() << std::endl ;
    if(framecfg->isAbort()) exit(EXIT_SUCCESS); 

    numpyserver<numpydelegate> srv(&delegate);

    frame.setSize(640,480);
    frame.setTitle("Demo");
    frame.gl_init_window();

    geometry.load("GLFWTEST_") ;
    renderer.setDrawable(geometry.getDrawable());

    GLFWwindow* window = frame.getWindow();

    while (!glfwWindowShouldClose(window))
    {
        frame.listen(); 

        srv.poll_one();  

        frame.render();
        renderer.render();

        glfwSwapBuffers(window);
    }
    srv.stop();


    frame.exit();

    exit(EXIT_SUCCESS);
}

