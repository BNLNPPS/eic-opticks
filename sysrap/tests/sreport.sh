#!/bin/bash 
usage(){ cat << EOU
sreport.sh : summarize SEvt metadata into eg ALL0_sreport SREPORT_FOLD 
========================================================================

The output NPFold and summary NPFold from several scripts are managed 
by this sreport.sh script. 

+-----------------------------------------+---------------+
|  bash script that creates SEvt NPFold   |   JOB tags    |      
+=========================================+===============+
| ~/opticks/CSGOptiX/cxs_min.sh           |  N3           |
+-----------------------------------------+---------------+
| ~/opticks/g4cx/tests/G4CXTest_GEOM.sh   |  N2,N4        |
+-----------------------------------------+---------------+
| ~/j/okjob.sh + ~/j/jok.bash             |  L1,N1        |
+-----------------------------------------+---------------+

::

   ~/opticks/sysrap/tests/sreport.sh

   JOB=N3 ~/opticks/sysrap/tests/sreport.sh           ## summarize SEvt folders
   JOB=N3 ~/opticks/sysrap/tests/sreport.sh grab      ## from remote to local 
   JOB=N3 ~/opticks/sysrap/tests/sreport.sh ana       ## local plotting 


   JOB=N7 ~/o/sreport.sh desc_info
   JOB=A7 ~/o/sreport.sh desc_info



   PLOT=Substamp_ONE_Delta PICK=A ~/o/sreport.sh
   PLOT=Substamp_ONE_Delta PICK=B ~/o/sreport.sh

   PLOT=Substamp_ONE_Etime PICK=A ~/o/sreport.sh
   PLOT=Substamp_ONE_Etime PICK=B ~/o/sreport.sh
   
   PLOT=Substamp_ONE_maxb_scan PICK=A ~/o/sreport.sh
   PLOT=Substamp_ONE_maxb_scan PICK=B ~/o/sreport.sh

   PLOT=Ranges_ONE ~/o/sreport.sh
   PLOT=Ranges_SPAN ~/o/sreport.sh

   PLOT=Substamp_ALL_Etime_vs_Photon ~/o/sreport.sh
   PLOT=Substamp_ALL_Hit_vs_Photon   ~/o/sreport.sh
   PLOT=Substamp_ALL_RATIO_vs_Photon ~/o/sreport.sh

   PLOT=Subprofile_ONE PICK=A ~/o/sreport.sh
   PLOT=Subprofile_ONE PICK=B ~/o/sreport.sh
 
   PLOT=Subprofile_ALL ~/o/sreport.sh
   PLOT=Runprof_ALL    ~/o/sreport.sh    




**JOB**
   selects the output and summary folders of various scripts

**build**
   standalone build of the sreport binary, CAUTION the binary is also built 
   and installed by the standard "om" build 

**run**  
   sreport loads the SEvt subfolders "p001" "n001" etc beneath 
   the invoking directory in NoData(metadata only) mode and 
   writes a summary NPFold into SREPORT_FOLD directory 
   
   NB DEV=1 uses the standalone binary built by this script, and 
   not defining DEV uses the CMake standardly built and installed sreport binary 

**grab**
   grab command just rsyncs the summary SREPORT_FOLD back to laptop for 
   metadata plotting without needing to transfer the potentially 
   large SEvt folders. 

**ana**
   python plotting using ~/opticks/sysrap/tests/sreport.py::

       substamp=1 ~/opticks/sysrap/tests/sreport.sh ana 
       subprofile=1 ~/opticks/sysrap/tests/sreport.sh ana 


**mpcap**
   capture plot screenshots::

       PLOT=Substamp_ONE_maxb_scan PICK=A ~/opticks/sreport.sh        ## display plot form one tab
       PLOT=Substamp_ONE_maxb_scan PICK=A ~/opticks/sreport.sh mpcap  ## capture from another
       PLOT=Substamp_ONE_maxb_scan PICK=A PUB=some_anno ~/opticks/sreport.sh mppub  ## publish 


Note that *sreport* executable can be used without this script 
by invoking it from appropriate directories, examples are shown below.

Invoking Directory 
   /data/blyth/opticks/GEOM/J23_1_0_rc3_ok0/CSGOptiXSMTest/ALL0
Summary "SREPORT_FOLD" Directory 
   /data/blyth/opticks/GEOM/J23_1_0_rc3_ok0/CSGOptiXSMTest/ALL0_sreport

EOU
}

