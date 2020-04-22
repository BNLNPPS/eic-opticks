#!/bin/bash -l
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


opticks-
opticks-boost-info

sdir=$(pwd)
bdir=/tmp/$USER/opticks/$(basename $sdir)/build 

rm -rf $bdir && mkdir -p $bdir && cd $bdir && pwd 


om-
om-export 
om-export-info

pkg=Boost
om-export-find $pkg


om-cmake $sdir

#cmake $sdir \
#     -DCMAKE_BUILD_TYPE=Debug \
#     -DCMAKE_INSTALL_PREFIX=$(opticks-prefix) \
#     -DCMAKE_MODULE_PATH=$(opticks-home)/cmake/Modules \
#     -DOPTICKS_PREFIX=$(opticks-prefix)


make
make install   


UseBoostFS

# boost before 1.70 does not have the CMake scripts, so find_package.py doesnt see it 


