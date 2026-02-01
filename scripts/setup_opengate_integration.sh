#!/bin/bash
# ============================================================================
# OpenGATE + eic-Opticks GPU Integration Setup Script
# ============================================================================
#
# This script modifies an OpenGATE installation to add GPU optical photon
# simulation support via eic-Opticks/OptiX.
#
# USAGE:
#   ./setup_opengate_opticks.sh [OPENGATE_DIR]
#
#   OPENGATE_DIR: Path to OpenGATE source (default: /src/opengate)
#
# PREREQUISITES:
#   - eic-opticks installed at /opt/eic-opticks (or set OPTICKS_PREFIX)
#   - CUDA toolkit installed
#   - Geant4 11.x installed
#
# ============================================================================
# an example to configure the environment on npps0
# docker run --rm -it ghcr.io/bnlnpps/eic-opticks:develop

set -e  # Exit on error

apt update
sudo apt install -y git

cd /src/
git clone --recursive https://github.com/OpenGATE/opengate.git
cd eic-opticks/

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
echo_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
echo_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Get script directory (where eic-opticks files are)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPTICKS_FILES_DIR="${SCRIPT_DIR}/opengate_opticks"

# OpenGATE directory (first argument or default)
OPENGATE_DIR="${1:-/src/opengate}"

# eic-Opticks installation
OPTICKS_PREFIX="${OPTICKS_PREFIX:-/opt/eic-opticks}"

echo "============================================================"
echo "OpenGATE + Opticks GPU Integration Setup"
echo "============================================================"
echo ""
echo "Configuration:"
echo "  Script directory:    ${SCRIPT_DIR}"
echo "  OpenGATE directory:  ${OPENGATE_DIR}"
echo "  eic-Opticks prefix:      ${OPTICKS_PREFIX}"
echo ""

# ============================================================================
# STEP 0: Validate prerequisites
# ============================================================================
echo_info "Checking prerequisites..."

if [ ! -d "${OPENGATE_DIR}" ]; then
    echo_error "OpenGATE directory not found: ${OPENGATE_DIR}"
    echo "  Please provide the path to OpenGATE source as first argument"
    exit 1
fi

if [ ! -d "${OPTICKS_PREFIX}" ]; then
    echo_error "eic-Opticks installation not found at: ${OPTICKS_PREFIX}"
    echo "  Please set OPTICKS_PREFIX environment variable"
    exit 1
fi

if [ ! -d "${OPTICKS_FILES_DIR}" ]; then
    echo_error "eic-Opticks actor files not found at: ${OPTICKS_FILES_DIR}"
    exit 1
fi

# Check for required files
REQUIRED_FILES=(
    "${OPTICKS_FILES_DIR}/GateOpticksActor.cpp"
    "${OPTICKS_FILES_DIR}/GateOpticksActor.h"
    "${OPTICKS_FILES_DIR}/pyGateOpticksActor.cpp"
    "${OPTICKS_FILES_DIR}/opticksactors.py"
)

for f in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$f" ]; then
        echo_error "Required file not found: $f"
        exit 1
    fi
done

echo_info "All prerequisites satisfied"

# ============================================================================
# STEP 0.5: Install system dependencies
# ============================================================================
echo_info "Checking/installing system dependencies..."

# Check if ITK dev libraries are installed
if ! dpkg -l | grep -q libinsighttoolkit5-dev 2>/dev/null; then
    echo_info "Installing ITK development libraries..."
    apt-get update -qq
    apt-get install -y libinsighttoolkit5-dev
else
    echo_info "ITK development libraries already installed"
fi

# ============================================================================
# STEP 1: Copy C++ source files
# ============================================================================
echo_info "Copying C++ source files..."

OPENGATE_LIB_DIR="${OPENGATE_DIR}/core/opengate_core/opengate_lib"

cp -v "${OPTICKS_FILES_DIR}/GateOpticksActor.cpp" "${OPENGATE_LIB_DIR}/"
cp -v "${OPTICKS_FILES_DIR}/GateOpticksActor.h" "${OPENGATE_LIB_DIR}/"
cp -v "${OPTICKS_FILES_DIR}/pyGateOpticksActor.cpp" "${OPENGATE_LIB_DIR}/"

# ============================================================================
# STEP 2: Copy Python wrapper
# ============================================================================
echo_info "Copying Python wrapper..."

OPENGATE_ACTORS_DIR="${OPENGATE_DIR}/opengate/actors"

