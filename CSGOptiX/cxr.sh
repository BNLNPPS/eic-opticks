#!/bin/bash -l 
usage(){ cat << EOU
cxr.sh : basis for higher level render scripts using CSGOptiXRenderTest
=========================================================================

This is typically invoked via higher level scripts, see::

   ./cxr_demo.sh 
   ./cxr_solid.sh 
   ./cxr_view.sh 
   ./cxr_overview.sh 

TODO: revive MOI=ALL controlled from a higher level script, 
MOI=ALL creates an arglist file and uses the --arglist option 
to create a sequence of renders at the positions specified in 
the arglist all from a single load of the geometry.  
So a single run creates multiple different snaps of different
parts of a geometry.

To copy the last jpg render into s5p publication folders rerun the commandline 
with PUB=some_descriptive_string. 
The last render will be opened and a publication path determined and the jpg copied to it. 
To check the path to be used without doing the copy use PUB=1. 
Examples::

   PUB=1                      EYE=1,0,0 ZOOM=1 ./cxr_overview.sh 
   PUB=simple_transform_check EYE=1,0,0 ZOOM=1 ./cxr_overview.sh 


EOU
}

msg="=== $BASH_SOURCE :"

if [ -n "$CFNAME" ]; then
    echo $msg CFNAME $CFNAME is defined : this is used by cxr_geochain.sh and cxr_demo.sh 
    export CFBASE=/tmp/$USER/opticks/${CFNAME}    ## override CFBASE envvar only used when CFNAME defined, eg for demo geometry
    echo $msg CFNAME $CFNAME CFBASE $CFBASE OVERRIDING 
    if [ ! -d "$CFBASE/CSGFoundry" ]; then 
        echo $msg ERROR CFNAME override but no corresponding CSGFoundry directory $CFBASE/CSGFoundry 
        echo $msg TO CREATE NON-STANDARD geometries use \"gc \; GEOM=$(basename $CFNAME) ./run.sh\"  
        return 1
    fi
else
    export ${GEOM}_CFBaseFromGEOM=$HOME/.opticks/GEOM/$GEOM
fi 

defarg="run"
arg=${1:-$defarg}

pkg=CSGOptiX
bin=CSGOptiXRenderTest

# defaults 
cvd=1            # default GPU to use
emm=t0           # what to include in the GPU geometry : default to t0 ie ~0 which means everything 
moi=sWaterTube   # should be same as lLowerChimney_phys
eye=-1,-1,-1,1   # where to look from, see okc/View::home 
top=i0           # hmm difficuly to support other than i0
sla=             # solid_label selection 
icam=0           # 0:perpective 1:orthographic 2:equirect (2:not supported in CSGOptiX(7) yet)
tmin=0.1         # near in units of extent, so typical range is 0.1-2.0 for visibility, depending on EYE->LOOK distance
zoom=1.0
size=1280,720,1
sizescale=1.5

[ "$(uname)" == "Darwin" ] && cvd=0    # only one GPU on laptop 

export CVD=${CVD:-$cvd}    # --cvd 
export EMM=${EMM:-$emm}    # -e 
export MOI=${MOI:-$moi}    # evar:MOI OR --arglist when MOI=ALL  
export EYE=${EYE:-$eye}    # evar:EYE 
export TOP=$top            # evar:TOP? getting TOP=0 from somewhere causing crash
export SLA="${SLA:-$sla}"  # --solid_label
export ICAM=${ICAM:-$icam} # evar:CAMERATYPE  NB caller script ICAM overrides
export TMIN=${TMIN:-$tmin} # evar:TMIN
export ZOOM=${ZOOM:-$zoom} 
export SIZE=${SIZE:-$size} 
export SIZESCALE=${SIZESCALE:-$sizescale} 


# okc Composition/Camera::Camera needs CAMERATYPE integer 
export CAMERATYPE=$ICAM     
# SGLM needs string sname 
case $ICAM in
  0) export CAM="perspective" ;;
  1) export CAM="orthographic" ;;
esac


export OPTICKS_GEOM=${OPTICKS_GEOM:-$MOI}  # "sweeper" role , used by Opticks::getOutPrefix   

case $(uname) in 
   Darwin) optix_version=70000 ;; 
   Linux)  optix_version=$(CSGOptiXVersion 2>/dev/null) ;;
esac


# the OPTICKS_RELDIR and NAMEPREFIX defaults are typically overridden from higher level script
nameprefix=cxr_${top}_${EMM}_${SIZE}_
export NAMEPREFIX=${NAMEPREFIX:-$nameprefix}

reldir=top_${TOP}_
export OPTICKS_RELDIR=${OPTICKS_RELDIR:-$reldir}  


