#!/bin/bash 
usage(){ cat << EOU
cxs_min.sh : minimal executable and script for shakedown
============================================================

Uses ~oneline main::

     CSGOptiX::SimulateMain();

Usage::

    ~/o/cxs_min.sh
    ~/o/cxs_min.sh info
    ~/o/cxs_min.sh run       ## create SEvt 
    ~/o/cxs_min.sh report    ## summarize SEvt metadata   

Debug::

    BP=SEvt::SEvt               ~/opticks/CSGOptiX/cxs_min.sh
    BP=SEvent::MakeTorchGenstep ~/opticks/CSGOptiX/cxs_min.sh

Analysis/Plotting::

    ~/o/cxs_min.sh grab 
    EVT=A000 ~/o/cxs_min.sh ana

    MODE=2 SEL=1 ~/o/cxs_min.sh ana 
    EVT=A005     ~/o/cxs_min.sh ana 
    EVT=A010     ~/o/cxs_min.sh ana

    PLOT=scatter MODE=3 ~/o/cxs_min.sh pvcap 

Monitor for GPU memory leaks::

    ~/o/sysrap/smonitor.sh build_run  # start monitor

    TEST=large_scan ~/o/cxs_min.sh   

    # CTRL-C smonitor.sh session sending SIGINT to process which saves smonitor.npy

    ~/o/sysrap/smonitor.sh grab  ## back to laptop
    ~/o/sysrap/smonitor.sh ana   ## plot 
    

EOU
}

vars=""
SDIR=$(dirname $(realpath $BASH_SOURCE))


case $(uname) in
   Linux) defarg=run_report_info ;;
   Darwin) defarg=ana ;;
esac

[ -n "$BP" ] && defarg=dbg
[ -n "$PLOT" ] && defarg=ana

arg=${1:-$defarg}


bin=CSGOptiXSMTest
script=$SDIR/cxs_min.py
script_AB=$SDIR/cxs_min_AB.py

source ~/.opticks/GEOM/GEOM.sh   # sets GEOM envvar 

vars="$vars BASH_SOURCE SDIR defarg arg bin script script_AB GEOM"

tmp=/tmp/$USER/opticks
export TMP=${TMP:-$tmp}
export EVT=${EVT:-A000}
export BASE=$TMP/GEOM/$GEOM
export BINBASE=$BASE/$bin
export SCRIPT=$(basename $BASH_SOURCE)

vars="$vars TMP EVT BASE BINBASE SCRIPT"

Resolve_CFBaseFromGEOM()
{
   : LOOK FOR CFBase directory containing CSGFoundry geometry 
   : HMM COULD PUT INTO GEOM.sh TO AVOID DUPLICATION ? BUT TOO MUCH HIDDEN ? 
   : G4CXOpticks_setGeometry_Test GEOM TAKES PRECEDENCE OVER .opticks/GEOM
   : HMM : FOR SOME TESTS WANT TO LOAD GDML BUT FOR OTHERS CSGFoundry 
   : to handle that added gdml resolution to eg g4cx/tests/GXTestRunner.sh 

   local A_CFBaseFromGEOM=$HOME/.opticks/GEOM/$GEOM
   local B_CFBaseFromGEOM=$TMP/G4CXOpticks_setGeometry_Test/$GEOM
   local C_CFBaseFromGEOM=/cvmfs/opticks.ihep.ac.cn/.opticks/GEOM/$GEOM

   local TestPath=CSGFoundry/prim.npy
   local GDMLPathFromGEOM=$HOME/.opticks/GEOM/$GEOM/origin.gdml 

    if [ -d "$A_CFBaseFromGEOM" -a -f "$A_CFBaseFromGEOM/$TestPath" ]; then
        export ${GEOM}_CFBaseFromGEOM=$A_CFBaseFromGEOM
        #echo $BASH_SOURCE : FOUND A_CFBaseFromGEOM $A_CFBaseFromGEOM containing $TestPath
    elif [ -d "$B_CFBaseFromGEOM" -a -f "$B_CFBaseFromGEOM/$TestPath" ]; then
        export ${GEOM}_CFBaseFromGEOM=$B_CFBaseFromGEOM
        #echo $BASH_SOURCE : FOUND B_CFBaseFromGEOM $B_CFBaseFromGEOM containing $TestPath
    elif [ -d "$C_CFBaseFromGEOM" -a -f "$C_CFBaseFromGEOM/$TestPath" ]; then
        export ${GEOM}_CFBaseFromGEOM=$C_CFBaseFromGEOM
        #echo $BASH_SOURCE : FOUND C_CFBaseFromGEOM $C_CFBaseFromGEOM containing $TestPath
    elif [ -f "$GDMLPathFromGEOM" ]; then 
        export ${GEOM}_GDMLPathFromGEOM=$GDMLPathFromGEOM
        echo $BASH_SOURCE : FOUND GDMLPathFromGEOM $GDMLPathFromGEOM 
    else
        echo $BASH_SOURCE : NOT-FOUND A_CFBaseFromGEOM $A_CFBaseFromGEOM containing $TestPath
        echo $BASH_SOURCE : NOT-FOUND B_CFBaseFromGEOM $B_CFBaseFromGEOM containing $TestPath
        echo $BASH_SOURCE : NOT-FOUND C_CFBaseFromGEOM $C_CFBaseFromGEOM containing $TestPath
        echo $BASH_SOURCE : NOT-FOUND GDMLPathFromGEOM $GDMLPathFromGEOM
    fi  
}
Resolve_CFBaseFromGEOM

