# OpenGATE Opticks Integration Files

This directory contains all files needed to add GPU optical photon simulation
support to OpenGATE using Opticks/OptiX.

## Files

### C++ Source Files (copy to `opengate/core/opengate_core/opengate_lib/`)

| File | Description |
|------|-------------|
| `GateOpticksActor.cpp` | Main C++ implementation - collects gensteps, runs GPU simulation |
| `GateOpticksActor.h` | Header file with class definition |
| `pyGateOpticksActor.cpp` | pybind11 bindings for Python access |

### Python Files (copy to `opengate/opengate/actors/`)

| File | Description |
|------|-------------|
| `opticksactors.py` | Python wrapper class for OpticksActor |

### Patch Files (in `patches/` subdirectory)

| File | Description |
|------|-------------|
| `CMakeLists.txt.patch` | Adds Opticks build configuration |
| `CMakeLists_link.txt.patch` | Adds Opticks library linking |
| `opengate_core.cpp.patch` | Adds Python binding registration |
| `managers.py.patch` | Adds actor type registration |

## Manual Installation

If the automated script doesn't work, apply changes manually:

### 1. Copy source files

```bash
# C++ files
cp GateOpticksActor.cpp /path/to/opengate/core/opengate_core/opengate_lib/
cp GateOpticksActor.h /path/to/opengate/core/opengate_core/opengate_lib/
cp pyGateOpticksActor.cpp /path/to/opengate/core/opengate_core/opengate_lib/

# Python file
cp opticksactors.py /path/to/opengate/opengate/actors/
```

### 2. Edit CMakeLists.txt

Add after `endif (ITK_FOUND)`:

```cmake
# Optional: Opticks for GPU optical photon propagation
option(GATE_USE_OPTICKS "Enable GPU optical photon propagation with Opticks" OFF)
if(GATE_USE_OPTICKS)
    set(OPTICKS_PREFIX "$ENV{OPTICKS_PREFIX}" CACHE PATH "Opticks installation prefix")
    if(NOT OPTICKS_PREFIX)
        message(FATAL_ERROR "OPTICKS_PREFIX not set.")
    endif()

    include_directories(
        ${OPTICKS_PREFIX}/include/eic-opticks
        ${OPTICKS_PREFIX}/include/eic-opticks/sysrap
        ${OPTICKS_PREFIX}/include/eic-opticks/qudarap
        ${OPTICKS_PREFIX}/include/eic-opticks/u4
        ${OPTICKS_PREFIX}/include/eic-opticks/g4cx
        ${OPTICKS_PREFIX}/include/eic-opticks/CSG
        ${OPTICKS_PREFIX}/include/eic-opticks/CSGOptiX
    )

    find_package(CUDA REQUIRED)
    include_directories(${CUDA_INCLUDE_DIRS})
    add_definitions(-DGATE_USE_OPTICKS)
    add_definitions(-DRNG_PHILOX)

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
```

Modify `target_link_libraries` at the end:

```cmake
if(GATE_USE_OPTICKS)
    target_link_libraries(opengate_core PRIVATE
        pybind11::module ${Geant4_LIBRARIES} Threads::Threads
        ${ITK_LIBRARIES} fmt::fmt-header-only ${OPTICKS_LIBRARIES})
else()
    target_link_libraries(opengate_core PRIVATE
        pybind11::module ${Geant4_LIBRARIES} Threads::Threads
        ${ITK_LIBRARIES} fmt::fmt-header-only)
endif()
```

### 3. Edit opengate_core.cpp

Add forward declaration (around line 325):

```cpp
void init_GateOpticksActor(py::module &m);
```

Add initialization call (around line 636):

```cpp
init_GateOpticksActor(m);
```

### 4. Edit managers.py

Add import (after other actor imports):

```python
# Opticks GPU optical photon propagation (optional)
try:
    from .actors.opticksactors import OpticksActor
    _has_opticks = True
except ImportError:
    _has_opticks = False
```

Add to actor_types (after the dict definition):

```python
# Add OpticksActor if available
if _has_opticks:
    actor_types["OpticksActor"] = OpticksActor
```

### 5. Build

```bash
cd /path/to/opengate/core/build
export OPTICKS_PREFIX=/opt/eic-opticks
cmake .. -DGATE_USE_OPTICKS=ON
cmake --build . --parallel 4
cp opengate_core.cpython-*.so /usr/local/lib/python3.10/dist-packages/opengate_core/
```

