#!/bin/bash 
usage(){ cat << EOU
input_photons.sh
===================

Running this script creates several input photon .npy arrays 
into the directory ~/.opticks/InputPhotons::

   o   # cd ~/opticks

   ./ana/input_photons.sh   # create input photon arrays (non interactive)
   ./ana/iinput_photons.sh  # create input photon arrays (interactive) 

Note that the python script runs twice with DTYPE envvar 
as np.float32 and np.float64 in order to create the arrays 
in both single and double precision. 

EOU
}

DIR=$(dirname $BASH_SOURCE)
script=$DIR/input_photons.py

defarg="info_numpy_run_ls"
arg=${1:-$defarg}

dtypes="np.float32 np.float64"
vars="DIR script defarg arg dtypes"

if [ "${arg/info}" != "$arg" ]; then
   for var in $vars ; do printf "%30s : %s \n" "$var" "${!var}" ; done 
fi 


if [ "${arg/numpy}" != "$arg" ]; then
    python -c "import numpy as np" 2>/dev/null  
    [ $? -ne 0 ] && echo $BASH_SOURCE numpy package is not within your python - cannot generate input photons && exit 2
fi

if [ "${arg/dbg}" != "$arg" ]; then
    for dtype in $dtypes ; do 
        DTYPE=$dtype ${IPYTHON:-ipython} --pdb $OPT $script -- $*
        [ $? -ne 0 ] && echo $BASH_SOURCE dbg error && exit 1 
    done
fi



if [ "${arg/run}" != "$arg" ]; then
    for dtype in $dtypes ; do 
        DTYPE=$dtype python $script 
        [ $? -ne 0 ] && echo $BASH_SOURCE run error && exit 2
    done
fi

if [ "${arg/ls}" != "$arg" ]; then
   ls -alst ~/.opticks/InputPhotons
fi 


