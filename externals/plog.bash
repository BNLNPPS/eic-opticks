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

# === func-gen- : tools/plog/plog fgp externals/plog.bash fgn plog fgh tools/plog
plog-src(){      echo externals/plog.bash ; }
plog-source(){   echo ${BASH_SOURCE:-$(opticks-home)/$(plog-src)} ; }
plog-vi(){       vi $(plog-source) ; }
plog-usage(){ cat << EOU

PLOG : Simple header only logging that works across DLLs
============================================================


old chestnut appender assert issue 
------------------------------------

* https://github.com/SergiusTheBest/plog#share-log-instances-across-modules-exe-dll-so-dylib

Looks like the first test release used an old cached plog. 
Not the updated new one with PLOG_LOCAL functionality. 
Thats the danger with caches. You have to remember to update them or clear them. 



clone an older plog
----------------------

::

    epsilon:tmp blyth$ git clone -b 1.1.4 https://github.com/SergiusTheBest/plog
    Cloning into 'plog'...
    remote: Enumerating objects: 3092, done.
    remote: Counting objects: 100% (427/427), done.
    remote: Compressing objects: 100% (202/202), done.
    remote: Total 3092 (delta 195), reused 389 (delta 175), pack-reused 2665
    Receiving objects: 100% (3092/3092), 515.63 KiB | 197.00 KiB/s, done.
    Resolving deltas: 100% (1729/1729), done.
    Note: checking out 'dcbcca75faccfbde3ba4aae85a185d042af5a185'.

    You are in 'detached HEAD' state. You can look around, make experimental
    changes and commit them, and you can discard any commits you make in this
    state without impacting any branches by performing another checkout.

    If you want to create a new branch to retain commits you create, you may
    do so (now or later) by using -b with the checkout command again. Example:

      git checkout -b <new-branch-name>





plog logging levels
---------------------

plog/Severity.h::

      3 namespace plog
      4 {
      5     enum Severity
      6     {
      7         none = 0,
      8         fatal = 1,
      9         error = 2,
     10         warning = 3,
     11         info = 4,
     12         debug = 5,
     13         verbose = 6
     14     };

plog/Logger.h::

     37         bool checkSeverity(Severity severity) const
     38         {
     39             return severity <= m_maxSeverity;
     40         }


m_maxSeverity 
   logger level, 
   typical Opticks usage fixes this at "info:4" for each shared lib
   and "verbose:6" in the main

severity 
   record level, coming from LOG(LEVEL) lines in the code, 
   typical Opticks usage (with SLOG::EnvLevel) defaults this to "debug:5" 
   Subsequently logging from individual classes are actuated by defining envvars, eg::

      export G4CXOpticks=INFO
   
   That changes record levels to "info:4" which is "<= m_maxSeverity"
   so logging messages appear. 
 
NB the decision to emit a logging record is::

    recordLevel <= loggerLevel 

   




Upstream Update : Oct 3, 2022
--------------------------------

* https://github.com/simoncblyth/plog
* https://github.com/SergiusTheBest/plog

I deleted my old fork of plog and forked again from the 
upstream latest under the same name. 
    
There was only one commit different in my old fork with 
IF_LOG logic inversion, that avoided compilation 
warnings from a common construct::

    if(cond) LOG(info) << "hello"

Subsequently I have changed all occurrences of the above construct 
within active Opticks packages to use:: 

    LOG_IF(info, cond) << "hello"  ; 

After doing that there is no need for the deviation, so can use 
the latest plog. 

However caution dictates to pin it at current status, for better 
control over future changes. 

PLOG_LOCAL symbol visibility control
--------------------------------------

The primary reason for the update is the new PLOG_LOCAL 
symbol visibility control which makes plog fully usable 
within projects that do not control symbol visibility 
with compiler options::

   -fvisibility=hidden -fvisibility-inlines-hidden

This makes it possible to extend Opticks style logging control 
to packages that are integrated with Opticks, like JUNOSW. 


old IF_LOG logic inversion : PR that was not accepted by Sergio
---------------------------------------------------------------------

* https://github.com/SergiusTheBest/plog/pull/92

::


    -#define IF_LOG_(instance, severity)     if (!plog::get<instance>() || !plog::get<instance>()->checkSeverity(severity)) {;} else
    +#define IF_LOG_(instance, severity)     if (plog::get<instance>() && plog::get<instance>()->checkSeverity(severity)) 
     #define IF_LOG(severity)                IF_LOG_(PLOG_DEFAULT_INSTANCE, severity)


About windows use
--------------------

Inclusion of plog/Log.h brings in Windef.h that does::

   #define near 
   #define far

So windows dictates:

* you cannot have identifiers called "near" or "far"



::

    In file included from /Users/blyth/env/numerics/npy/numpy.hpp:40:
    /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../include/c++/v1/fstream:864:20: error: no member named 'plog' in
          'std::__1::codecvt_base'; did you mean simply 'plog'?
            if (__r == codecvt_base::error)
                       ^

Resolve by moving the BLog.hh include after NPY.hpp::

     10 #include "NPY.hpp"
     12 #include "BLog.hh"



fixed this by bringing my plog fork uptodate
-----------------------------------------------

:: 

    epsilon:externals blyth$ opticks--
    Scanning dependencies of target SysRap
    [  0%] Building CXX object sysrap/CMakeFiles/SysRap.dir/SYSRAP_LOG.cc.o
    In file included from /Users/blyth/opticks/sysrap/SYSRAP_LOG.cc:2:
    In file included from /usr/local/opticks/externals/plog/include/plog/Log.h:7:
    In file included from /usr/local/opticks/externals/plog/include/plog/Record.h:3:
    /usr/local/opticks/externals/plog/include/plog/Util.h:89:48: warning: 'syscall' is deprecated: first deprecated in macOS 10.12 - syscall(2) is
          unsupported; please switch to a supported interface. For SYS_kdebug_trace use kdebug_signpost(). [-Wdeprecated-declarations]
                return static_cast<unsigned int>(::syscall(SYS_thread_selfid));
                                                   ^
    /usr/include/unistd.h:745:6: note: 'syscall' has been explicitly marked deprecated here
    int      syscall(int, ...);
             ^
    1 warning generated.
    [  0%] Building CXX object sysrap/CMakeFiles/SysRap.dir/PLOG.cc.o
    In file included from /Users/blyth/opticks/sysrap/PLOG.cc:7:



Logging with embedded Opticks
-------------------------------

Current approach takes too much real estate in main with include + macro invoke
for every package you want to see logging from, see eg g4ok/test/G4OKTest.cc

How to shrink that ? Whats the biz with planting symbols in the separate proj libs ?

Can I make it invisible, controlled via envvar ?


Review how plog is implemented, in order to help with debugging misbehaviour when used from externals
--------------------------------------------------------------------------------------------------------

* https://github.com/simoncblyth/plog



plog/Severity.h::

     03 namespace plog
      4 {
      5     enum Severity
      6     {
      7         none = 0,
      8         fatal = 1,
      9         error = 2,
     10         warning = 3,
     11         info = 4,
     12         debug = 5,
     13         verbose = 6
     14     };
     ..
     49 }

plog/Log.h::

     * LOG macro : if record severity <= logger severity append the record  

     33 //////////////////////////////////////////////////////////////////////////
     34 // Log severity level checker
     35 
     36 #define IF_LOG_(instance, severity)     if (plog::get<instance>() && plog::get<instance>()->checkSeverity(severity)) 
     37 #define IF_LOG(severity)                IF_LOG_(PLOG_DEFAULT_INSTANCE, severity)
     38 
     39 //////////////////////////////////////////////////////////////////////////
     40 // Main logging macros
     41 
     42 #define LOG_(instance, severity)        IF_LOG_(instance, severity) (*plog::get<instance>()) += plog::Record(severity, PLOG_GET_FUNC(), __LINE__, PLOG_GET_FILE(), PLOG_GET_THIS())
     43 #define LOG(severity)                   LOG_(PLOG_DEFAULT_INSTANCE, severity)


plog/Init.h::

     08 namespace plog
     09 {
     ..
     26     template<int instance>
     27     inline Logger<instance>& init(Severity maxSeverity = none, IAppender* appender = NULL)
     28     {
     29         static Logger<instance> logger(maxSeverity);
     30         return appender ? logger.addAppender(appender) : logger;
     31     }
     ..
     95 }

plog/Logger.h::

     * Logger ISA IAppender itself and contains a vector of "child" IAppender
     * Logger::write appends only for records with severity <= Logger::m_maxSeverity
     * Logger::operator+= writes record to all child IAppender but not to self 

     10 namespace plog
     11 {
     12     template<int instance>
     13     class Logger : public util::Singleton<Logger<instance> >, public IAppender
     14     {
     15     public:
     16         Logger(Severity maxSeverity = none) : m_maxSeverity(maxSeverity)
     17         {
     18         }
     19 
     20         Logger& addAppender(IAppender* appender)
     21         {
     22             assert(appender != this);
     23             m_appenders.push_back(appender);
     24             return *this;
     25         }
     .. 
     37         bool checkSeverity(Severity severity) const
     38         {
     39             return severity <= m_maxSeverity;
     40         }
     41 
     42         virtual void write(const Record& record)
     43         {
     44             if (checkSeverity(record.getSeverity()))
     45             {
     46                 *this += record;
     47             }
     48         }
     49 
     50         void operator+=(const Record& record)
     51         {
     52             for (std::vector<IAppender*>::iterator it = m_appenders.begin(); it != m_appenders.end(); ++it)
     53             {
     54                 (*it)->write(record);
     55             }
     56         }
     57 
     58     private:
     59         Severity m_maxSeverity;
     60         std::vector<IAppender*> m_appenders;
     61     };
     62 
     63     template<int instance>
     64     inline Logger<instance>* get()
     65     {
     66         return Logger<instance>::getInstance();
     67     }
     68 
     69     inline Logger<PLOG_DEFAULT_INSTANCE>* get()
     70     {
     71         return Logger<PLOG_DEFAULT_INSTANCE>::getInstance();
     72     }
     73 }


plog/Appenders/IAppender.h::

     * place where Records go 

     01 #pragma once
      2 #include <plog/Record.h>
      3 
      4 namespace plog
      5 {
      6     class IAppender
      7     {
      8     public:
      9         virtual ~IAppender()
     10         {
     11         }
     12 
     13         virtual void write(const Record& record) = 0;
     14     };
     15 }
         



EOU
}
plog-env(){      opticks- ;  }
plog-dir(){  echo $(opticks-prefix)/externals/plog ; }
plog-prefix(){  echo $(plog-dir) ; }

plog-idir(){ echo $(opticks-prefix)/externals/plog/include/plog ; }
plog-ifold(){ echo $(opticks-prefix)/externals/plog/include ; }
plog-pc-path(){ echo $(opticks-prefix)/externals/lib/pkgconfig/PLog.pc ; }

plog-c(){    cd $(plog-dir); }
plog-cd(){   cd $(plog-dir); }
plog-icd(){  cd $(plog-idir); }


plog-url-upstream(){  echo https://github.com/SergiusTheBest/plog ; }
plog-url-pinned(){  echo https://github.com/simoncblyth/plog.git ; }
plog-url(){  echo ${PLOG_URL:-$(plog-url-pinned)} ; }


plog-wipe(){
   local iwd=$PWD
   local dir=$(plog-dir)
   cd $(dirname $dir)
   rm -rf plog
   cd $iwd
}

plog-get(){
   local iwd=$PWD
   local msg="=== $FUNCNAME :"
   local dir=$(dirname $(plog-dir)) &&  mkdir -p $dir && cd $dir
   local url=$(plog-url)
   echo $msg url $url 
   if [ ! -d plog ]; then  
       opticks-git-clone $url 
   else
       echo $msg plog already cloned : use plog-wipe and try again to force it
   fi 
   cd $iwd
}

plog--()
{
   local msg="=== $FUNCNAME :"
   plog-get
   [ $? -ne 0 ] && echo $msg get FAIL && return 1
   #plog-pc
   #[ $? -ne 0 ] && echo $msg pc FAIL && return 2
   return 0
}

plog--upstream()
{
   PLOG_URL=$(plog-url-upstream) plog--
}
plog-p2s()
{
    perl -pi -e 's/PLOG::EnvLevel/SLOG::EnvLevel/g' *.cc *.hh *.hpp *.cpp
    perl -pi -e 's/PLOG.hh/SLOG.hh/'                *.cc *.hh *.hpp *.cpp
    perl -pi -e 's/PLOG::instance/SLOG::instance/g' *.cc *.hh *.hpp *.cpp 
    perl -pi -e 's/PLOG_INIT/SLOG_INIT/g'           *.cc *.hh *.hpp *.cpp 
    perl -pi -e 's/PLOG_CHECK/SLOG_CHECK/g'           *.cc *.hh *.hpp *.cpp 
}



plog-pc-scratch-(){ cat << EOS
#prefix=\${pcfiledir}/../../..
#includedir=\${prefix}/externals/plog/include


EOS
}

plog-pc-(){ cat << EOP

# use --define-prefix to set as the grandparent of the pkgconfig 
# when using xlib trick that is the canonical /usr/local/opticks prefix

prefix=$(opticks-prefix)
includedir=\${prefix}/externals/plog/include

Name: plog
Description: Logging 
Version: 0.1.0

Cflags:  -I\${includedir}
Libs: -lstdc++
Requires: 

EOP
}


plog-pc(){ 
   local msg="=== $FUNCNAME :"
   local path=$(plog-pc-path)
   local dir=$(dirname $path)

   [ ! -d "$dir" ] && echo $msg creating dir $dir && mkdir -p $dir 
   echo $msg $path
   plog-pc- > $path 
}

plog-pc-info(){ cat << EOI

   plog-pc-path : $(plog-pc-path)

EOI

   cat $(plog-pc-path)


}






plog-edit(){  vi $(opticks-home)/cmake/Modules/FindPLog.cmake ; }


plog-old-genlog-cc(){ 

   local tag=${1:-NPY}
   cat << EOL

#include <plog/Log.h>

#include "${tag}_LOG.hh"
#include "PLOG_INIT.hh"
#include "PLOG.hh"
       
void ${tag}_LOG::Initialize(void* whatever, int level )
{
    PLOG_INIT(whatever, level);
}
void ${tag}_LOG::Check(const char* msg)
{
    PLOG_CHECK(msg);
}

EOL
}

plog-old-genlog-hh(){ 
   local tag=${1:-NPY}
   cat << EOL

#pragma once
#include "${tag}_API_EXPORT.hh"

#define ${tag}_LOG__ \
 { \
    ${tag}_LOG::Initialize(plog::get(), PLOG::instance->prefix_parse( info, "${tag}") ); \
 } \


#define ${tag}_LOG_ \
{ \
    ${tag}_LOG::Initialize(plog::get(), plog::get()->getMaxSeverity() ); \
} \


class ${tag}_API ${tag}_LOG {
   public:
       static void Initialize(void* whatever, int level );
       static void Check(const char* msg);
};

EOL
}









plog-genlog-hh(){ 
   local tag=${1:-NPY}
   cat << EOL

#pragma once
#include "${tag}_API_EXPORT.hh"

#define ${tag}_LOG__ \
 { \
    ${tag}_LOG::Initialize(PLOG::instance->prefixlevel_parse( info, "${tag}"), plog::get(), NULL ); \
 } \


#define ${tag}_LOG_ \
{ \
    ${tag}_LOG::Initialize(plog::get()->getMaxSeverity(), plog::get(), NULL ); \
} \

class ${tag}_API ${tag}_LOG {
   public:
       static void Initialize(int level, void* app1, void* app2 );
       static void Check(const char* msg);
};

EOL
}



plog-genlog-cc(){ 

   local tag=${1:-NPY}
   cat << EOL

#include <plog/Log.h>

#include "${tag}_LOG.hh"
#include "PLOG_INIT.hh"
#include "PLOG.hh"
       
void ${tag}_LOG::Initialize(int level, void* app1, void* app2 )
{
    PLOG_INIT(level, app1, app2);
}
void ${tag}_LOG::Check(const char* msg)
{
    PLOG_CHECK(msg);
}

EOL
}


plog-genlog(){
  local cmd=$1 
  local msg="=== $FUNCNAME :"
  local ex=$(ls -1 *_API_EXPORT.hh 2>/dev/null) 
  [ -z "$ex" ] && echo $msg ERROR there is no export in PWD $PWD : run from project source with the correct tag : not $tag && return 

  local tag=${ex/_API_EXPORT.hh} 
  local cc=${tag}_LOG.cc
  local hh=${tag}_LOG.hh

  if [ "$cmd" == "FORCE" ] ; then 
     rm -f $cc
     rm -f $hh
  fi

  [ -f "$cc" -o -f "$hh" ] && echo $msg cc $cc or hh $hh exists already : delete to regenerate && return  

  echo $msg tag $tag generating cc $cc and hh $hh 

  plog-genlog-cc $tag > $cc
  plog-genlog-hh $tag > $hh

  echo $msg remember to commit and add to CMakeLists.txt 
}


plog-inplace-edit(){
   perl -pi -e 's,BLog\.hh,PLOG.hh,g' *.cc && rm *.cc.bak
}


plog-t-(){ cat << EOC

#include <plog/Log.h> // Step1: include the header.

int main()
{
    plog::init(plog::debug, "Hello.txt"); // Step2: initialize the logger.

    // Step3: write log messages using a special macro. 
    // There are several log macros, use the macro you liked the most.

    LOGD << "Hello log!"; // short macro
    LOG_DEBUG << "Hello log!"; // long macro
    LOG(plog::debug) << "Hello log!"; // function-style macro

    int verbose = 5 ; 
    if(verbose > 3) LOGD << "logging in short causes dangling else " ; 
       

    return 0;
}

EOC
}

plog-t()
{
   local tmp=/tmp/$USER/opticks/$FUNCNAME
   mkdir -p $tmp
   cd $tmp
   local name=plogtest.cc
   $FUNCNAME- > $name
   cc $name -I$(plog-ifold) -lc++ -o plogtest
   ./plogtest 
   cat Hello.txt

}

plog-t-notes(){ cat << EON

Updating plog ?
=================

macOS 10.13.4 deprecated syscall
------------------------------------

::

    epsilon:plog-t blyth$ plog-;plog-t
    In file included from plogtest.cc:2:
    In file included from /usr/local/opticks/externals/plog/include/plog/Log.h:7:
    In file included from /usr/local/opticks/externals/plog/include/plog/Record.h:3:
    /usr/local/opticks/externals/plog/include/plog/Util.h:89:48: warning: 'syscall' is deprecated: first deprecated in macOS 10.12 - syscall(2) is
          unsupported; please switch to a supported interface. For SYS_kdebug_trace use kdebug_signpost(). [-Wdeprecated-declarations]
                return static_cast<unsigned int>(::syscall(SYS_thread_selfid));
                                                   ^
    /usr/include/unistd.h:745:6: note: 'syscall' has been explicitly marked deprecated here
    int      syscall(int, ...);
             ^
    1 warning generated.
    epsilon:plog-t blyth$ 


Offending line 89::

     82         inline unsigned int gettid()
     83         {
     84 #ifdef _WIN32
     85             return ::GetCurrentThreadId();
     86 #elif defined(__unix__)
     87             return static_cast<unsigned int>(::syscall(__NR_gettid));
     88 #elif defined(__APPLE__)
     89             return static_cast<unsigned int>(::syscall(SYS_thread_selfid));
     90 #endif
     91         }


Note changes in the latest plog:

* https://github.com/SergiusTheBest/plog/blob/master/include/plog/Util.h

::

    #elif defined(__APPLE__)
            uint64_t tid64;
            pthread_threadid_np(NULL, &tid64);
            return static_cast<unsigned int>(tid64);
    #endif



dangling else:  "if(smth) LOG(info) << blah "  
-------------------------------------------------

Old plog macros didnt have this issue

/Volumes/Delta/usr/local/opticks/externals/plog/include/plog/Log.h::

     28 //////////////////////////////////////////////////////////////////////////
     29 // Log severity level checker
     30 
     31 #define IF_LOG_(instance, severity)     if (plog::get<instance>() && plog::get<instance>()->checkSeverity(severity))
     32 #define IF_LOG(severity)                IF_LOG_(PLOG_DEFAULT_INSTANCE, severity)
     33 
     34 //////////////////////////////////////////////////////////////////////////
     35 // Main logging macros
     36 
     37 #define LOG_(instance, severity)        IF_LOG_(instance, severity) (*plog::get<instance>()) += plog::Record(severity, PLOG_GET_FUNC(), __LINE__, PLOG_GET_THIS())
     38 #define LOG(severity)                   LOG_(PLOG_DEFAULT_INSTANCE, severity)
     39 


/usr/local/opticks/externals/plog/include/plog/Log.h::

     34 // Log severity level checker
     35 
     36 #define IF_LOG_(instance, severity)     if (!plog::get<instance>() || !plog::get<instance>()->checkSeverity(severity)) {;} else
     37 #define IF_LOG(severity)                IF_LOG_(PLOG_DEFAULT_INSTANCE, severity)
     38 
     39 //////////////////////////////////////////////////////////////////////////
     40 // Main logging macros
     41 
     42 #define LOG_(instance, severity)        IF_LOG_(instance, severity) (*plog::get<instance>()) += plog::Record(severity, PLOG_GET_FUNC(), __LINE__, PLOG_GET_FILE(), PLOG_GET_THIS())
     43 #define LOG(severity)                   LOG_(PLOG_DEFAULT_INSTANCE, severity)
     44 


::

    36 //#define IF_LOG_(instance, severity)     if (!plog::get<instance>() || !plog::get<instance>()->checkSeverity(severity)) {;} else
     37 #define IF_LOG_(instance, severity)     if (plog::get<instance>() && plog::get<instance>()->checkSeverity(severity)) 

    epsilon:plog blyth$ git diff
    diff --git a/include/plog/Log.h b/include/plog/Log.h
    index cf0a68c..e45669c 100644
    --- a/include/plog/Log.h
    +++ b/include/plog/Log.h
    @@ -33,7 +33,8 @@
     //////////////////////////////////////////////////////////////////////////
     // Log severity level checker
     
    -#define IF_LOG_(instance, severity)     if (!plog::get<instance>() || !plog::get<instance>()->checkSeverity(severity)) {;} else
    +//#define IF_LOG_(instance, severity)     if (!plog::get<instance>() || !plog::get<instance>()->checkSeverity(severity)) {;} else^M
    +#define IF_LOG_(instance, severity)     if (plog::get<instance>() && plog::get<instance>()->checkSeverity(severity)) ^M
     #define IF_LOG(severity)                IF_LOG_(PLOG_DEFAULT_INSTANCE, severity)
     
     //////////////////////////////////////////////////////////////////////////
    epsilon:plog blyth$ 




EON
}


plog-issue-(){ cat << EOI


[  1%] Building CXX object sysrap/CMakeFiles/SysRap.dir/SMap.cc.o
/Users/blyth/opticks/sysrap/SMap.cc:26:9: warning: add explicit braces to avoid dangling else [-Wdangling-else]
        LOG(info) << " value " << std::setw(32) << std::hex << value << std::dec ; 
        ^
/usr/local/opticks/externals/plog/include/plog/Log.h:43:41: note: expanded from macro 'LOG'
#define LOG(severity)                   LOG_(PLOG_DEFAULT_INSTANCE, severity)
                                        ^
/usr/local/opticks/externals/plog/include/plog/Log.h:42:41: note: expanded from macro 'LOG_'
#define LOG_(instance, severity)        IF_LOG_(instance, severity) (*plog::get<instance>()) += plog::Record(severity, PLOG_GET_FUNC(), __LIN...
                                        ^
/usr/local/opticks/externals/plog/include/plog/Log.h:36:124: note: expanded from macro 'IF_LOG_'
#define IF_LOG_(instance, severity)     if (!plog::get<instance>() || !plog::get<instance>()->checkSeverity(severity)) {;} else
                                                                                                                           ^
/Users/blyth/opticks/sysrap/SMap.cc:35:13: warning: add explicit braces to avoid dangling else [-Wdangling-else]
            LOG(info) 


EOI
}


plog-setup(){ cat << EOS
# $FUNCNAME
EOS
}