cp -v "${OPTICKS_FILES_DIR}/opticksactors.py" "${OPENGATE_ACTORS_DIR}/"

# ============================================================================
# STEP 3: Fix visualization detection in CMakeLists.txt
# ============================================================================
echo_info "Fixing visualization detection in CMakeLists.txt..."

CMAKE_FILE="${OPENGATE_DIR}/core/CMakeLists.txt"

# Fix the visualization block to auto-detect private headers
# This is needed because eic-opticks Geant4 doesn't have private headers installed
if grep -q "private/G4OpenGLSceneHandler.hh" "${CMAKE_FILE}" 2>/dev/null || \
   ! grep -q "GATE_DISABLE_VISU" "${CMAKE_FILE}"; then

    # Check if we need to patch the visualization block
    if grep -q "Geant4_qt_FOUND OR Geant4_vis_opengl_x11_FOUND" "${CMAKE_FILE}" && \
       ! grep -q "G4_PRIVATE_HEADERS" "${CMAKE_FILE}"; then

        echo_info "Patching visualization detection to handle missing private headers..."

        # Create the new visualization block
        VISU_BLOCK=$(mktemp)
        cat > "${VISU_BLOCK}" << 'VISU_EOF'
# QT/Visualization in Geant4
# NOTE: OpenGATE visualization requires private Geant4 headers (private/G4OpenGLSceneHandler.hh)
# which are only available when Geant4 is built with GEANT4_INSTALL_PACKAGE_SOURCES=ON.
# Use GATE_DISABLE_VISU=ON to force disable visualization if private headers are missing.
option(GATE_DISABLE_VISU "Force disable visualization (use when Geant4 private headers are not installed)" OFF)
IF (GATE_DISABLE_VISU)
    message(STATUS "OPENGATE - Visualization disabled by GATE_DISABLE_VISU option")
    add_definitions(-DUSE_VISU=0)
ELSEIF (Geant4_qt_FOUND OR Geant4_vis_opengl_x11_FOUND)
    # Check if private headers are available
    find_path(G4_PRIVATE_HEADERS "private/G4OpenGLSceneHandler.hh" HINTS ${Geant4_INCLUDE_DIR} ${Geant4_INCLUDE_DIRS})
    IF (G4_PRIVATE_HEADERS)
        message(STATUS "OPENGATE - Geant4 is compiled with QT and private headers available")
        find_package(OpenGL QUIET)
        include_directories(${OPENGL_INCLUDE_DIR})
        add_definitions(-DUSE_VISU=1)
    ELSE ()
        message(STATUS "OPENGATE - Geant4 QT found but private headers missing, disabling visualization")
        message(STATUS "OPENGATE - To enable visualization, rebuild Geant4 with GEANT4_INSTALL_PACKAGE_SOURCES=ON")
        add_definitions(-DUSE_VISU=0)
    ENDIF ()
ELSE ()
    message(STATUS "OPENGATE without Geant4 visualisation")
    add_definitions(-DUSE_VISU=0)
ENDIF ()
VISU_EOF

        # Find and replace the visualization block
        # Look for the start: "# QT in Geant4" or "IF (Geant4_qt_FOUND"
        START_LINE=$(grep -n "# QT in Geant4" "${CMAKE_FILE}" | head -1 | cut -d: -f1)
        if [ -z "${START_LINE}" ]; then
            START_LINE=$(grep -n "IF (Geant4_qt_FOUND OR Geant4_vis_opengl_x11_FOUND)" "${CMAKE_FILE}" | head -1 | cut -d: -f1)
        fi

        # Find the end: "ENDIF ()" after the visualization block
        if [ -n "${START_LINE}" ]; then
            END_LINE=$(tail -n +${START_LINE} "${CMAKE_FILE}" | grep -n "^ENDIF ()" | head -1 | cut -d: -f1)
            if [ -n "${END_LINE}" ]; then
                END_LINE=$((START_LINE + END_LINE - 1))

                head -n $((START_LINE - 1)) "${CMAKE_FILE}" > "${CMAKE_FILE}.new"
                cat "${VISU_BLOCK}" >> "${CMAKE_FILE}.new"
                tail -n +$((END_LINE + 1)) "${CMAKE_FILE}" >> "${CMAKE_FILE}.new"
                mv "${CMAKE_FILE}.new" "${CMAKE_FILE}"
                echo_info "Visualization detection patched successfully"
            fi
        fi
        rm -f "${VISU_BLOCK}"
    else
        echo_info "Visualization detection already patched"
    fi
