#!/bin/bash -l 
usage(){ cat << EOU
stree_load_test.sh 
=====================

CAUTION the "ana" python script is independent from the C++ side
with some different envvar controls.


Python
--------

::

   GEOM=J007 RIDX=1 ./stree_load_test.sh ana

Comparing with CSG/tests::

   RIDX=1 ./CSGFoundryLoadTest.sh ana     


C++
-----

::

    ~/o/sysrap/tests/stree_load_test.sh 
    TEST=desc ~/o/sysrap/tests/stree_load_test.sh 


To update the input tree::

    ~/o/u4/tests/U4TreeCreateTest.sh







    epsilon:~ blyth$ st
    /Users/blyth/opticks/sysrap/tests
    epsilon:tests blyth$ 
    epsilon:tests blyth$ LVID=112 ./stree_load_test.sh 
    stree::init 
    stree::load_ /tmp/blyth/opticks/U4TreeCreateTest/stree
     LVID 112 num_nds 11
     ix:  531 dp:    3 sx:    0 pt:  533     nc:    0 fc:   -1 ns:  532 lv:  112     tc:  103 pa:  319 bb:  319 xf:  208    zs
     ix:  532 dp:    3 sx:    1 pt:  533     nc:    0 fc:   -1 ns:   -1 lv:  112     tc:  105 pa:  320 bb:  320 xf:  209    cy
     ix:  533 dp:    2 sx:    0 pt:  535     nc:    2 fc:  531 ns:  534 lv:  112     tc:    1 pa:   -1 bb:   -1 xf:   -1    un
     ix:  534 dp:    2 sx:    1 pt:  535     nc:    0 fc:   -1 ns:   -1 lv:  112     tc:  105 pa:  321 bb:  321 xf:  210    cy
     ix:  535 dp:    1 sx:    0 pt:  541     nc:    2 fc:  533 ns:  540 lv:  112     tc:    1 pa:   -1 bb:   -1 xf:   -1    un
     ix:  536 dp:    3 sx:    0 pt:  538     nc:    0 fc:   -1 ns:  537 lv:  112     tc:  103 pa:  322 bb:  322 xf:  211    zs
     ix:  537 dp:    3 sx:    1 pt:  538     nc:    0 fc:   -1 ns:   -1 lv:  112     tc:  105 pa:  323 bb:  323 xf:  212    cy
     ix:  538 dp:    2 sx:    0 pt:  540     nc:    2 fc:  536 ns:  539 lv:  112     tc:    1 pa:   -1 bb:   -1 xf:   -1    un
     ix:  539 dp:    2 sx:    1 pt:  540     nc:    0 fc:   -1 ns:   -1 lv:  112     tc:  105 pa:  324 bb:  324 xf:  213    cy
     ix:  540 dp:    1 sx:    1 pt:  541     nc:    2 fc:  538 ns:   -1 lv:  112     tc:    1 pa:   -1 bb:   -1 xf:  214    un
     ix:  541 dp:    0 sx:   -1 pt:   -1     nc:    2 fc:  535 ns:   -1 lv:  112     tc:    3 pa:   -1 bb:   -1 xf:   -1    di


See:

* notes/issues/U4Tree_stree_snd_scsg_FAIL_consistent_parent.rst (realloc stale pointer issue now fixed)


EOU
}


SDIR=$(dirname $(realpath $BASH_SOURCE))

#defarg="info_build_run_ana"
defarg="info_build_run"
[ -n "$LVID" ] && defarg="build_run" 

arg=${1:-$defarg}

name=stree_load_test 
bin=/tmp/$name/$name 

source $HOME/.opticks/GEOM/GEOM.sh 
source $HOME/.opticks/GEOM/MOI.sh    # sets MOI envvar, use MOI bash function to setup/edit 



base=$HOME/.opticks/GEOM/$GEOM/CSGFoundry/SSim
#base=/tmp/$USER/opticks/U4TreeCreateTest 
#base=/data/blyth/opticks/U4TreeCreateTest

export BASE=${BASE:-$base}

cuda_prefix=/usr/local/cuda
CUDA_PREFIX=${CUDA_PREFIX:-$cuda_prefix}


opt="-DWITH_PLACEHOLDER"
opt="$opt -DWITH_CHILD"

export stree_level=1 
export FOLD=$BASE/stree
#export stree__get_frame_dump=1

#test=get_factor_nodes
test=get_repeat_node
#test=desc_node_solids
#test=desc_solids
#test=desc_solid

export TEST=${TEST:-$test}


vars="BASH_SOURCE BASE FOLD opt GEOM MOI TEST"


if [ ! -d "$BASE/stree" ]; then
    echo $BASH_SOURCE : BASE $BASE GEOM $GEOM
    echo $BASH_SOURCE : BASE directory MUST contain an stree directory : THIS DOES NOT 
    exit 1
fi 

if [ "${arg/info}" != "$arg" ]; then 
    for var in $vars ; do printf "%30s : %s \n" "$var" "${!var}" ; done
fi 

if [ "${arg/build}" != "$arg" ]; then 
    mkdir -p $(dirname $bin)

    gcc \
      $opt \
      $SDIR/$name.cc \
      $SDIR/../s_tv.cc \
      $SDIR/../s_bb.cc \
      $SDIR/../s_pa.cc \
      $SDIR/../sn.cc \
      $SDIR/../s_csg.cc  \
      -g -std=c++17 -lstdc++ -lm \
      -I$SDIR/.. \
      -I$CUDA_PREFIX/include \
      -I$OPTICKS_PREFIX/externals/glm/glm \
      -o $bin

    [ $? -ne 0 ] && echo $BASH_SOURCE build error with opt $opt && exit 1 
fi 

if [ "${arg/run}" != "$arg" ]; then 
    $bin
    [ $? -ne 0 ] && echo $BASH_SOURCE run error && exit 2 
fi 

if [ "${arg/dbg}" != "$arg" ]; then 
    case $(uname) in
       Darwin) lldb__ $bin ;;
       Linux)  gdb__  $bin ;;
    esac
    [ $? -ne 0 ] && echo $BASH_SOURCE dbg error && exit 3 
fi 

if [ "${arg/ana}" != "$arg" ]; then 
    ${IPYTHON:-ipython} --pdb -i $SDIR/$name.py 
    [ $? -ne 0 ] && echo $BASH_SOURCE ana error && exit 4
fi 

if [ "${arg/csg}" != "$arg" ]; then 
    FOLD=$FOLD/csg ${IPYTHON:-ipython} --pdb -i $SDIR/${name}_csg.py 
    [ $? -ne 0 ] && echo $BASH_SOURCE ana error && exit 4
fi 

exit 0 

