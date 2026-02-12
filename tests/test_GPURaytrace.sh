#!/usr/bin/env bash

set -e

export CSGOptiX__optixpath=${OPTICKS_PREFIX}/ptx/CSGOptiX_generated_CSGOptiX7.cu.ptx

SEED=42
TOLERANCE=113
EXPECTED_G4_HITS=12672
EXPECTED_OPTICKS_HITS=12664

echo "Running GPURaytrace with seed $SEED ..."
OUTPUT=$(USER=fakeuser GEOM=fakegeom GPURaytrace \
    -g "$OPTICKS_HOME/tests/geom/opticks_raindrop.gdml" \
    -m "$OPTICKS_HOME/tests/run.mac" \
    -s "$SEED" 2>&1)

G4_HITS=$(echo "$OUTPUT" | grep "Geant4: NumHits:" | awk '{print $NF}')
OPTICKS_HITS=$(echo "$OUTPUT" | grep "Opticks: NumHits:" | awk '{print $NF}')

echo "Geant4:  NumHits: $G4_HITS  (expected $EXPECTED_G4_HITS +/- $TOLERANCE)"
echo "Opticks: NumHits: $OPTICKS_HITS  (expected $EXPECTED_OPTICKS_HITS +/- $TOLERANCE)"

PASS=true

# Check Geant4 hits
G4_LO=$((EXPECTED_G4_HITS - TOLERANCE))
G4_HI=$((EXPECTED_G4_HITS + TOLERANCE))
if [ "$G4_HITS" -ge "$G4_LO" ] && [ "$G4_HITS" -le "$G4_HI" ]; then
    echo "PASSED: Geant4 NumHits ($G4_HITS) is within [$G4_LO, $G4_HI]"
else
    echo "FAILED: Geant4 NumHits ($G4_HITS) is outside [$G4_LO, $G4_HI]"
    PASS=false
fi

# Check Opticks hits
OPT_LO=$((EXPECTED_OPTICKS_HITS - TOLERANCE))
OPT_HI=$((EXPECTED_OPTICKS_HITS + TOLERANCE))
if [ "$OPTICKS_HITS" -ge "$OPT_LO" ] && [ "$OPTICKS_HITS" -le "$OPT_HI" ]; then
    echo "PASSED: Opticks NumHits ($OPTICKS_HITS) is within [$OPT_LO, $OPT_HI]"
else
    echo "FAILED: Opticks NumHits ($OPTICKS_HITS) is outside [$OPT_LO, $OPT_HI]"
    PASS=false
fi

if [ "$PASS" = true ]; then
    echo "Test passed"
    exit 0
else
    echo "Test FAILED"
    exit 1
fi
