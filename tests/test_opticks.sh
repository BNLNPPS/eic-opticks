#!/usr/bin/env bash

set -e

QCurandStateMonolithic_SPEC=1:0:0  $OPTICKS_PREFIX/lib/QCurandStateMonolithicTest
QCurandStateMonolithic_SPEC=3:0:0  $OPTICKS_PREFIX/lib/QCurandStateMonolithicTest
QCurandStateMonolithic_SPEC=10:0:0 $OPTICKS_PREFIX/lib/QCurandStateMonolithicTest

mv $HOME/.opticks/rngcache/RNG/QCurandStateMonolithic_1M_0_0.bin $HOME/.opticks/rngcache/RNG/QCurandState_1000000_0_0.bin
mv $HOME/.opticks/rngcache/RNG/QCurandStateMonolithic_3M_0_0.bin $HOME/.opticks/rngcache/RNG/QCurandState_3000000_0_0.bin
mv $HOME/.opticks/rngcache/RNG/QCurandStateMonolithic_10M_0_0.bin $HOME/.opticks/rngcache/RNG/QCurandState_10000000_0_0.bin

install -D $OPTICKS_HOME/tests/GEOM.sh $HOME/.opticks/GEOM/GEOM.sh

DTYPE=np.float32 $OPTICKS_HOME/ana/input_photons.py
DTYPE=np.float64 $OPTICKS_HOME/ana/input_photons.py

ctest --test-dir $OPTICKS_PREFIX/build/okconf -j
ctest --test-dir $OPTICKS_PREFIX/build/sysrap -j -E STTFTest
ctest --test-dir $OPTICKS_PREFIX/build/ana -j
ctest --test-dir $OPTICKS_PREFIX/build/analytic -j
ctest --test-dir $OPTICKS_PREFIX/build/bin -j
ctest --test-dir $OPTICKS_PREFIX/build/CSG -j -E "CSGNodeTest|CSGPrimSpecTest|CSGPrimTest|CSGFoundryTest|CSGFoundry_getCenterExtent_Test|CSGFoundry_findSolidIdx_Test|CSGNameTest|CSGTargetTest|CSGTargetGlobalTest|CSGFoundry_MakeCenterExtentGensteps_Test|CSGFoundry_getFrame_Test|CSGFoundry_getFrameE_Test|CSGFoundry_getMeshName_Test|CSGFoundryLoadTest|CSGQueryTest|CSGSimtraceTest|CSGSimtraceRerunTest|CSGSimtraceSampleTest|CSGCopyTest|CSGScanTest"
ctest --test-dir $OPTICKS_PREFIX/build/qudarap -j -E "QSimTest|QOpticalTest|QEvent_Lifecycle_Test|QSim_Lifecycle_Test|QSimWithEventTest"
ctest --test-dir $OPTICKS_PREFIX/build/CSGOptiX -j -E CSGOptiXRenderTest
ctest --test-dir $OPTICKS_PREFIX/build/gdxml -j
ctest --test-dir $OPTICKS_PREFIX/build/u4 -j -E "U4GDMLReadTest|U4RandomTest|U4TraverseTest"
ctest --test-dir $OPTICKS_PREFIX/build/g4cx -j -E G4CXRenderTest
