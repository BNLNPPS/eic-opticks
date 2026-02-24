#!/usr/bin/env python3
"""
test_opengate_eic-opticks_scintillation.py - GPU vs CPU Scintillation Comparison

PURPOSE:
Compare Opticks GPU and Geant4 CPU optical photon simulation for scintillation
photons from a CsI crystal detector, based on ex.gdml geometry.

GEOMETRY (simplified from ex.gdml 8x8 array to single pixel):
- Air world (100 mm cube)
- CsI crystal (20x20x20 mm) - scintillation radiator
- SiPM detector (18x18x0.5 mm) inside crystal at +Z face

SCINTILLATION PROPERTIES (CsI):
- Yield: 5000 photons/MeV
- Fast component: 87%, tau = 21.5 ns
- Slow component: 13%, tau = 43.8 ns
- Emission peak: 2.896 eV (~428 nm)

USAGE:
    U4SensorIdentifierDefault__GLOBAL_SENSOR_BOUNDARY_LIST='G4_CESIUM_IODIDE/DetectorSurface_0001//G4_SILICON_DIOXIDE' \
    GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000 \
    OPTICKS_MAX_SLOT=M1 \
    python3 test_opengate_eic-opticks_scintillation.py

    # CPU only:
    python3 test_opengate_eic-opticks_scintillation.py --cpu

    # GPU only:
    ... python3 test_opengate_eic-opticks_scintillation.py --gpu
"""

import opengate as gate
from pathlib import Path
import argparse
import subprocess
import sys
import json
import os

# Units
m = gate.g4_units.m
cm = gate.g4_units.cm
mm = gate.g4_units.mm
MeV = gate.g4_units.MeV
keV = gate.g4_units.keV
eV = gate.g4_units.eV

N_GAMMAS = 100


def create_simulation():
    """Create simulation with CsI crystal scintillation detector geometry."""

    sim = gate.Simulation()
    sim.visu = False
    sim.number_of_threads = 1
    sim.random_seed = 12345

    # World volume (air)
    world = sim.world
    world.size = [100 * mm, 100 * mm, 100 * mm]
    world.material = "G4_AIR"

    # CsI crystal (scintillator) - 20x20x20 mm for good gamma interaction
    crystal = sim.add_volume("Box", "crystal")
    crystal.size = [20 * mm, 20 * mm, 20 * mm]
    crystal.material = "G4_CESIUM_IODIDE"
    crystal.translation = [0, 0, 0]
    crystal.color = [0, 0, 1, 0.5]

    # SiPM detector inside crystal at +Z face
    # Daughter of crystal so border surface works for Opticks sensor ID
    det_thickness = 0.5 * mm
    det_offset = 10 * mm - det_thickness / 2
    sipm = sim.add_volume("Box", "sipm")
    sipm.mother = "crystal"
    sipm.size = [18 * mm, 18 * mm, det_thickness]
    sipm.material = "G4_SILICON_DIOXIDE"
    sipm.translation = [0, 0, det_offset]
    sipm.color = [0, 1, 0, 0.5]

    # Physics
    sim.physics_manager.physics_list_name = "G4EmStandardPhysics_option4"
    sim.physics_manager.special_physics_constructors.G4OpticalPhysics = True
    sim.physics_manager.energy_range_min = 1 * eV
    sim.physics_manager.energy_range_max = 10 * MeV

    # Optical & scintillation properties from XML
    geom_dir = Path(__file__).parent / "geom"
    sim.physics_manager.optical_properties_file = str(
        geom_dir / "CrystalOpticalProperties.xml"
    )
    sim.physics_manager.surface_properties_file = str(
        geom_dir / "CrystalSurfaceProperties.xml"
    )

    # Optical surfaces
    sim.physics_manager.add_optical_surface(
        volume_from="world",
        volume_to="crystal",
        g4_surface_name="CrystalSurface",
    )
    sim.physics_manager.add_optical_surface(
        volume_from="crystal",
        volume_to="sipm",
        g4_surface_name="DetectorSurface",
    )

    # Source: 511 keV gamma
    source = sim.add_source("GenericSource", "gamma_beam")
    source.particle = "gamma"
    source.energy.mono = 511 * keV
    source.position.type = "point"
    source.position.translation = [0, 0, -5 * mm]
    source.direction.type = "momentum"
    source.direction.momentum = [0, 0, 1]
    source.n = N_GAMMAS

    # Statistics actor
    stats = sim.add_actor("SimulationStatisticsActor", "stats")
    stats.track_types_flag = True

    return sim