vars="$vars ${GEOM}_CFBaseFromGEOM ${GEOM}_GDMLPathFromGEOM"







knobs()
{
   type $FUNCNAME 

   local exceptionFlags
   local debugLevel
   local optLevel

   #exceptionFlags=STACK_OVERFLOW   
   exceptionFlags=NONE

   #debugLevel=DEFAULT
   debugLevel=NONE
   #debugLevel=FULL

   #optLevel=DEFAULT
   #optLevel=LEVEL_0
   optLevel=LEVEL_3

 
   #export PIP__max_trace_depth=1
   export PIP__CreatePipelineOptions_exceptionFlags=$exceptionFlags # NONE/STACK_OVERFLOW/TRACE_DEPTH/USER/DEBUG
   export PIP__CreateModule_debugLevel=$debugLevel  # DEFAULT/NONE/MINIMAL/MODERATE/FULL   (DEFAULT is MINIMAL)
   export PIP__linkPipeline_debugLevel=$debugLevel  # DEFAULT/NONE/MINIMAL/MODERATE/FULL   
   export PIP__CreateModule_optLevel=$optLevel      # DEFAULT/LEVEL_0/LEVEL_1/LEVEL_2/LEVEL_3  

   #export Ctx=INFO
   #export PIP=INFO
   #export CSGOptiX=INFO


   export NPFold__substamp_DUMP=1


}


version=1
#version=98   ## set to 98 for low stats debugging

export VERSION=${VERSION:-$version}   ## see below currently using VERSION TO SELECT OPTICKS_EVENT_MODE
## VERSION CHANGES OUTPUT DIRECTORIES : SO USEFUL TO ARRANGE SEPARATE STUDIES

vars="$vars version VERSION"


#test=debug
#test=ref1
#test=ref5
#test=ref8
#test=ref10
#test=ref10_multilaunch
#test=input_genstep
#test=input_photon
test=large_evt


export TEST=${TEST:-$test}


#ctx=Debug_XORWOW 
#ctx=Debug_Philox
ctx=$(TEST=ContextString sbuild_test)

export OPTICKS_EVENT_NAME=${ctx}_${TEST}   
## SEventConfig::Initialize_EventName asserts OPTICKS_EVENT_NAME sbuild::Matches config of the build 


opticks_event_reldir=ALL${VERSION:-0}_${OPTICKS_EVENT_NAME:-none}   ## matches SEventConfig::_DefaultEventReldir OPTICKS_EVENT_RELDIR
export OPTICKS_EVENT_RELDIR='ALL${VERSION:-0}_${OPTICKS_EVENT_NAME:-none}'  
# opticks_event_reldir is resolved here, OPTICKS_EVENT_RELDIR by SEvt/SEventConfig 