fi

# ============================================================================
# STEP 4: Patch CMakeLists.txt for eic-Opticks support
# ============================================================================
echo_info "Patching CMakeLists.txt for eic-Opticks support..."

# Check if already patched
if grep -q "GATE_USE_OPTICKS" "${CMAKE_FILE}"; then
    echo_warn "CMakeLists.txt already contains eic-Opticks support - skipping patch"
else
    # Create a temporary file with the Opticks configuration
    OPTICKS_CONFIG_FILE=$(mktemp)
    cat > "${OPTICKS_CONFIG_FILE}" << 'OPTICKS_CMAKE_EOF'

# Optional: eic-Opticks for GPU optical photon propagation
option(GATE_USE_OPTICKS "Enable GPU optical photon propagation with Opticks" OFF)
if(GATE_USE_OPTICKS)
    # Find Opticks - adjust path as needed
    set(OPTICKS_PREFIX "$ENV{OPTICKS_PREFIX}" CACHE PATH "Opticks installation prefix")
    if(NOT OPTICKS_PREFIX)
        message(FATAL_ERROR "OPTICKS_PREFIX not set. Please set OPTICKS_PREFIX environment variable.")
    endif()

    # Add Opticks include directories
    include_directories(
        ${OPTICKS_PREFIX}/include/eic-opticks
        ${OPTICKS_PREFIX}/include/eic-opticks/sysrap
        ${OPTICKS_PREFIX}/include/eic-opticks/qudarap
        ${OPTICKS_PREFIX}/include/eic-opticks/u4
        ${OPTICKS_PREFIX}/include/eic-opticks/g4cx
        ${OPTICKS_PREFIX}/include/eic-opticks/CSG
        ${OPTICKS_PREFIX}/include/eic-opticks/CSGOptiX
    )

    # Find CUDA
    find_package(CUDA REQUIRED)
    include_directories(${CUDA_INCLUDE_DIRS})

    add_definitions(-DGATE_USE_OPTICKS)

    # Use Philox RNG - does not require pre-computed curandState files
    add_definitions(-DRNG_PHILOX)

    # eic-Opticks libraries
    set(OPTICKS_LIBRARIES
        ${OPTICKS_PREFIX}/lib/libG4CX.so
        ${OPTICKS_PREFIX}/lib/libU4.so
        ${OPTICKS_PREFIX}/lib/libSysRap.so
        ${OPTICKS_PREFIX}/lib/libCSGOptiX.so
        ${OPTICKS_PREFIX}/lib/libCSG.so
        ${OPTICKS_PREFIX}/lib/libQUDARap.so
        ${CUDA_LIBRARIES}
    )

    message(STATUS "OPENGATE - Opticks GPU support enabled")
endif()
OPTICKS_CMAKE_EOF

    # Find line number of "endif (ITK_FOUND)" and insert after it
    LINE_NUM=$(grep -n "endif (ITK_FOUND)" "${CMAKE_FILE}" | head -1 | cut -d: -f1)
    if [ -n "${LINE_NUM}" ]; then
        head -n "${LINE_NUM}" "${CMAKE_FILE}" > "${CMAKE_FILE}.new"
        cat "${OPTICKS_CONFIG_FILE}" >> "${CMAKE_FILE}.new"
        tail -n +$((LINE_NUM + 1)) "${CMAKE_FILE}" >> "${CMAKE_FILE}.new"
        mv "${CMAKE_FILE}.new" "${CMAKE_FILE}"
    else
        echo_warn "Could not find 'endif (ITK_FOUND)' - appending Opticks config at end"
        cat "${OPTICKS_CONFIG_FILE}" >> "${CMAKE_FILE}"
    fi
    rm -f "${OPTICKS_CONFIG_FILE}"

    # Modify target_link_libraries to conditionally include Opticks
    # Find and replace the simple target_link_libraries line
    # Check if we have the conditional linking block (not just GATE_USE_OPTICKS option)
    if grep -q "target_link_libraries(opengate_core PRIVATE" "${CMAKE_FILE}" && \
       ! grep -q 'OPTICKS_LIBRARIES}' "${CMAKE_FILE}"; then

        # Create replacement content
        LINK_REPLACEMENT=$(mktemp)
        cat > "${LINK_REPLACEMENT}" << 'LINK_EOF'
