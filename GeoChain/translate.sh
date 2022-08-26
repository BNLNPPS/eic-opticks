#!/bin/bash -l 
source $PWD/../bin/GEOM.sh trim    ## sets GEOM envvar with projection suffix eg _XZ trimmed 
msg="=== $BASH_SOURCE :"
usage(){ cat << EOU
GeoChain/translate.sh  : geometry conversions using GeoChainSolidTest or GeoChainVolumeTest
===========================================================================================

HMM: maybe rename to geochain.sh 

Performs full geometry translation chain:

1. Geant4 C++ G4VSolid OR G4VPhysicalVolume/G4LogicalVolume definition
2. X4PhysicalVolume steered conversion into NNode
3. X4PhysicalVolume steered loading into GGeo/GPts/GParts/GMesh/GMergedMesh
4. CSG_GGeo convertion of GGeo into CSGFoundry 

NB currently two executables are used, with the split decided by this
script based on the GEOM name:

GeoChainSolidTest
   for single G4VSolid

GeoChainVolumeTest
   for volumes 

HMM : consolidating to a single executable would need to implement
the below name switch in the main. But that would hardcode specific 
geometry names into code, so just stick to doing it in the script for now.
Doing this in PMTSim which is JUNO specific might be a solution.

Usage::

   gc                          # cd ~/opticks/GeoChain  

   vi ~/.opticks/GEOM.txt     ## or "geom" shortcut function, add or uncomment GEOM string in the file 
   ./translate.sh             ## default GEOM value is obtained from the GEOM.txt file


   GEOM=body_solid ./translate.sh 
   GEOM=body_phys  ./translate.sh 


   GEOM=body_phys ./grab.sh    # grab from remote node 


To render the resulting CSG geometry on GPU node use eg::

   cx
   ./cxs_geochain.sh     # 2D python intersect render, using center-extent-gensteps 
   ./cxr_geochain.sh     # 3D rendered view 

2D intersect CSG geometry on CPU::

   c 
   ./csg_geochain.sh 


NB IF YOU GET PERPLEXING FAILS REBUILD THE BELOW PACKAGES WHICH INCLUDE HEADERS WHICH MUST BE CONSISTENT

* CSG
* CSG_GGeo
* GeoChain 

EOU
}


# pick the Solid or Volume binary depending on GEOM

bin=
case $GEOM in 
   body_phys*)                  bin=GeoChainVolumeTest ;;
   inner1_phys)                 bin=GeoChainVolumeTest ;; 
   inner2_phys)                 bin=GeoChainVolumeTest ;; 
   dynode_phys)                 bin=GeoChainVolumeTest ;; 

   *)                           bin=GeoChainSolidTest  ;;    # default : assume solid
esac

# TODO: arrange auto-detection of Solid OR Volume so can then have single executable 
geoscript=../extg4/${GEOM}.sh 

if [ -f "$geoscript" ]; then
   echo $msg sourcing geoscript $geoscript
   source $geoscript 
else
   echo $msg THERE IS NO extg4 geoscript $geoscript 
fi 


echo $msg GEOM $GEOM bin $bin

if [ "$bin" == "" ]; then
   echo $msg ERROR do not know which executable to use for GEOM $GEOM
   exit 1 
fi

############### logging control ###################

#export GGeo=INFO
#export CSGSolid=INFO
export CSG_GGeo_Convert=INFO
export GeoChain=INFO

export NTreeProcess=INFO   ## balance decision happens here 
#export NTREEPROCESS_LVLIST=0


Unbalanced(){ cat << EOU
$FUNCNAME Unbalanced Tree
============================

Balancing is hereby switched off by setting 
the envvar  NTREEPROCESS_MAXHEIGHT0 to a large value.

NTREEPROCESS_MAXHEIGHT0 : $NTREEPROCESS_MAXHEIGHT0

Have confirmed that switching off balancing 
prevents interior boundary spurious issue 
found with some solids, but it also causes 
a very large performance problem.

TODO: find better way to postorder traverse a complete binary 
tree with lots of CSG_ZERO eg each node could carry a nextIdx
reference. 

EOU
}

if [ "${GEOM/Unbalanced}" != "${GEOM}" ]; then
   export NTREEPROCESS_MAXHEIGHT0=10   ## 3 is default
   Unbalanced 
fi 


#export NNodeNudger=INFO
#export NNODENUDGER_LVLIST=0

#export NTreeBalance=INFO
#export NTreeBuilder=INFO

#export nthetacut=INFO 
#export nphicut=INFO 
export NCSG=INFO
export NCSGData=INFO
export nmultiunion=INFO

export X4Solid=INFO  
export X4SolidTree=INFO
export X4SolidTree_verbose=1
export X4SolidMaker=INFO  
export X4PhysicalVolume=INFO

# checking that --skipsolidname is working 
#export OpticksDbg=INFO  
#export GInstancer=INFO

#export DUMP_RIDX=0


################# mechanics ###############

unset OPTICKS_KEY      # TODO: do this inside executables, as kinda important 

#####################################################

#cd $(opticks-home)/GeoChain

if [ -f "$bin.log" ]; then 
    rm $bin.log 
fi 

which $bin

opts=""
#opts="$opts --x4tubsnudgeskip 0"
#opts="$opts --skipsolidname ${GEOM}_body_solid_1_9   " 

#DEBUG=1
if [ -n "$DEBUG" ]; then 
    if [ "$(uname)" == "Darwin" ]; then 
        lldb__ $bin $opts 
    else
        gdb -ex r --args $bin $opts 
    fi
else 
    $bin $opts
fi

exit 0