vars="$vars test TEST opticks_event_reldir OPTICKS_EVENT_RELDIR"

case $TEST in 
ref10_multilaunch) alt_TEST=ref10_onelaunch ;;
ref10_onelaunch)   alt_TEST=ref10_multilaunch ;;
esac

alt_opticks_event_reldir=ALL${VERSION:-0}_${alt_TEST} 
vars="$vars alt_TEST alt_opticks_event_reldir"



export LOGDIR=$BINBASE/$opticks_event_reldir
export AFOLD=$BINBASE/$opticks_event_reldir/$EVT
export STEM=${opticks_event_reldir}_${PLOT}

#export BFOLD=$BASE/G4CXTest/ALL0/$EVT 
#export BFOLD=$BASE/jok-tds/ALL0/A001  
#BFOLD_NOTE="comparison with A from another executable"

export BFOLD=$BINBASE/$alt_opticks_event_reldir/$EVT   # comparison with alt_TEST
BFOLD_NOTE="comparison with alt_TEST:$alt_TEST"


mkdir -p $LOGDIR 
cd $LOGDIR 
LOGFILE=$bin.log

vars="$vars LOGDIR AFOLD BFOLD BFOLD_NOTE STEM LOGFILE"


case $VERSION in 
 0) opticks_event_mode=Minimal ;;
 1) opticks_event_mode=Hit ;; 
 2) opticks_event_mode=HitPhoton ;; 
 3) opticks_event_mode=HitPhoton ;; 
 4) opticks_event_mode=HitPhotonSeq ;; 
 5) opticks_event_mode=HitSeq ;; 
98) opticks_event_mode=DebugLite ;;
99) opticks_event_mode=DebugHeavy ;;  # formerly StandardFullDebug
esac 

vars="$vars opticks_event_mode"


if [ "$TEST" == "debug" ]; then 

   opticks_num_photon=100
   opticks_num_genstep=1
   opticks_max_photon=M1
   opticks_num_event=1
   opticks_running_mode=SRM_TORCH

elif [ "$TEST" == "ref1" ]; then 

   opticks_num_photon=M1
   opticks_num_genstep=10
   opticks_max_photon=M1
   opticks_num_event=1
   opticks_running_mode=SRM_TORCH


elif [ "$TEST" == "ref5" -o "$TEST" == "ref6" -o "$TEST" == "ref7" -o "$TEST" == "ref8" -o "$TEST" == "ref9" -o "$TEST" == "ref10" ]; then 

   opticks_num_photon=M${TEST:3}
   opticks_num_genstep=1
   opticks_max_photon=M10
   opticks_num_event=1
   opticks_running_mode=SRM_TORCH

elif [ "$TEST" == "refX" ]; then 

   opticks_num_photon=${X:-7500000}
   opticks_num_genstep=1
   opticks_max_photon=M10
   opticks_num_event=1
   opticks_running_mode=SRM_TORCH

elif [ "$TEST" == "ref10_multilaunch" -o "$TEST" == "ref10_onelaunch" ]; then 

   opticks_num_photon=M10
   opticks_num_genstep=10
   opticks_max_photon=M10
   opticks_num_event=1
   opticks_running_mode=SRM_TORCH

   #opticks_max_curand=0    # zero loads all states : ready for whopper XORWOW running 
   #opticks_max_curand=M10  # non-zero loads the specified number
   #opricks_max_curand not relevant for Philox as no need to load states, so default is G1 1-billion-states 

   case $TEST in 
      *multilaunch) opticks_max_slot=M1 ;;     ## causes M10 to be done in 10 launches
        *onelaunch) opticks_max_slot=M10 ;; 
   esac 


elif [ "$TEST" == "tiny_scan" ]; then 

   opticks_num_photon=K1:10
   opticks_num_genstep=1,1,1,1,1,1,1,1,1,1
   opticks_max_photon=M1
   opticks_num_event=10
   opticks_running_mode=SRM_TORCH

elif [ "$TEST" == "large_scan" ]; then 

   opticks_num_photon=H1:10,M2,3,5,7,10,20,40,60,80,100
   opticks_num_genstep=1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
   opticks_max_photon=M100   ## cost: QRng init time + VRAM 
   opticks_num_event=20
   opticks_running_mode=SRM_TORCH

