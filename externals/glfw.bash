##
## Copyright (c) 2019 Opticks Team. All Rights Reserved.
##
## This file is part of Opticks
## (see https://bitbucket.org/simoncblyth/opticks).
##
## Licensed under the Apache License, Version 2.0 (the "License"); 
## you may not use this file except in compliance with the License.  
## You may obtain a copy of the License at
##
##   http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software 
## distributed under the License is distributed on an "AS IS" BASIS, 
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
## See the License for the specific language governing permissions and 
## limitations under the License.
##

glfw-src(){      echo externals/glfw.bash ; }
glfw-source(){   echo ${BASH_SOURCE} ; }
glfw-vi(){       vi $(glfw-source) ; }



glfw-usage(){ cat << EOU

GLFW
======

* http://www.glfw.org

GLFW is an Open Source, multi-platform library for creating windows with OpenGL
contexts and receiving input and events. It is easy to integrate into existing
applications and does not lay claim to the main loop.

Version 3.1.1 released on March 19, 2015

Headless
-----------

* https://github.com/intel-isl/Open3D/issues/17


Build may fail for lack of a few system dependencies
-----------------------------------------------------

For example::

    CMake Error at CMakeLists.txt:206 (message):
    RandR headers not found; install libxrandr development package

How to install these depends on your system and its package manager.
Some suggestions for various systems are described below.


Linux Ubuntu with apt-get
---------------------------

::

     sudo apt-get install libxrandr-dev
     sudo apt-get install libxinerama-dev
     sudo apt-get install libxi-dev


Linux CentOS 7 with yum
--------------------------

eventually got 3.2.1 from epel
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

	[blyth@localhost imgui]$ yum list installed | grep glfw
	glfw.x86_64                             1:3.2.1-2.el7                  @epel    
	glfw-devel.x86_64                       1:3.2.1-2.el7                  @epel    


Initially tried to install manually with 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You may need::

    yum install libXrandr-devel 
    yum install libXinerama-devel 
    yum install libXcursor-devel 


::

	[ 16%] Building C object src/CMakeFiles/glfw.dir/glx_context.c.o
	/home/blyth/local/opticks/externals/glfw/glfw-3.1.1/src/glx_context.c: In function ‘_glfwPlatformGetProcAddress’:
	/home/blyth/local/opticks/externals/glfw/glfw-3.1.1/src/glx_context.c:541:5: warning: pointer targets in passing argument 2 of ‘dlsym’ differ in signedness [-Wpointer-sign]
	     return _glfw_glXGetProcAddress((const GLubyte*) procname);
	     ^
	In file included from /home/blyth/local/opticks/externals/glfw/glfw-3.1.1/src/glx_context.h:41:0,
			 from /home/blyth/local/opticks/externals/glfw/glfw-3.1.1/src/x11_platform.h:63,
			 from /home/blyth/local/opticks/externals/glfw/glfw-3.1.1/src/internal.h:85,
			 from /home/blyth/local/opticks/externals/glfw/glfw-3.1.1/src/glx_context.c:28:
	/usr/include/dlfcn.h:65:14: note: expected ‘const char * __restrict__’ but argument is of type ‘const GLubyte *’
	 extern void *dlsym (void *__restrict __handle,
		      ^
	[ 17%] Linking C shared library libglfw.so
	[ 17%] Built target glfw
	Scanning dependencies of target boing
	[ 18%] Building C object examples/CMakeFiles/boing.dir/boing.c.o
	[ 20%] Linking C executable boing
	/usr/bin/ld: CMakeFiles/boing.dir/boing.c.o: undefined reference to symbol 'glClear'
	//usr/lib64/libGL.so.1: error adding symbols: DSO missing from command line
	collect2: error: ld returned 1 exit status


Windows
--------

gitbash glfw-make which uses "cmake --build ."::

      -- Install configuration: "Debug"
      -- Up-to-date: C:/usr/local/opticks/externals/include/GLFW
      -- Up-to-date: C:/usr/local/opticks/externals/include/GLFW/glfw3.h
      -- Up-to-date: C:/usr/local/opticks/externals/include/GLFW/glfw3native.h
      -- Installing: C:/usr/local/opticks/externals/lib/cmake/glfw/glfw3Config.cmake
      -- Installing: C:/usr/local/opticks/externals/lib/cmake/glfw/glfw3ConfigVersion.cmake
      -- Installing: C:/usr/local/opticks/externals/lib/cmake/glfw/glfwTargets.cmake
      -- Installing: C:/usr/local/opticks/externals/lib/cmake/glfw/glfwTargets-debug.cmake
      -- Installing: C:/usr/local/opticks/externals/lib/pkgconfig/glfw3.pc
      -- Installing: C:/usr/local/opticks/externals/lib/glfw3dll.lib
      -- Installing: C:/usr/local/opticks/externals/lib/glfw3.dll



APIENTRY Macro Redefinition
-----------------------------

oglrap-- build warning::
::

      Shdr.cc
      Texture.cc
    C:\Program Files (x86)\Windows Kits\8.1\Include\shared\minwindef.h(130): warning C4005: 'APIENTRY': macro redefinition [C:\usr\local\opticks\build\graphics\oglrap\OGLRap.vcxproj]
      C:\usr\local\opticks\externals\include\GLFW/glfw3.h(85): note: see previous definition of 'APIENTRY'





OSX 10.11 sensitivity to clang version
----------------------------------------

* https://github.com/glfw/glfw/issues/559


GLFW 3.1.1 OSX 10.5.8 compilation failure
------------------------------------------

GLFW 3.1.1 in cocoa_platform.h uses CGDisplayModeRef which is an OSX 10.6 ism 

* https://developer.apple.com/library/mac/documentation/GraphicsImaging/Conceptual/QuartzDisplayServicesConceptual/Articles/DisplayModes.html


pkg-config
------------

Uppercased the glfw3.pc in attempt to get oglplus- cmake to find it, 
to no avail. 

::

    delta:oglplustest blyth$ ll $(glfw-idir)/lib/pkgconfig/
    total 8
    -rw-r--r--  1 blyth  staff  422 Mar 24 13:00 glfw3.pc
    drwxr-xr-x  5 blyth  staff  170 Mar 24 13:00 ..

    After renaming pkg-config from commamdline can find it, but not cmake 

    delta:~ blyth$ PKG_CONFIG_PATH=$(glfw-idir)/lib/pkgconfig pkg-config GLFW3 --modversion
    3.1.1

    delta:pkgconfig blyth$ glfw-;glfw-pc GLFW3 --libs --static
    -L/usr/local/env/graphics/glfw/3.1.1/lib -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreFoundation -framework CoreVideo 

    delta:pkgconfig blyth$ glfw-;glfw-pc GLFW3 --libs 
    -L/usr/local/env/graphics/glfw/3.1.1/lib -lglfw3 


::

    PKG_CONFIG_PATH=$(glfw-prefix)/lib/pkgconfig pkg-config --cflags glfw3
    -I/usr/local/env/graphics/glfw/3.1.1/include 


retina control ?  NSHighResolutionCapable
-------------------------------------------

::

    simon:glfw-3.1.1 blyth$ find . -name '*.plist'
    ./CMake/AppleInfo.plist

::

     35         <key>NSHighResolutionCapable</key>
     36         <true/>


* http://stackoverflow.com/questions/20426140/how-to-enable-use-low-resolution-on-retina-display-for-an-application-when-shi




OS X specific CMake options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* http://www.glfw.org/docs/latest/compile.html#compile_options_osx

GLFW_USE_CHDIR determines whether glfwInit changes the current directory of bundled applications to the Contents/Resources directory.

GLFW_USE_MENUBAR determines whether the first call to glfwCreateWindow sets up a minimal menu bar.

GLFW_USE_RETINA determines whether windows will use the full resolution of Retina displays.

GLFW_BUILD_UNIVERSAL determines whether to build Universal Binaries.


runtime control ?
~~~~~~~~~~~~~~~~~~~

An app bundle (rather than a unix executable) supports the setting of 
resolution handling via the info box. How to make app bundle ?

* https://github.com/glfw/glfw-legacy/blob/master/tests/bundle.sh


cmake app bundles
~~~~~~~~~~~~~~~~~~

* http://www.cmake.org/Wiki/BundleUtilitiesExample
* http://www.cmake.org/cmake/help/v3.0/module/BundleUtilities.html



input events
-------------

::

    glfw-bcd
    cd tests
    ./events   # blank window demo of all events 

    delta:tests blyth$ pwd
    /usr/local/env/graphics/glfw/glfw-3.1.1.build/tests
    delta:tests blyth$ ./events -h
    Library initialized
    Usage: events [-f] [-h] [-n WINDOWS]
    Options:
      -f use full screen
      -h show this help
      -n the number of windows to create

    00000433 to 2 at 70.437: Cursor position: 302.957031 217.011719
    00000434 to 2 at 70.454: Cursor position: 310.250000 212.843750
    00000435 to 2 at 70.471: Cursor position: 312.332031 211.800781

Observations

* origin of cursor coordinates is the top left of current focused window,
* goes negative to the left and up, increases to right and down 
  and continues beyond window dimensions 



trackball
------------

*  https://github.com/ishikawash/glfw-example/blob/master/normal_map_demo/trackball.hpp



external events, eg messages from UDP,  ZeroMQ
------------------------------------------------

* https://github.com/elmindreda/Wendy


GLEQ : simple single header event queue for GLFW 3
----------------------------------------------------------

By the author of GLFW

* https://github.com/elmindreda/gleq
* https://github.com/elmindreda/gleq/blob/master/test.c 
* https://github.com/elmindreda/gleq/blob/master/test.c 

When a window is tracked by GLEQ it handles setting all the 
callbacks, which populate a unionized event struct. This
yields a clean interface for consuming input events which 
keeps the callbacks out of sight.






EOU
}




glfw-env(){      opticks- ;  }


glfw-info(){ cat << EOI

   glfw-dir : $(glfw-dir)


EOI
}


glfw-dir(){  echo $(opticks-prefix)/externals/glfw/$(glfw-name) ; }
glfw-bdir(){ echo $(glfw-dir).build ; }
glfw-idir(){ echo $(opticks-prefix)/externals ; }
glfw-prefix(){ echo $(opticks-prefix)/externals ; }

glfw-cd(){  cd $(glfw-dir); }
glfw-bcd(){ cd $(glfw-bdir); }
glfw-icd(){ cd $(glfw-idir); }


glfw-pc-notes(){ cat << EON

  fix messing up 
  had to change lib to lib64

EON
}

glfw-pc(){ 
   local msg="=== $FUNCNAME :"

   local path0="$OPTICKS_PREFIX/externals/lib/pkgconfig/glfw3.pc"
   local path1="$OPTICKS_PREFIX/externals/lib64/pkgconfig/glfw3.pc"
   local path
   if [ -f "$path0" ]; then  
       path=$path0
   elif [ -f "$path1" ]; then
       path=$path1
   else
       path=/dev/null
   fi 

   local path2="$OPTICKS_PREFIX/externals/lib/pkgconfig/OpticksGLFW.pc"
   local rc=0
   if [ -f "$path2" -a ! -f "$path" ]; then  
       echo $msg path2 exists already $path2
   elif [ -f "$path" ]; then  
       $(opticks-home)/bin/pc.py $path --fix
       rc=$?
       mv $path $path2
   else
       echo $msg no such path $path
       rc=1 
   fi  
   return $rc
}


#glfw-pcc(){
#  PKG_CONFIG_PATH=$(glfw-prefix)/lib/pkgconfig pkg-config GLFW3 $*
#}
#
#glfw-pcc-kludge(){
#   cd $(glfw-prefix)/lib/pkgconfig
#   mv glfw3.pc GLFW3.pc
#}

#glfw-version(){ echo 3.1.1 ; }
glfw-version(){ echo 3.3.2 ; }
glfw-version-grep(){ grep "^#define GLFW_VERSION" $(glfw-dir)/include/GLFW/glfw3.h ; }

glfw-name(){ echo glfw-$(glfw-version) ; }
glfw-url(){  
   case $(glfw-version) in
      3.1.1) echo http://downloads.sourceforge.net/project/glfw/glfw/$(glfw-version)/$(glfw-name).zip ;;
          *) echo https://github.com/glfw/glfw/releases/download/$(glfw-version)/$(glfw-name).zip ;;
   esac
}