## Architecture

The integration works as follows:

1. **SteppingAction**: When Geant4 generates Cerenkov/Scintillation photons,
   the actor immediately kills them on CPU and collects "gensteps" (generation
   parameters) instead.

2. **EndOfRunAction**: All collected gensteps are sent to the GPU for parallel
   photon propagation using Opticks/OptiX ray tracing.

3. **Results**: Hit counts and photon statistics are returned to Python.

## Critical Code Sequence

The GPU initialization MUST follow this order:

```cpp
// 1. Create SEvt for GPU mode (enables device detection)
SEvt::Create_EGPU();

// 2. Set up geometry (converts to GPU format)
gx = G4CXOpticks::SetGeometry(world);

// 3. Verify QSim is available (required for simulation)
QSim* qs = QSim::Get();
```

## Usage

### Running Simulations

Required environment variables:

```shell
export GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000
export OPTICKS_MAX_SLOT=M100
python3 test_opengate_eic-opticks_cerenkov.py
```

### Example

```python
import opengate as gate

sim = gate.Simulation()
# ... configure geometry, physics, sources ...

# Add OpticksActor for GPU optical photon simulation
opticks = sim.add_actor("OpticksActor", "opticks")
opticks.attached_to = "world"
opticks.batch_size = 0  # Process all at end of run
opticks.output_path = "/tmp/opticks_output"
opticks.save_to_file = True

sim.run()

# Get results
print(f"Total photons: {opticks.GetTotalNumPhotons()}")
print(f"Total hits: {opticks.GetTotalNumHits()}")
```

## Key Requirements for Hit Detection

### 1. Volume Hierarchy

Detector volumes must be children of the radiator volume (not world). This ensures photons cross the correct border surface.

```python
# Correct: detector is child of waterbox
detector = sim.add_volume("Box", "detector")
detector.mother = "waterbox"  # NOT "world"
detector.translation = [0, 0, offset]
```

### 2. Border Surfaces

Optical surfaces with EFFICIENCY property must connect volumes:

```python
sim.physics_manager.add_optical_surface(
    volume_from="waterbox",
    volume_to="detector",
    g4_surface_name="DetectorSurface"
)
```

### 3. Surface Properties

Define EFFICIENCY in the surface properties XML file (e.g., `SurfaceProperties.xml`):

```xml
<opticalsurface name="DetectorSurface" model="glisur" finish="polished" type="dielectric_dielectric" value="1">
  <property name="EFFICIENCY" ref="DETECTOR_EFFICIENCY"/>
</opticalsurface>
```

## GDML vs OpenGATE: Surface Definition Differences

**GDML approach** (standalone eic-opticks):
- All-in-one file containing geometry, materials, surfaces, and properties
- Optical surfaces defined directly with `<opticalsurface>` elements
- Opticks reads the GDML and automatically picks up surface definitions

**OpenGATE approach**:
- Geometry is built programmatically (not from GDML)
- Optical properties loaded from separate XML files
- Border surfaces must be explicitly created via `sim.physics_manager.add_optical_surface()` to connect surface definitions to volume pairs

The physics is identical - both approaches create `G4LogicalBorderSurface` objects with EFFICIENCY properties. OpenGATE requires explicit API calls to wire things together since it doesn't parse a monolithic GDML file.

## Validation

The `validate_opengate_opticks.py` script compares CPU (Geant4) and GPU (Opticks) optical photon simulation:

```shell
GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000 \
OPTICKS_MAX_SLOT=M100 \
python3 validate_opengate_opticks.py
```

### Validation Results (11,000 electrons, ~51M photons)

| Metric | CPU (Geant4) | GPU (Opticks) | Difference |
|--------|--------------|---------------|------------|
| Optical photons | 51,107,547 | 51,046,884 | 0.12% |
| Hits detected | 3,254,998 | 3,246,930 | 0.25% |
| Detection efficiency | 6.37% | 6.36% | - |

All 5 validation checks pass.

## Performance

Benchmark results on NVIDIA RTX 4090:

| Electrons | Photons | CPU Time | GPU Time | Speedup |
|-----------|---------|----------|----------|---------|
| 50 | 232k | 3.66s | 1.18s | 3.1x total, 305x GPU-only |
| 200 | 930k | 12.66s | 1.99s | 6.4x total, 550x GPU-only |
| 500 | 2.3M | 30.92s | 3.58s | 8.6x total, 687x GPU-only |
