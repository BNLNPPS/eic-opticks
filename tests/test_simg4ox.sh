#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_DIR=${REPO_DIR:-$(cd "${SCRIPT_DIR}/.." && pwd)}

SIMG4OX_BIN=${SIMG4OX_BIN:-simg4ox}
PYTHON=${PYTHON:-python3}
COMPARE_AB="${REPO_DIR}/optiphy/ana/compare_ab.py"
GEOMETRY=${1:-raindrop}

export OPTICKS_HOME="${REPO_DIR}"
export SIMPHONY_CONFIG_DIR="${SIMPHONY_CONFIG_DIR:-${REPO_DIR}/config}"
export PYTHONPATH="${REPO_DIR}${PYTHONPATH:+:${PYTHONPATH}}"

if [[ $# -gt 1 ]]; then
    echo "Usage: $(basename "$0") [raindrop|dune_mock_wls_detector_box]" >&2
    exit 2
fi

case "${GEOMETRY}" in
    raindrop)
        CONFIG=dev
        ;;
    dune_mock_wls_detector_box)
        CONFIG=dune_mock_wls_detector_box
        ;;
    *)
        echo "Unknown simg4ox test geometry: ${GEOMETRY}" >&2
        exit 2
        ;;
esac

rm -rf "${PWD}/ALL0_no_opticks_event_name"

"${SIMG4OX_BIN}" \
    -g "${REPO_DIR}/tests/geom/${GEOMETRY}.gdml" \
    -m "${REPO_DIR}/tests/run.mac" \
    -c "${CONFIG}"
"${PYTHON}" "${COMPARE_AB}" --base "${PWD}" --geometry "${GEOMETRY}"

# The DUNE invocation is record-only. Raindrop also retains the consolidated
# CPU/GPU hit validations from the main branch, whether selected explicitly or
# by default.
if [[ "${GEOMETRY}" == dune_mock_wls_detector_box ]]; then
    exit 0
fi

SEED=42
NSIGMA=3
GEOM_FILE="${REPO_DIR}/tests/geom/8x8SiPM_w_CSI_optial_grease.gdml"
MAC_FILE="${REPO_DIR}/tests/run.mac"
HIT_OUTPUT_DIR="${PWD}/simg4ox_hits"

run_hit_validation() {
    local case_name=$1
    local geom_file=$2
    local config_name=$3
    local case_dir="${HIT_OUTPUT_DIR}/${case_name}"

    mkdir -p "${case_dir}"
    echo "=== simg4ox ${case_name} CPU/GPU hit validation ==="
    (
        cd "${case_dir}"
        "${SIMG4OX_BIN}" -g "${geom_file}" -c "${config_name}" -m "${MAC_FILE}" -s "${SEED}" > run.log 2>&1
    )

    "${PYTHON}" "${COMPARE_AB}" hits "${case_dir}/g_hits.npy" "${case_dir}/s_hits.npy" \
        --count-nsigma "${NSIGMA}" \
        --chi2-ndf-tolerance 5 \
        --require-hits
}

run_hit_validation 8x8-sipm "${GEOM_FILE}" 8x8SiPM_crystal

# This geometry has two distinct SensDet logical volumes.  It catches a
# regression where each PhotonSD overwrites g_hits.npy with its own collection.
run_hit_validation multi-sensdet "${REPO_DIR}/tests/geom/opticks_two_spheres.gdml" dev