elif [ "$TEST" == "medium_scan" ]; then 

   opticks_num_photon=M1,1,10,20,30,40,50,60,70,80,90,100  # duplication of M1 is to workaround lack of metadata
   opticks_num_genstep=1,1,1,1,1,1,1,1,1,1,1,1
   opticks_max_photon=M100   
   opticks_num_event=12
   opticks_running_mode=SRM_TORCH

elif [ "$TEST" == "larger_scan" ]; then 

   opticks_num_photon=M1,1,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200  # duplication of M1 is to workaround lack of metadata
   opticks_num_genstep=1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
   opticks_max_photon=M200   
   opticks_num_event=22
   opticks_running_mode=SRM_TORCH

elif [ "$TEST" == "large_evt" ]; then 

   opticks_num_photon=M200         ## OOM with TITAN RTX 24G, avoided by multi-launch sliced genstep running
   opticks_num_genstep=10
   opticks_max_photon=M200         ## cost: QRng init time + VRAM (with XORWOW) 
   opticks_num_event=1
   opticks_running_mode=SRM_TORCH
   opticks_max_slot=0              ## zero -> SEventConfig::SetDevice determines MaxSlot based on VRAM   

elif [ "$TEST" == "vlarge_evt" ]; then 

   opticks_num_photon=M500  
   opticks_num_genstep=500
   opticks_max_photon=M200   
   opticks_num_event=1
   opticks_running_mode=SRM_TORCH
   opticks_max_slot=0              ## zero -> SEventConfig::SetDevice determines MaxSlot based on VRAM   

elif [ "$TEST" == "input_genstep" ]; then

   opticks_num_photon=     # ignored ?
   opticks_num_genstep=    # ignored
   opticks_max_photon=M1 
   opticks_num_event=1000 
   opticks_running_mode=SRM_INPUT_GENSTEP
   opticks_event_mode=Nothing

elif [ "$TEST" == "input_photon" ]; then

   opticks_num_photon=     # ignored ?
   opticks_num_genstep=    # ignored
   opticks_max_photon=M1 
   opticks_num_event=1
   opticks_running_mode=SRM_INPUT_PHOTON
   #opticks_event_mode=Nothing

fi 

vars="$vars opticks_num_photon opticks_num_genstep opticks_max_photon opticks_num_event opticks_running_mode"



#opticks_running_mode=SRM_DEFAULT
#opticks_running_mode=SRM_INPUT_PHOTON
#opticks_running_mode=SRM_GUN


opticks_start_index=0
opticks_max_bounce=31
opticks_integration_mode=1

export OPTICKS_EVENT_MODE=${OPTICKS_EVENT_MODE:-$opticks_event_mode}
export OPTICKS_NUM_PHOTON=${OPTICKS_NUM_PHOTON:-$opticks_num_photon} 
export OPTICKS_NUM_GENSTEP=${OPTICKS_NUM_GENSTEP:-$opticks_num_genstep} 
export OPTICKS_NUM_EVENT=${OPTICKS_NUM_EVENT:-$opticks_num_event}
export OPTICKS_MAX_PHOTON=${OPTICKS_MAX_PHOTON:-$opticks_max_photon}
export OPTICKS_START_INDEX=${OPTICKS_START_INDEX:-$opticks_start_index}
export OPTICKS_MAX_BOUNCE=${OPTICKS_MAX_BOUNCE:-$opticks_max_bounce}
export OPTICKS_INTEGRATION_MODE=${OPTICKS_INTEGRATION_MODE:-$opticks_integration_mode}
export OPTICKS_RUNNING_MODE=${OPTICKS_RUNNING_MODE:-$opticks_running_mode}


vars="$vars OPTICKS_EVENT_MODE OPTICKS_NUM_PHOTON OPTICKS_NUM_GENSTEP OPTICKS_MAX_PHOTON OPTICKS_NUM_EVENT OPTICKS_RUNNING_MODE"