# Conditional linking for Opticks
if(GATE_USE_OPTICKS)
    target_link_libraries(opengate_core PRIVATE
        pybind11::module ${Geant4_LIBRARIES} Threads::Threads
        ${ITK_LIBRARIES} fmt::fmt-header-only ${OPTICKS_LIBRARIES})
else()
    target_link_libraries(opengate_core PRIVATE
        pybind11::module ${Geant4_LIBRARIES} Threads::Threads
        ${ITK_LIBRARIES} fmt::fmt-header-only)
endif()
LINK_EOF

        # Find the line with target_link_libraries and replace it
        LINE_NUM=$(grep -n "target_link_libraries(opengate_core PRIVATE" "${CMAKE_FILE}" | tail -1 | cut -d: -f1)
        if [ -n "${LINE_NUM}" ]; then
            # Count how many lines the original target_link_libraries spans
            # It typically ends with a line containing just ")"
            END_LINE=${LINE_NUM}
            while ! sed -n "${END_LINE}p" "${CMAKE_FILE}" | grep -q ')$'; do
                END_LINE=$((END_LINE + 1))
                if [ ${END_LINE} -gt $((LINE_NUM + 10)) ]; then
                    break
                fi
            done

            head -n $((LINE_NUM - 1)) "${CMAKE_FILE}" > "${CMAKE_FILE}.new"
            cat "${LINK_REPLACEMENT}" >> "${CMAKE_FILE}.new"
            tail -n +$((END_LINE + 1)) "${CMAKE_FILE}" >> "${CMAKE_FILE}.new"
            mv "${CMAKE_FILE}.new" "${CMAKE_FILE}"
        fi
        rm -f "${LINK_REPLACEMENT}"
    fi

    echo_info "CMakeLists.txt patched successfully"
fi

# ============================================================================
# STEP 5: Fix unused vnl_matrix.h include in GateVBiasOptrActor.cpp
# ============================================================================
echo_info "Fixing unused includes in biasing code..."

BIAS_ACTOR_FILE="${OPENGATE_LIB_DIR}/biasing/GateVBiasOptrActor.cpp"

if [ -f "${BIAS_ACTOR_FILE}" ]; then
    if grep -q "#include <vnl_matrix.h>" "${BIAS_ACTOR_FILE}"; then
        echo_info "Removing unused vnl_matrix.h include..."
        sed -i 's/#include <vnl_matrix.h>/\/\/ Note: vnl_matrix.h removed - it was not used and requires ITK VNL headers/' "${BIAS_ACTOR_FILE}"
        echo_info "Fixed GateVBiasOptrActor.cpp"
    else
        echo_info "vnl_matrix.h include already fixed or not present"
    fi
else
    echo_warn "GateVBiasOptrActor.cpp not found - skipping"
fi

# ============================================================================
# STEP 6: Patch opengate_core.cpp (Python bindings)
# ============================================================================
echo_info "Patching opengate_core.cpp..."

OPENGATE_CORE_CPP="${OPENGATE_DIR}/core/opengate_core/opengate_core.cpp"

# Check if already patched
if grep -q "init_GateOpticksActor" "${OPENGATE_CORE_CPP}"; then
    echo_warn "opengate_core.cpp already contains OpticksActor binding - skipping patch"
else
    # Add forward declaration after init_GateARFTrainingDatasetActor
    LINE_NUM=$(grep -n "void init_GateARFTrainingDatasetActor" "${OPENGATE_CORE_CPP}" | head -1 | cut -d: -f1)
    if [ -n "${LINE_NUM}" ]; then
        sed -i "${LINE_NUM}a\\
\\
void init_GateOpticksActor(py::module \&m);" "${OPENGATE_CORE_CPP}"
    fi

    # Add initialization call after init_GateARFTrainingDatasetActor(m);
    LINE_NUM=$(grep -n "init_GateARFTrainingDatasetActor(m);" "${OPENGATE_CORE_CPP}" | head -1 | cut -d: -f1)
    if [ -n "${LINE_NUM}" ]; then
        sed -i "${LINE_NUM}a\\  init_GateOpticksActor(m);" "${OPENGATE_CORE_CPP}"
    fi

    echo_info "opengate_core.cpp patched successfully"
fi

# ============================================================================
# STEP 7: Patch managers.py (actor registration)
# ============================================================================
echo_info "Patching managers.py..."

MANAGERS_PY="${OPENGATE_DIR}/opengate/managers.py"

