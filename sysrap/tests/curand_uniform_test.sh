#!/bin/bash
usage(){ cat << EOU
curand_uniform_test.sh 
========================

EOU
}

cd $(dirname $(realpath $BASH_SOURCE))

name=curand_uniform_test

defarg="build_run_ana"
arg=${1:-$defarg}

cuda_prefix=/usr/local/cuda
CUDA_PREFIX=${CUDA_PREFIX:-$cuda_prefix}

export FOLD=/tmp/$name
mkdir -p $FOLD
bin=$FOLD/$name

vars="BASH_SOURCE SDIR name FOLD"


if [ "${arg/info}" != "$arg" ]; then 
   for var in $vars ; do printf "%20s : %s \n" "$var" "${!var}" ; done 
fi

if [ "${arg/build}" != "$arg" ]; then 
    #opt="-use_fast_math"
    opt="" 
    echo $msg opt $opt
    nvcc $name.cu -std=c++11 $opt -I$HOME/np -I..  -I$CUDA_PREFIX/include -o $bin
    [ $? -ne 0 ] && echo compilation error && exit 1
fi 

if [ "${arg/run}" != "$arg" ]; then 
    $bin
    [ $? -ne 0 ] && echo run  error && exit 2
fi 

if [ "${arg/dbg}" != "$arg" ]; then 
    ${IPYTHON:-ipython} --pdb -i $name.py 
    [ $? -ne 0 ] && echo dbg error && exit 3
fi 

if [ "${arg/ana}" != "$arg" ]; then 
    python $name.py 
    [ $? -ne 0 ] && echo ana error && exit 4
fi 


if [ "${arg/grab}" != "$arg" ]; then 
    rsync -av P:$FOLD/ $FOLD
    [ $? -ne 0 ] && echo grab error && exit 4
fi 



exit 0 