export OPTICKS_MAX_CURAND=$opticks_max_curand  ## SEventConfig::MaxCurand only relevant to XORWOW
export OPTICKS_MAX_SLOT=$opticks_max_slot      ## SEventConfig::MaxSlot
vars="$vars OPTICKS_MAX_CURAND OPTICKS_MAX_SLOT" 


if [ "$OPTICKS_RUNNING_MODE" == "SRM_INPUT_GENSTEP" ]; then 

    #igs=$BASE/jok-tds/ALL0/A000/genstep.npy 
    igs=$BASE/jok-tds/ALL0/A%0.3d/genstep.npy 
    if [ "${igs/\%}" != "$igs" ]; then
        igs0=$(printf "$igs" 0) 
    else
        igs0=$igs
    fi 
    [ ! -f "$igs0" ] && echo $BASH_SOURCE : FATAL : NO SUCH PATH : igs0 $igs0 igs $igs && exit 1
    export OPTICKS_INPUT_GENSTEP=$igs

elif [ "$OPTICKS_RUNNING_MODE" == "SRM_INPUT_PHOTON" ]; then 

    #ipho=RainXZ_Z195_1000_f8.npy      ## ok 
    #ipho=RainXZ_Z230_1000_f8.npy      ## ok
    #ipho=RainXZ_Z230_10k_f8.npy       ## ok
    ipho=RainXZ_Z230_100k_f8.npy
    #ipho=RainXZ_Z230_X700_10k_f8.npy  ## X700 to illuminate multiple PMTs
    #ipho=GridXY_X700_Z230_10k_f8.npy 
    #ipho=GridXY_X1000_Z1000_40k_f8.npy

    #moi=-1
    #moi=sWorld:0:0
    #moi=NNVT:0:0
    #moi=NNVT:0:50
    moi=NNVT:0:1000
    #moi=PMT_20inch_veto:0:1000
    #moi=sChimneyAcrylic 

    # SEventConfig
    export OPTICKS_INPUT_PHOTON=${OPTICKS_INPUT_PHOTON:-$ipho};
    export OPTICKS_INPUT_PHOTON_FRAME=${MOI:-$moi}

elif [ "$OPTICKS_RUNNING_MODE" == "SRM_TORCH" ]; then 

    #export SEvent_MakeGenstep_num_ph=100000  OVERRIDEN BY OPTICKS_NUM_PHOTON
    #export SEvent__MakeGenstep_num_gs=10     OVERRIDEN BY OPTICKS_NUM_GENSTEP



    #src="rectangle"
    #src="disc"
    src="sphere"

    if [ "$src" == "rectangle" ]; then
        export storch_FillGenstep_pos=0,0,0
        export storch_FillGenstep_type=rectangle
        export storch_FillGenstep_zenith=-20,20
        export storch_FillGenstep_azimuth=-20,20
    elif [ "$src" == "disc" ]; then
        export storch_FillGenstep_type=disc
        export storch_FillGenstep_radius=50      
        export storch_FillGenstep_zenith=0,1       # radial range scale
        export storch_FillGenstep_azimuth=0,1      # phi segment twopi fraction 
        export storch_FillGenstep_mom=1,0,0
        export storch_FillGenstep_pos=-80,0,0
    elif [ "$src" == "sphere" ]; then
        export storch_FillGenstep_type=sphere
        export storch_FillGenstep_radius=100    # +ve for outwards    
        export storch_FillGenstep_pos=0,0,0
        export storch_FillGenstep_distance=1.00 # frac_twopi control of polarization phase(tangent direction)
    fi 



elif [ "$OPTICKS_RUNNING_MODE" == "SRM_GUN" ]; then 

    echo -n 

fi 