# Check if already patched
if grep -q "OpticksActor" "${MANAGERS_PY}"; then
    echo_warn "managers.py already contains OpticksActor registration - skipping patch"
else
    # Find the line with "particle_names_Gate_to_G4" and insert before it
    LINE_NUM=$(grep -n "^particle_names_Gate_to_G4" "${MANAGERS_PY}" | head -1 | cut -d: -f1)
    if [ -n "${LINE_NUM}" ]; then
        IMPORT_BLOCK=$(mktemp)
        cat > "${IMPORT_BLOCK}" << 'IMPORT_EOF'
# eic-Opticks GPU optical photon propagation (optional)
try:
    from .actors.opticksactors import OpticksActor
    _has_opticks = True
except ImportError:
    _has_opticks = False

IMPORT_EOF
        head -n $((LINE_NUM - 1)) "${MANAGERS_PY}" > "${MANAGERS_PY}.new"
        cat "${IMPORT_BLOCK}" >> "${MANAGERS_PY}.new"
        tail -n +${LINE_NUM} "${MANAGERS_PY}" >> "${MANAGERS_PY}.new"
        mv "${MANAGERS_PY}.new" "${MANAGERS_PY}"
        rm -f "${IMPORT_BLOCK}"
    fi

    # Find the closing brace of actor_types dict and add after it
    # First find actor_types = {, then find the next } on its own line
    ACTOR_TYPES_LINE=$(grep -n "^actor_types = {" "${MANAGERS_PY}" | head -1 | cut -d: -f1)
    if [ -n "${ACTOR_TYPES_LINE}" ]; then
        # Find the next closing brace after actor_types definition
        LINE_NUM=$(tail -n +${ACTOR_TYPES_LINE} "${MANAGERS_PY}" | grep -n "^}" | head -1 | cut -d: -f1)
        LINE_NUM=$((ACTOR_TYPES_LINE + LINE_NUM - 1))
    fi
    if [ -n "${LINE_NUM}" ]; then
        REGISTER_BLOCK=$(mktemp)
        cat > "${REGISTER_BLOCK}" << 'REGISTER_EOF'

# Add eic-OpticksActor if available
if _has_opticks:
    actor_types["OpticksActor"] = OpticksActor

REGISTER_EOF
        head -n ${LINE_NUM} "${MANAGERS_PY}" > "${MANAGERS_PY}.new"
        cat "${REGISTER_BLOCK}" >> "${MANAGERS_PY}.new"
        tail -n +$((LINE_NUM + 1)) "${MANAGERS_PY}" >> "${MANAGERS_PY}.new"
        mv "${MANAGERS_PY}.new" "${MANAGERS_PY}"
        rm -f "${REGISTER_BLOCK}"
    fi

    echo_info "managers.py patched successfully"
fi

# ============================================================================
# STEP 8: Initialize submodules if needed
# ============================================================================
echo_info "Checking git submodules..."

cd "${OPENGATE_DIR}"
if [ -d ".git" ]; then
    # Check if pybind11 submodule is empty
    if [ ! -f "core/external/pybind11/CMakeLists.txt" ]; then
        echo_info "Initializing git submodules..."
        git submodule update --init --recursive
    else
        echo_info "Submodules already initialized"
    fi
else
    echo_warn "Not a git repository - assuming submodules are present"
fi

# ============================================================================
# STEP 9: Configure Python environment
# ============================================================================
echo_info "Configuring Python environment..."

# Check if we're in a virtual environment
VENV_CFG=""
if [ -n "${VIRTUAL_ENV}" ]; then
    VENV_CFG="${VIRTUAL_ENV}/pyvenv.cfg"
elif [ -f "/src/eic-opticks/.venv/pyvenv.cfg" ]; then
    VENV_CFG="/src/eic-opticks/.venv/pyvenv.cfg"
fi

if [ -n "${VENV_CFG}" ] && [ -f "${VENV_CFG}" ]; then
    # Enable system site-packages in the venv so we can access scipy, etc.
    if grep -q "include-system-site-packages = false" "${VENV_CFG}"; then
        echo_info "Enabling system site-packages in virtual environment..."
        sed -i 's/include-system-site-packages = false/include-system-site-packages = true/' "${VENV_CFG}"
    fi
fi

# ============================================================================
# STEP 10: Build OpenGATE with eic-Opticks support
# ============================================================================
echo ""
echo_info "Building OpenGATE with Opticks support..."

