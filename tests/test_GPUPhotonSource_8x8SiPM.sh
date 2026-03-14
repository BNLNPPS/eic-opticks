#!/usr/bin/env bash

set -e

export OPTICKS_MAX_BOUNCE=32

SEED=42
TOLERANCE_PCT=5
PASS=true

check_within_pct() {
    local label=$1
    local actual=$2
    local expected=$3
    local tol=$4
    local diff=$(( actual - expected ))
    if [ "$diff" -lt 0 ]; then diff=$(( -diff )); fi
    local threshold=$(( expected * tol / 100 ))
    if [ "$threshold" -lt 1 ]; then threshold=1; fi
    if [ "$diff" -le "$threshold" ]; then
        echo "PASSED: $label ($actual) is within ${tol}% of $expected (diff=$diff, threshold=$threshold)"
    else
        echo "FAILED: $label ($actual) is NOT within ${tol}% of $expected (diff=$diff, threshold=$threshold)"
        PASS=false
    fi
}

check_nonzero() {
    local label=$1
    local actual=$2
    if [ "$actual" -gt 0 ]; then
        echo "PASSED: $label ($actual) > 0"
    else
        echo "FAILED: $label ($actual) should be > 0"
        PASS=false
    fi
}

# ---- Test: GPU vs CPU hit comparison with 8x8 SiPM + CsI + optical grease ----
echo "=== Test: GPUPhotonSource with 8x8SiPM_w_CSI_optial_grease.gdml ==="
echo "Config: 1000 photons inside crystal at (-8.7, -8.7, 4.0)mm, 420nm, disc r=0.5mm"
echo "Running GPUPhotonSource with seed $SEED ..."

OUTPUT=$(USER=fakeuser GEOM=fakegeom GPUPhotonSource \
    -g "$OPTICKS_HOME/tests/geom/8x8SiPM_w_CSI_optial_grease.gdml" \
    -c 8x8SiPM_crystal \
    -m "$OPTICKS_HOME/tests/run.mac" \
    -s "$SEED" 2>&1)

G4_HITS=$(echo "$OUTPUT" | grep "Geant4: NumHits:" | awk '{print $NF}')
OPTICKS_HITS=$(echo "$OUTPUT" | grep "Opticks: NumHits:" | tail -1 | awk '{print $NF}')

echo ""
echo "Geant4 (CPU):  NumHits: $G4_HITS"
echo "Opticks (GPU): NumHits: $OPTICKS_HITS"

# Both should produce hits
check_nonzero "Geant4 hits"  "$G4_HITS"
check_nonzero "Opticks hits" "$OPTICKS_HITS"

# GPU and CPU should agree within tolerance
echo ""
echo "=== Comparing GPU vs CPU hit counts (tolerance: ${TOLERANCE_PCT}%) ==="
check_within_pct "Opticks vs Geant4" "$OPTICKS_HITS" "$G4_HITS" "$TOLERANCE_PCT"

# ---- Summary ----
echo ""
echo "=== Summary ==="
echo "GPU (Opticks): $OPTICKS_HITS hits"
echo "CPU (Geant4):  $G4_HITS hits"
if [ "$G4_HITS" -gt 0 ]; then
    PCT=$(awk "BEGIN {printf \"%.1f\", ($OPTICKS_HITS/$G4_HITS)*100}")
    echo "GPU/CPU ratio: ${PCT}%"
fi
echo ""
if [ "$PASS" = true ]; then
    echo "All tests passed"
    exit 0
else
    echo "Some tests FAILED"
    exit 1
fi