glfw-dist(){ echo $(dirname $(glfw-dir))/$(basename $(glfw-url)) ; }

glfw-edit(){  vi $(opticks-home)/cmake/Modules/FindGLFW.cmake ; }


glfw-get(){
   local dir=$(dirname $(glfw-dir)) &&  mkdir -p $dir && cd $dir

   local rc=0
   local url=$(glfw-url)
   local zip=$(basename $url)
   local opt=$( [ -n "${VERBOSE}" ] && echo "" || echo "-q" )

   local nam=${zip/.zip}
   [ ! -f "$zip" ] && opticks-curl $url
   [ ! -d "$nam" ] && unzip $opt $zip 
   
   [ ! -d "$nam" ] && rc=1
 
   #ln -sfnv $nam glfw
   # try to avoid having to pass the version around via symbolic link

   return $rc
}


glfw-wipe(){
  local bdir=$(glfw-bdir)
  rm -rf $bdir
}

glfw-manifest(){ cat << EOP
lib64/libglfw.so 
lib64/libglfw.so.3 
lib64/libglfw.so.3.3
lib64/pkgconfig/glfw3.pc
lib64/cmake/glfw3
include/GLFW
EOP
}

glfw-manifest-wipe(){
  local pfx=$(glfw-prefix)
  cd $pfx 
  [ $? -ne 0 ] && return 1

  local rel
  echo "# $FUNCNAME "
  echo "cd $PWD" 
  for rel in $(glfw-manifest) 
  do
      if [ -d "$rel" ]; then 
          echo rm -rf \"$rel\"
      elif [ -f "$rel" ]; then 
          echo rm -f \"$rel\"
      fi      
  done
 

   

}


