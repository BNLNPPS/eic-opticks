#!/usr/bin/env bash

set -e

QCurandStateMonolithic_SPEC=1:0:0  $OPTICKS_BUILD/qudarap/tests/QCurandStateMonolithicTest
QCurandStateMonolithic_SPEC=3:0:0  $OPTICKS_BUILD/qudarap/tests/QCurandStateMonolithicTest
QCurandStateMonolithic_SPEC=10:0:0 $OPTICKS_BUILD/qudarap/tests/QCurandStateMonolithicTest

mv $HOME/.opticks/rngcache/RNG/QCurandStateMonolithic_1M_0_0.bin $HOME/.opticks/rngcache/RNG/QCurandState_1000000_0_0.bin
mv $HOME/.opticks/rngcache/RNG/QCurandStateMonolithic_3M_0_0.bin $HOME/.opticks/rngcache/RNG/QCurandState_3000000_0_0.bin
mv $HOME/.opticks/rngcache/RNG/QCurandStateMonolithic_10M_0_0.bin $HOME/.opticks/rngcache/RNG/QCurandState_10000000_0_0.bin

install -D $OPTICKS_HOME/tests/GEOM.sh $HOME/.opticks/GEOM/GEOM.sh

DTYPE=np.float32 $OPTICKS_HOME/optiphy/tools/input_photons.py
DTYPE=np.float64 $OPTICKS_HOME/optiphy/tools/input_photons.py

export GEOM=RaindropRockAirWater
export G4CXOpticks__setGeometry_saveGeometry=$TMP/G4CXOpticks_setGeometry_Test/$GEOM

ctest --test-dir $OPTICKS_BUILD/sysrap -j -E STTFTest
ctest --test-dir $OPTICKS_BUILD/ana -j
ctest --test-dir $OPTICKS_BUILD/analytic -j
ctest --test-dir $OPTICKS_BUILD/bin -j
ctest --test-dir $OPTICKS_BUILD/g4cx -j -E G4CXRenderTest
ctest --test-dir $OPTICKS_BUILD/CSG -j -E CSGQueryTest
ctest --test-dir $OPTICKS_BUILD/qudarap -j -E "QSimTest|QOpticalTest|QEvent_Lifecycle_Test|QSim_Lifecycle_Test|QSimWithEventTest"
ctest --test-dir $OPTICKS_BUILD/CSGOptiX -j -E CSGOptiXRenderTest
ctest --test-dir $OPTICKS_BUILD/gdxml -j
ctest --test-dir $OPTICKS_BUILD/u4 -j -E "U4GDMLReadTest|U4RandomTest|U4TraverseTest"