BUILD_DIR="${OPENGATE_DIR}/core/build"
rm -rf "${BUILD_DIR}"  # Clean build to ensure fresh state
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Export eic-Opticks environment
export OPTICKS_PREFIX="${OPTICKS_PREFIX}"

# Find ITK
if [ -z "${ITK_DIR}" ]; then
    # Try to find ITK automatically
    ITK_CONFIG=$(find /usr /opt -name "ITKConfig.cmake" 2>/dev/null | head -1)
    if [ -n "${ITK_CONFIG}" ]; then
        ITK_DIR=$(dirname "${ITK_CONFIG}")
        echo_info "Found ITK at: ${ITK_DIR}"
    else
        echo_error "ITK not found. Please install ITK or set ITK_DIR environment variable."
        exit 1
    fi
else
    echo_info "Using ITK_DIR from environment: ${ITK_DIR}"
fi

# Configure with eic-Opticks enabled
echo_info "Running CMake configuration..."
cmake .. \
    -DGATE_USE_OPTICKS=ON \
    -DOPTICKS_PREFIX="${OPTICKS_PREFIX}" \
    -DITK_DIR="${ITK_DIR}"

# Build
echo_info "Building (this may take a while)..."
cmake --build . --parallel $(nproc)

# ============================================================================
# STEP 11: Install OpenGATE
# ============================================================================
echo ""
echo_info "Installing OpenGATE..."

cd "${OPENGATE_DIR}"

# Install in editable mode
pip install -e . 2>&1 | tail -5

# The editable install should use our built opengate_core automatically
# But verify the path is correct
CORE_PATH=$(GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000 python3 -c "import opengate_core; print(opengate_core.__file__)" 2>/dev/null || echo "")

if [ -z "${CORE_PATH}" ] || [[ "${CORE_PATH}" != *"/src/opengate/core/build/"* ]]; then
    echo_warn "opengate_core not pointing to our build, copying manually..."
    SITE_PACKAGES=$(python3 -c "import site; print(site.getsitepackages()[0])")

    # Create opengate_core directory if needed
    mkdir -p "${SITE_PACKAGES}/opengate_core"

    # Copy our built module
    cp "${BUILD_DIR}/opengate_core.cpython-"*.so "${SITE_PACKAGES}/opengate_core/"

    # Copy __init__.py if it exists in the installed package
    if [ -f "${SITE_PACKAGES}/opengate_core/__init__.py" ]; then
        echo_info "opengate_core __init__.py exists"
    else
        # Create a minimal __init__.py
        echo "from .opengate_core import *" > "${SITE_PACKAGES}/opengate_core/__init__.py"
    fi
fi

# ============================================================================
# STEP 12: Verify installation
# ============================================================================
echo ""
echo_info "Verifying installation..."

# Set up runtime environment for eic-Opticks libraries
export LD_LIBRARY_PATH="${OPTICKS_PREFIX}/lib:${LD_LIBRARY_PATH}"
export GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000

python3 << 'VERIFY_EOF'
import sys
sys.path.insert(0, '/src/opengate')

try:
    import opengate as gate
    from opengate.managers import actor_types, _has_opticks

    print(f"  Opticks available: {_has_opticks}")
    print(f"  OpticksActor registered: {'OpticksActor' in actor_types}")

    if _has_opticks and 'OpticksActor' in actor_types:
        print("SUCCESS: OpticksActor is available!")
        sys.exit(0)
    else:
        print("WARNING: OpticksActor not fully configured")
        sys.exit(1)
except Exception as e:
    print(f"ERROR: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
VERIFY_EOF

VERIFY_RESULT=$?

echo ""
echo "============================================================"
if [ ${VERIFY_RESULT} -eq 0 ]; then
    echo_info "Setup complete!"
else
    echo_warn "Setup completed with warnings - please check the output above"
fi
echo "============================================================"
echo ""
echo "To run simulations with eic-Opticks, always set:"
echo ""
echo "  export GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000"
echo ""
echo "Example test simulation:"
echo ""
echo "  export GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000"
echo "  python3 << 'EOF'"
echo "import sys"
echo "sys.path.insert(0, '/src/opengate')"
echo "import opengate as gate"
echo "from opengate.managers import actor_types, _has_opticks"
echo "print(f'eic-Opticks available: {_has_opticks}')"
echo "print(f'OpticksActor registered: {\"OpticksActor\" in actor_types}')"
echo "EOF"
echo ""