glfw-cmake(){
  local iwd=$PWD
   
  #local pref=GLVND ;  ## doesnt build : linking problem
  local pref=LEGACY ;
  local rc=0 

  local bdir=$(glfw-bdir)
  if [ -d "$bdir" ]; then
      glfw-bcd
  else
      mkdir -p $bdir
      glfw-bcd
      cmake -G "$(opticks-cmake-generator)" \
                   -DBUILD_SHARED_LIBS=ON \
                   -DOpenGL_GL_PREFERENCE=$pref \
                   -DCMAKE_INSTALL_PREFIX=$(glfw-prefix) \
                   $(glfw-dir)

      rc=$? 
  fi 

  cd $iwd
  return $rc 
}
glfw-check(){ echo TEST6 ; }

glfw-configure()
{
   glfw-wipe
   glfw-cmake $*
}


glfw-config()
{
   echo Debug
}

glfw-make(){
  local iwd=$PWD
  local rc 
  glfw-bcd

  #make $* 
  cmake --build . --config $(glfw-config) --target ${1:-install}
  rc=$?

  cd $iwd
  return $rc 
}


glfw--()
{
   local msg="=== $FUNCNAME :"
   glfw-get
   [ $? -ne 0 ] && echo $msg get FAIL && return 1

   glfw-cmake
   [ $? -ne 0 ] && echo $msg cmake FAIL && return 2

   glfw-make install
   [ $? -ne 0 ] && echo $msg make/instal FAIL && return 3

   #glfw-pc
   #[ $? -ne 0 ] && echo $msg pc FAIL && return 4

   return 0
}



