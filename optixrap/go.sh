#!/bin/bash -l

opticks-

sdir=$(pwd)
name=$(basename $sdir)

bdir=$(opticks-prefix)/build/$name

if [ "$1" == "clean" ]; then
   echo $0 $1 : remove bdir $bdir
   rm -rf $bdir 
fi 
mkdir -p $bdir && cd $bdir && pwd 

cmake $sdir \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH=$(opticks-prefix)/externals \
    -DCMAKE_INSTALL_PREFIX=$(opticks-prefix) \
    -DCMAKE_MODULE_PATH=$(opticks-home)/cmake/Modules 


# the below is not needed here anyore, moved to OKConf
#    -DOptiX_INSTALL_DIR=$(opticks-optix-install-dir) 

make
make install   

opticks-t $bdir

