#!/usr/bin/env bash

set -e

export USER=fakeuser
export GEOM=fakegeom
export OPTICKS_EVENT_MODE=DebugLite
export OPTICKS_MAX_SLOT=1000000

simg4ox -g $OPTICKS_HOME/tests/geom/raindrop.gdml -m $OPTICKS_HOME/tests/run.mac
python $OPTICKS_HOME/tests/compare_ab.py
