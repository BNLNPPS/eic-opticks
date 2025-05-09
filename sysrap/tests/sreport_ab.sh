#!/bin/bash 
usage(){ cat << EOU
sreport_ab.sh : comparison between two report folders 
========================================================

::

   ~/o/sreport_ab.sh

   A_JOB=N7 B_JOB=A7 ~/opticks/sysrap/tests/sreport_ab.sh

   A_JOB=N7 B_JOB=A7 PLOT=Substamp_ALL_Etime_vs_Photon ~/o/sreport_ab.sh


Usage from laptop example
--------------------------

This performance plotting has traditionally been done on laptop, after rsync-ing the 
small metadata only "_sreport" folders from REMOTE workstations which 
are ssh tunnel connected to laptop, via eg::

   laptop> ## check the P and A tunnels are running 
   laptop> o ; git pull   ## update opticks source, for the scripts only 

   laptop> TMP=/data/blyth/opticks TEST=medium_scan REMOTE=P ~/o/cxs_min.sh info   ## check remote LOGDIR is  correct 
   laptop> TMP=/data/blyth/opticks TEST=medium_scan REMOTE=P ~/o/cxs_min.sh grep   ## rsync the report to laptop

   laptop> TMP=/data1/blyth/tmp TEST=medium_scan REMOTE=A ~/o/cxs_min.sh info   ## check remote LOGDIR is  correct 
   laptop> TMP=/data1/blyth/tmp TEST=medium_scan REMOTE=A ~/o/cxs_min.sh grep   ## rsync the report to laptop


For reference the "grep" ~/o/cxs_min.sh grep subcommand::

    600 if [ "${arg/grep}" != "$arg" ]; then
    601     source $OPTICKS_HOME/bin/rsync.sh ${LOGDIR}_sreport
    602 fi

NB the LOGDIR (aka the run dir) is one level above the event folders
and the sreport folder is a sibling to that containing only metadata
of small size.  


EOU
}

cd $(dirname $(realpath $BASH_SOURCE))

defarg="info_ana"
arg=${1:-$defarg}

name=sreport_ab
script=$name.py

source $HOME/.opticks/GEOM/GEOM.sh 

a=N8   
b=A8   

export A=${A:-$a}
export B=${B:-$b}

resolve(){
    case $1 in 
      N7) echo /data/blyth/opticks/GEOM/J_2024aug27/CSGOptiXSMTest/ALL1 ;; 
      A7) echo /data1/blyth/tmp/GEOM/J_2024aug27/CSGOptiXSMTest/ALL1    ;;
      S7) echo /data/simon/opticks/GEOM/J_2024aug27/CSGOptiXSMTest/ALL1 ;; 

      N8) echo /data/blyth/opticks/GEOM/J_2024nov27/CSGOptiXSMTest/ALL1_Debug_Philox_medium_scan ;; 
      A8) echo /data1/blyth/tmp/GEOM/J_2024nov27/CSGOptiXSMTest/ALL1_Debug_Philox_medium_scan ;; 
    esac
}

plot=AB_Substamp_ALL_Etime_vs_Photon
export PLOT=${PLOT:-$plot}

export A_SREPORT_FOLD=$(resolve $A)_sreport  
export B_SREPORT_FOLD=$(resolve $B)_sreport  
export MODE=2                                 ## 2:matplotlib plotting 

vars="0 BASH_SOURCE arg defarg A B A_SREPORT_FOLD B_SREPORT_FOLD MODE PLOT"

if [ "${arg/info}" != "$arg" ]; then
    for var in $vars ; do printf "%25s : %s \n" "$var" "${!var}" ; done 
fi 

if [ "${arg/ana}" != "$arg" ]; then 
    export COMMANDLINE="A=$A B=$B PLOT=$PLOT ~/o/sreport_ab.sh"
    ${IPYTHON:-ipython} --pdb -i $script
    [ $? -ne 0 ] && echo $BASH_SOURCE : ana error && exit 3
fi

if [ "$arg" == "mpcap" -o "$arg" == "mppub" ]; then
    export CAP_BASE=$A_SREPORT_FOLD/figs
    export CAP_REL=sreport_ab
    export CAP_STEM=$PLOT
    case $arg in  
       mpcap) source mpcap.sh cap  ;;  
       mppub) source mpcap.sh env  ;;  
    esac
    if [ "$arg" == "mppub" ]; then 
        source epub.sh 
    fi  
fi 

exit 0 