gdb__() 
{ 
    : prepares and invokes gdb - sets up breakpoints based on BP envvar containing space delimited symbols;
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


logging(){ 
    export CSGFoundry=INFO
    export CSGOptiX=INFO
    export QEvent=INFO 
    export QSim=INFO
    export SEvt__LIFECYCLE=1
}
[ -n "$LOG" ] && logging
[ -n "$LIFECYCLE" ] && export SEvt__LIFECYCLE=1
[ -n "$MEMCHECK" ] && export QU__MEMCHECK=1
[ -n "$MINIMAL"  ] && export SEvt__MINIMAL=1

export QRng__init_VERBOSE=1
export SEvt__MINIMAL=1  ## just output dir 
#export SEvt__DIRECTORY=1  ## getDir dumping 

#export SEvt__NPFOLD_VERBOSE=1 
#export QSim__simulate_KEEP_SUBFOLD=1
#export SEvt__transformInputPhoton_VERBOSE=1
#export CSGFoundry__getFrameE_VERBOSE=1
#export CSGFoundry__getFrame_VERBOSE=1




if [ "${arg/info}" != "$arg" ]; then
   for var in $vars ; do printf "%-30s : %s \n" "$var" "${!var}" ; done 
fi 

if [ "${arg/env}" != "$arg" ]; then 
    env | grep OPTICKS | perl -n -e 'm/(\S*)=(\S*)/ && printf("%50s : %s\n", $1, $2) ' -
fi 

if [ "${arg/fold}" != "$arg" ]; then
    echo $AFOLD
    du -hs $AFOLD/*
fi 

if [ "${arg/run}" != "$arg" -o "${arg/dbg}" != "$arg" ]; then

   knobs

   if [ -f "$LOGFILE" ]; then 
       echo $BASH_SOURCE : run : delete prior LOGFILE $LOGFILE 
       rm "$LOGFILE" 
   fi 

   if [ "${arg/run}" != "$arg" ]; then
       date +"%Y-%m-%d %H:%M:%S.%3N  %N : [$BASH_SOURCE "
       $bin
       [ $? -ne 0 ] && echo $BASH_SOURCE run error && exit 1 
       date +"%Y-%m-%d %H:%M:%S.%3N  %N : ]$BASH_SOURCE "
   elif [ "${arg/dbg}" != "$arg" ]; then
       gdb__ $bin 
       [ $? -ne 0 ] && echo $BASH_SOURCE dbg error && exit 1 
   fi 
fi 

if [ "${arg/meta}" != "$arg" ]; then
   if [ -f "run_meta.txt" -a -n "$OPTICKS_SCAN_INDEX"  -a -d "$OPTICKS_SCAN_INDEX" ] ; then
       cp run_meta.txt $OPTICKS_SCAN_INDEX/run_meta.txt
   fi 
   [ $? -ne 0 ] && echo $BASH_SOURCE meta error && exit 1 
fi 


if [ "${arg/report}" != "$arg" ]; then
   sreport
   [ $? -ne 0 ] && echo $BASH_SOURCE sreport error && exit 1 
fi 

if [ "${arg/grab}" != "$arg" ]; then
    source $OPTICKS_HOME/bin/rsync.sh $LOGDIR
fi 

if [ "${arg/grep}" != "$arg" ]; then
    source $OPTICKS_HOME/bin/rsync.sh ${LOGDIR}_sreport
fi 

if [ "${arg/gevt}" != "$arg" ]; then
    source $OPTICKS_HOME/bin/rsync.sh $LOGDIR/$EVT
fi 


if [ "${arg/pdb1}" != "$arg" ]; then
    ${ipython:-ipython} --pdb -i $script
fi 

if [ "${arg/pdb0}" != "$arg" ]; then
    MODE=0 ${ipython:-ipython} --pdb -i $script
fi 

if [ "${arg/AB}" != "$arg" ]; then
    MODE=0 ${ipython:-ipython} --pdb -i $script_AB
fi 


if [ "${arg/ana}" != "$arg" ]; then
    MODE=0 ${PYTHON:-python} $script
fi 

if [ "$arg" == "pvcap" -o "$arg" == "pvpub" -o "$arg" == "mpcap" -o "$arg" == "mppub" ]; then
    export CAP_BASE=$AFOLD/figs
    export CAP_REL=cxs_min
    export CAP_STEM=$STEM
    case $arg in  
       pvcap) source pvcap.sh cap  ;;  
       mpcap) source mpcap.sh cap  ;;  
       pvpub) source pvcap.sh env  ;;  
       mppub) source mpcap.sh env  ;;  
    esac
    if [ "$arg" == "pvpub" -o "$arg" == "mppub" ]; then 
        source epub.sh 
    fi  
fi 