glfw-setup(){ cat << EOS
# $FUNCNAME 
EOS
}





glfw-keyname-notes(){ cat << EON
$FUNCNAME
===================

See examples/UseOpticksGLFW/UseOpticksGLFW.cc

For system installs /usr/include/GLFW/glfw3.h


EON
}


glfw-header(){
   ## assume system installs on Linux 
   case $(uname) in 
      Linux)  echo /usr/include/GLFW/glfw3.h ;; 
      Darwin) echo $(glfw-dir)/include/GLFW/glfw3.h ;; 
   esac
}

glfw-keyname-()
{
    grep \#define\ GLFW_KEY $(glfw-header)
}
glfw-keyname()
{
    cat << EOH
// generated by glfw-;$FUNCNAME $(date)
const char* glfw_keyname( int key )
{
    const char* s = NULL ; 
    switch(key)
    {
EOH
    local line
    $FUNCNAME- | while read line ; do 
       $FUNCNAME-line $line
    done 

    cat << EOT
    }
    return s ; 
}
EOT
}
glfw-keyname-line()
{
    #printf "// %-30s : %-10s \n" $2 $3
    [ "$2" == "GLFW_KEY_LAST" ] && return 
    printf "        case %-30s : s = %-30s ; break ; \n" $2 \"$2\"    
}