def run_cpu():
    """Run CPU simulation with Geant4 optical tracking."""
    print("Running CPU simulation (Geant4 optical tracking)...")

    sim = create_simulation()

    # OpticksActor in CPU mode: tracks photons on CPU, counts hits at boundaries
    opticks = sim.add_actor("OpticksActor", "opticks")
    opticks.attached_to = "world"
    opticks.cpu_mode = True
    opticks.detector_volume_name = "sipm"

    sim.run()

    stats = sim.get_actor("stats")
    counts = stats.counts
    track_types = counts.get("track_types", {})
    optical_photon_tracks = track_types.get("opticalphoton", 0)

    opticks_actor = sim.get_actor("opticks")
    cpu_hits = opticks_actor.GetTotalNumCpuHits()

    results = {
        "mode": "cpu",
        "optical_photon_tracks": optical_photon_tracks,
        "total_hits": cpu_hits,
    }

    print(f"  Optical photon tracks: {optical_photon_tracks}")
    print(f"  CPU hits (boundary detection): {cpu_hits}")
    return results


def run_gpu():
    """Run GPU simulation with Opticks."""
    print("Running GPU simulation (Opticks)...")

    sim = create_simulation()

    opticks = sim.add_actor("OpticksActor", "opticks")
    opticks.attached_to = "world"
    opticks.batch_size = 0
    opticks.output_path = "/tmp/opticks_scintillation_test"
    opticks.save_to_file = False
    opticks.verbose_batch = False

    sim.run()

    opticks_actor = sim.get_actor("opticks")

    results = {
        "mode": "gpu",
        "total_photons": opticks_actor.GetTotalNumPhotons(),
        "total_hits": opticks_actor.GetTotalNumHits(),
        "total_gensteps": opticks_actor.GetTotalNumGensteps(),
    }

    print(f"  Total photons: {results['total_photons']}")
    print(f"  Total hits: {results['total_hits']}")
    return results


def run_subprocess(mode):
    """Run simulation in a subprocess (OpenGATE single-instance limitation)."""
    env = os.environ.copy()
    cmd = [sys.executable, __file__, f"--{mode}", "--subprocess"]

    result = subprocess.run(cmd, capture_output=True, text=True, env=env)

    if result.returncode != 0:
        print(f"Subprocess stderr:\n{result.stderr[-2000:]}")
        return None

    for line in result.stdout.strip().split('\n'):
        if line.startswith('RESULT:'):
            return json.loads(line[7:])

    print(f"Could not parse result from subprocess:\n{result.stdout[-2000:]}")
    return None


