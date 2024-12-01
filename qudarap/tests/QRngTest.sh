#/bin/bash 
usage(){ cat << EOU
QRngTest.sh
==============

~/o/qudarap/tests/QRngTest.sh 

OPTICKS_MAX_PHOTON=M4 ~/o/qudarap/tests/QRngTest.sh


EOU
}

cd $(dirname $(realpath $BASH_SOURCE))

name=QRngTest
bin=$name
script=$name.py 

export FOLD=$TMP/$name   ## needs to match whats in QRngTest.cc
mkdir -p $FOLD/float
mkdir -p $FOLD/double


export QRng__init_VERBOSE=1


defarg="info_run_ana"
arg=${1:-$defarg}


vars="FOLD bin script"


gdb__() 
{ 
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



if [ "${arg/info}" != "$arg" ]; then 
   for var in $vars ; do printf "%20s : %s\n" "$var" "${!var}" ; done
fi 

if [ "${arg/run}" != "$arg" ]; then 
   $bin
   [ $? -ne 0 ] && echo $BASH_SOURCE : run error && exit 1
   rm $name.log
fi 

if [ "${arg/dbg}" != "$arg" ]; then 
   gdb__ $bin
   [ $? -ne 0 ] && echo $BASH_SOURCE : dbg error && exit 2
   rm $name.log
fi 


if [ "${arg/ana}" != "$arg" ]; then 
    ${PYTHON:-python} $script
fi 

if [ "${arg/pdb}" != "$arg" ]; then 
    ${IPYTHON:-ipython} -i --pdb $script
fi 