SDIR=$(dirname $(realpath $BASH_SOURCE))
# sreport does different things depending on the invoking directory : so caution with cd

dbg__ () 
{ 
    case $(uname) in 
        Darwin)
            lldb__ $*
        ;;
        Linux)
            gdb__ $*
        ;;
    esac
}

lldb__ () 
{ 
    : macOS only - this function requires LLDB envvar to provide the path;
    : to the lldb application within the appropriate Xcode.app resources eg;
    local BINARY=$1;
    shift;
    local ARGS=$*;
    local H="$HEAD";
    local B;
    local bp;
    echo HEAD $HEAD;
    echo TAIL $TAIL;
    if [ -z "$BP" ]; then
        B="";
    else
        B="";
        for bp in $BP;
        do
            B="$B -o \"b $bp\" ";
        done;
        B="$B -o b";
        [ -n "$BX" ] && B="$B -o \"$BX\" ";
    fi;
    local T="$TAIL";
    local def_lldb=/Applications/Xcode/Xcode.app/Contents/Developer/usr/bin/lldb;
    local runline="${LLDB:-$def_lldb} -f ${BINARY} $H $B $T -- ${ARGS}";
    echo $runline;
    eval $runline
}


gdb__ () 
{ 
    : opticks/opticks.bash prepares and invokes gdb - sets up breakpoints based on BP envvar containing space delimited symbols;
    if [ -z "$BP" ]; then
        H="";
        B="";
        T="-ex r";
    else
        H="-ex \"set breakpoint pending on\"";
        B="";
        for bp in $BP;
        do
            B="$B -ex \"break $bp\" ";
        done;
        T="-ex \"info break\" -ex r";
    fi;
    local runline="gdb $H $B $T --args $* ";
    echo $runline;
    date;
    eval $runline;
    date
}




name=sreport
src=$SDIR/$name.cc
script=$SDIR/$name.py

DEV=0  ## set to 1 to standalone build the sreport binary before use
if [ "$DEV" = "0" ]; then
    bin=$name                                   ## standard binary 
    defarg="run_info_ana"
else
    bin=${TMP:-/tmp/$USER/opticks}/$name/$name    ## standalone binary
    #defarg="build_run_info_ana"
    defarg="build_run_info_noa"
fi

[ -n "$PLOT" ] && defarg="ana"
arg=${1:-$defarg}


if [ "$bin" == "$name" ]; then
    echo $BASH_SOURCE : using standard CMake built and installed binary 
else
    mkdir -p $(dirname $bin)
fi

source $HOME/.opticks/GEOM/GEOM.sh 

#job=N5
#job=S5
#job=Y1
job=N7   
#job=A7   
#job=S7  

JOB=${JOB:-$job}
LAB="Undefined"

DIR=unknown 
case $JOB in 
  L1) DIR=/hpcfs/juno/junogpu/blyth/tmp/GEOM/$GEOM/jok-tds/ALL0 ;;
  N1) DIR=/data/blyth/opticks/GEOM/$GEOM/jok-tds/ALL0 ;;
  N2) DIR=/data/blyth/opticks/GEOM/$GEOM/G4CXTest/ALL0 ;;
  N4) DIR=/data/blyth/opticks/GEOM/$GEOM/G4CXTest/ALL2 ;;
  N5) DIR=/data/blyth/opticks/GEOM/$GEOM/G4CXTest/ALL3 ;;   ## blyth:Debug 
  S5) DIR=/data/simon/opticks/GEOM/$GEOM/G4CXTest/ALL3 ;;   ## simon:Release TODO 

  N3) DIR=/data/blyth/opticks/GEOM/$GEOM/CSGOptiXSMTest/ALL2 ;;
  N6) DIR=/data/blyth/opticks/GEOM/$GEOM/CSGOptiXSMTest/ALL3 ;;
  N7) DIR=/data/blyth/opticks/GEOM/$GEOM/CSGOptiXSMTest/ALL1 ; LAB="TITAN RTX : Debug" ;; 
  A7) DIR=/data1/blyth/tmp/GEOM/$GEOM/CSGOptiXSMTest/ALL1    ; LAB="Ada RTX 5000 : Debug" ;;
  S7) DIR=/data/simon/opticks/GEOM/$GEOM/CSGOptiXSMTest/ALL1 ; LAB="TITAN RTX : Release" ;; 

  Y1) DIR=/tmp/ihep/opticks/GEOM/$GEOM/jok-tds/ALLLUT_1_ENE_-1_OIM_1_GUN_5 ;; ## 'yuxiang': Release TODO