def compare_results(cpu_results, gpu_results):
    """Compare CPU and GPU scintillation results."""
    print("\n" + "=" * 60)
    print("SCINTILLATION VALIDATION RESULTS")
    print("=" * 60)

    cpu_photons = cpu_results['optical_photon_tracks']
    cpu_hits = cpu_results.get('total_hits', 0)
    gpu_photons = gpu_results['total_photons']
    gpu_hits = gpu_results['total_hits']

    print(f"\nCPU (Geant4):")
    print(f"  Optical photon tracks: {cpu_photons:,}")
    print(f"  Hits (boundary detection): {cpu_hits:,}")

    print(f"\nGPU (Opticks):")
    print(f"  Total photons generated: {gpu_photons:,}")
    print(f"  Total hits detected: {gpu_hits:,}")
    print(f"  Total gensteps: {gpu_results['total_gensteps']:,}")

    print("\n" + "=" * 60)
    print("COMPARISON")
    print("=" * 60)

    if cpu_photons > 0 and gpu_photons > 0:
        ratio = gpu_photons / cpu_photons
        diff_pct = abs(1.0 - ratio) * 100
        print(f"\nPhoton count ratio (GPU/CPU): {ratio:.4f}")
        print(f"  CPU: {cpu_photons:,}  GPU: {gpu_photons:,}  Diff: {diff_pct:.2f}%")

    if cpu_hits > 0 and gpu_hits > 0:
        hit_ratio = gpu_hits / cpu_hits
        hit_diff_pct = abs(1.0 - hit_ratio) * 100
        print(f"\nHit count ratio (GPU/CPU): {hit_ratio:.4f}")
        print(f"  CPU: {cpu_hits:,}  GPU: {gpu_hits:,}  Diff: {hit_diff_pct:.2f}%")

    if cpu_photons > 0:
        print(f"\nCPU detection efficiency: {cpu_hits / cpu_photons:.2%}")
    if gpu_photons > 0:
        print(f"GPU detection efficiency: {gpu_hits / gpu_photons:.2%}")

    print("\n" + "=" * 60)
    print("VALIDATION CHECKS")
    print("=" * 60)

    checks_passed = 0
    total_checks = 5

    # Check 1: CPU produced scintillation photons
    if cpu_photons > 0:
        print("[PASS] CPU produced scintillation photons")
        checks_passed += 1
    else:
        print("[FAIL] CPU produced no scintillation photons")

    # Check 2: GPU produced photons
    if gpu_photons > 0:
        print("[PASS] GPU produced scintillation photons")
        checks_passed += 1
    else:
        print("[FAIL] GPU produced no scintillation photons")

    # Check 3: GPU produced hits
    if gpu_hits > 0:
        print("[PASS] GPU detected hits")
        checks_passed += 1
    else:
        print("[FAIL] GPU detected no hits")

    # Check 4: Photon counts match within 5%
    if cpu_photons > 0 and gpu_photons > 0:
        diff_pct = abs(1.0 - gpu_photons / cpu_photons) * 100
        if diff_pct <= 5.0:
            print(f"[PASS] Photon counts match within 5% (diff: {diff_pct:.2f}%)")
            checks_passed += 1
        elif diff_pct <= 10.0:
            print(f"[WARN] Photon counts differ by {diff_pct:.2f}%")
            checks_passed += 0.5
        else:
            print(f"[FAIL] Photon counts differ by {diff_pct:.2f}%")
    else:
        print("[SKIP] Cannot compare photon counts")

    # Check 5: Hit counts match within 10%
    if cpu_hits > 0 and gpu_hits > 0:
        hit_diff_pct = abs(1.0 - gpu_hits / cpu_hits) * 100
        if hit_diff_pct <= 10.0:
            print(f"[PASS] Hit counts match within 10% (diff: {hit_diff_pct:.2f}%)")
            checks_passed += 1
        elif hit_diff_pct <= 20.0:
            print(f"[WARN] Hit counts differ by {hit_diff_pct:.2f}%")
            checks_passed += 0.5
        else:
            print(f"[FAIL] Hit counts differ by {hit_diff_pct:.2f}%")
    elif cpu_hits == 0:
        print("[WARN] CPU detected no hits")
    else:
        print("[SKIP] Cannot compare hit counts")

    print(f"\nValidation: {checks_passed}/{total_checks} checks passed")

    if checks_passed >= total_checks - 0.5:
        print("\nVALIDATION SUCCESSFUL")
        return True
    else:
        print("\nVALIDATION FAILED")
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Scintillation test: compare CPU vs GPU optical photon simulation"
    )
    parser.add_argument("--cpu", action="store_true", help="Run only CPU simulation")
    parser.add_argument("--gpu", action="store_true", help="Run only GPU simulation")
    parser.add_argument("--subprocess", action="store_true", help="Internal: running as subprocess")
    args = parser.parse_args()

    if args.subprocess:
        if args.cpu:
            results = run_cpu()
        elif args.gpu:
            results = run_gpu()
        else:
            results = {}
        print(f"RESULT:{json.dumps(results)}")
        return 0

    if args.cpu:
        results = run_cpu()
        print(f"\nResults: {results}")
        return 0

    if args.gpu:
        results = run_gpu()
        print(f"\nResults: {results}")
        return 0

    # Full validation: CPU and GPU as subprocesses
    print("=" * 60)
    print("Scintillation Validation: CPU vs GPU")
    print("=" * 60)
    print(f"Source: {N_GAMMAS} gammas at 511 keV into CsI crystal")
    print(f"Scintillation yield: 5000 photons/MeV")

    print("\n" + "-" * 60)
    cpu_results = run_subprocess("cpu")
    if cpu_results is None:
        print("ERROR: CPU simulation failed")
        return 1

    print("\n" + "-" * 60)
    gpu_results = run_subprocess("gpu")
    if gpu_results is None:
        print("ERROR: GPU simulation failed")
        return 1

    success = compare_results(cpu_results, gpu_results)
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