export TMPDIR=/tmp/$USER/opticks
export LOGDIR=$TMPDIR/$pkg/$bin
mkdir -p $LOGDIR 
cd $LOGDIR 


export OPTICKS_OUT_FOLD=${CFBASE:-$TMPDIR}/$pkg/$bin/$(SCVDLabel)/$optix_version
export OPTICKS_OUT_NAME=$MOI
# envvars used by SEventConfig::OutPath 

export BASE=$OPTICKS_OUT_FOLD/$OPTICKS_OUT_NAME


vars="CVD EMM MOI EYE TOP SLA CAM TMIN ZOOM CAMERATYPE OPTICKS_GEOM OPTICKS_RELDIR SIZE SIZESCALE CFBASE OPTICKS_OUT_FOLD OPTICKS_OUT_NAME"
for var in $vars ; do printf "%10s : %s \n" $var ${!var} ; done 


if [ -n "$DEBUG" ]; then 
    case $(uname) in 
       Darwin) GDB=lldb__ ;;
       Linux)  GDB=gdb    ;;
    esac
fi 

DIV=""
[ -n "$GDB" ] && DIV="--" 

#obsolete-render-cmd(){ cat << EOC
# modern Opticks executables dont parse commandlines, they read envvars 
# $GDB $bin $DIV --nameprefix "$NAMEPREFIX" --cvd $CVD -e "$EMM" --size "$SIZE" --sizescale "$SIZESCALE" --solid_label "$SLA" $* 
#EOC
# }   


render()
{
   local msg="=== $FUNCNAME :"
   which $bin
   pwd

   local log=$bin.log
   local cmd="$GDB $bin" 
   echo $cmd

   printf "\n\n\n$cmd\n\n\n" >> $log 

   eval $cmd
   local rc=$?

   printf "\n\n\nRC $rc\n\n\n" >> $log 

   echo $msg rc $rc

   return $rc
}


relative_stem(){
   local jpg=$1

   # HMM:geocache is very old world   
   local geocache=${OPTICKS_GEOCACHE_PREFIX:-$HOME/.opticks}/geocache/
   local oktmp=/tmp/$USER/opticks/

   local rel 
   case $jpg in 
      ${geocache}*)  rel=${jpg/$geocache/} ;;
      ${oktmp}*)     rel=${jpg/$oktmp/} ;; 
   esac 
   rel=${rel/\.jpg}

   echo $rel 
}


publish()
{
    : copy outputs from tmpdirs into the publication tree

    source CSGOptiXRenderTest_OUTPUT_DIR.sh || return 1  
    local outdir=$CSGOptiXRenderTest_OUTPUT_DIR 

    if [ -n "$outdir" ]; then 
        ls -1rt `find $outdir -name '*.jpg' `
        jpg=$(ls -1rt `find $outdir -name '*.jpg' ` | tail -1)
        echo $msg jpg $jpg 
        ls -l $jpg

        [ -n "$jpg" -a "$(uname)" == "Darwin" ] && open $jpg

        if [ -n "$jpg" -a "$(uname)" == "Darwin" -a -n "$PUB" ]; then 

            if [ "$PUB" == "1" ]; then 
                ext=""    ## use PUB=1 to debug the paths 
            else
                ext="_${PUB}" 
            fi 

            rel=$(relative_stem $jpg)
            s5p=/env/presentation/${rel}${ext}.jpg
            pub=$HOME/simoncblyth.bitbucket.io$s5p

            echo $msg jpg $jpg
            echo $msg rel $rel
            echo $msg ext $ext
            echo $msg pub $pub
            echo $msg s5p $s5p 1280px_720px 
            mkdir -p $(dirname $pub)

            if [ -f "$pub" ]; then 
                echo $msg published path exists already : NOT COPYING : set PUB to an ext string to distinguish the name or more permanently arrange for a different path   
            elif [ "$ext" == "" ]; then 
                echo $msg skipping copy : to do the copy you must set PUB to some descriptive string rather than just using PUB=1
            else
                echo $msg copying jpg to pub 
                cp $jpg $pub
                echo $msg add s5p to s5_background_image.txt
            fi 
        fi 

    else
        echo $msg outdir not defined 
    fi 
}



if [ "$arg" == "run" ]; then

    # ARGLIST needs rejig as no longer parsing args 
    #    render --arglist $ARGLIST $*            ## effectively multiple MOI via the arglist 

    render ## single MOI via envvar 

elif [ "$arg" == "grab" -o "$arg" == "open" -o "$arg" == "grab_open" ]; then 

    source $OPTICKS_HOME/bin/BASE_grab.sh $arg 

elif [ "$arg" == "pub" ]; then 

    publish 

fi 