esac

export STEM=${JOB}_${PLOT}_${PICK}
export SREPORT_FOLD=${DIR}_${name}   ## SREPORT_FOLD is output directory used by binary, export it for python 
export MODE=2                        ## 2:matplotlib plotting 

vars="0 BASH_SOURCE arg defarg DEV bin script SDIR JOB LAB DIR SREPORT_FOLD MODE name STEM PLOT PICK"

if [ "${arg/info}" != "$arg" ]; then
    for var in $vars ; do printf "%25s : %s \n" "$var" "${!var}" ; done 
fi 

if [ "${arg/build}" != "$arg" ]; then 
    gcc $src -g -std=c++11 -lstdc++ -I$SDIR/.. -o $bin 
    #gcc $src -O3 -DNDEBUG -std=c++11 -lstdc++ -I$SDIR/.. -o $bin 
    [ $? -ne 0 ] && echo $BASH_SOURCE : build error && exit 1
fi





if [ "${arg/dbg}" != "$arg" ]; then 
    cd $DIR
    [ $? -ne 0 ] && echo $BASH_SOURCE : NO SUCH DIRECTORY : JOB  $JOB DIR $DIR && exit 0 

    dbg__ $bin
    [ $? -ne 0 ] && echo $BASH_SOURCE : dbg error && exit 3
fi

if [ "${arg/run}" != "$arg" ]; then   ## create report from SEvt metadata
    cd $DIR
    [ $? -ne 0 ] && echo $BASH_SOURCE : NO SUCH DIRECTORY : JOB  $JOB DIR $DIR && exit 0 
    $bin
    [ $? -ne 0 ] && echo $BASH_SOURCE : run error && exit 3
fi

if [ "${arg/desc}" != "$arg" ]; then   ## load existing report and present it 
    cd $SREPORT_FOLD
    [ $? -ne 0 ] && echo $BASH_SOURCE : NO SUCH DIRECTORY : JOB  $JOB DIR $DIR SREPORT_FOLD $SREPORT_FOLD  && exit 0 
    $bin
    [ $? -ne 0 ] && echo $BASH_SOURCE : desc error && exit 3
fi


if [ "${arg/grab}" != "$arg" ]; then 
    echo $BASH_SOURCE : grab SREPORT_FOLD $SREPORT_FOLD 
    source $OPTICKS_HOME/bin/rsync.sh $SREPORT_FOLD
    [ $? -ne 0 ] && echo $BASH_SOURCE : grab error && exit 4
fi

if [ "${arg/noa}" != "$arg" ]; then 
    echo $BASH_SOURCE : noa : no analysis exit 
    exit 0
fi

if [ "${arg/ana}" != "$arg" ]; then 
    export COMMANDLINE="JOB=$JOB PLOT=$PLOT ~/o/sreport.sh"
    ${IPYTHON:-ipython} --pdb -i $script
    [ $? -ne 0 ] && echo $BASH_SOURCE : ana error && exit 3
fi

if [ "${arg/info}" != "$arg" ]; then
    for var in $vars ; do printf "%25s : %s \n" "$var" "${!var}" ; done 
fi 

if [ "$arg" == "mpcap" -o "$arg" == "mppub" ]; then
    export CAP_BASE=$SREPORT_FOLD/figs
    export CAP_REL=cxs_min
    export CAP_STEM=$STEM
    case $arg in  
       mpcap) source mpcap.sh cap  ;;  
       mppub) source mpcap.sh env  ;;  
    esac
    if [ "$arg" == "mppub" ]; then 
        source epub.sh 
    fi  
fi 

exit 0 

