#!/usr/bin/env python3
"""
validate_opengate_opticks.py - Compare CPU vs GPU optical photon simulation

PURPOSE:
Validates Opticks GPU integration by comparing results with standard Geant4 CPU simulation.

USAGE:
    # Run full validation (both CPU and GPU):
    GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000 \
    OPTICKS_MAX_SLOT=M100 \
    python3 validate_opengate_opticks.py

    # Run only CPU mode:
    python3 validate_opengate_opticks.py --cpu

    # Run only GPU mode:
    GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000 \
    OPTICKS_MAX_SLOT=M100 \
    python3 validate_opengate_opticks.py --gpu
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
eV = gate.g4_units.eV


def create_base_simulation():
    """Create base simulation geometry and physics."""

    sim = gate.Simulation()
    sim.visu = False
    sim.number_of_threads = 1
    sim.random_seed = 12345  # Same seed for reproducibility

    # World
    world = sim.world
    world.size = [1*m, 1*m, 1*m]
    world.material = "G4_AIR"

    # Water box - Cerenkov radiator
    waterbox = sim.add_volume("Box", "waterbox")
    waterbox.size = [30*cm, 30*cm, 30*cm]
    waterbox.material = "G4_WATER"
    waterbox.translation = [0, 0, 0]

    # Detector inside waterbox
    det_thickness = 5*mm
    det_offset = 15*cm - det_thickness/2

    # Single detector (+Z face)
    detector = sim.add_volume("Box", "detector")
    detector.mother = "waterbox"
    detector.size = [28*cm, 28*cm, det_thickness]
    detector.material = "G4_SILICON_DIOXIDE"
    detector.translation = [0, 0, det_offset]

    # Physics
    sim.physics_manager.physics_list_name = "G4EmStandardPhysics_option4"
    sim.physics_manager.special_physics_constructors.G4OpticalPhysics = True
    sim.physics_manager.energy_range_min = 1 * eV
    sim.physics_manager.energy_range_max = 10 * MeV

    # Optical properties
    script_dir = Path(__file__).parent
    sim.physics_manager.optical_properties_file = str(script_dir / "WaterOpticalProperties.xml")
    sim.physics_manager.surface_properties_file = str(script_dir / "SurfaceProperties.xml")

    # Border surface with EFFICIENCY
    sim.physics_manager.add_optical_surface(
        volume_from="waterbox",
        volume_to="detector",
        g4_surface_name="DetectorSurface"
    )

    # Electron source
    source = sim.add_source("GenericSource", "electron_beam")
    source.particle = "e-"
    source.energy.mono = 10 * MeV
    source.position.type = "point"
    source.position.translation = [0, 0, -10*cm]
    source.direction.type = "momentum"
    source.direction.momentum = [0, 0, 1]
    source.n = 11000  # ~50 million optical photons (~4500 per electron)

    # Statistics actor
    stats = sim.add_actor("SimulationStatisticsActor", "stats")
    stats.track_types_flag = True

    return sim


def run_cpu():
    """Run CPU simulation and return results."""
    print("Running CPU simulation (Geant4 optical tracking)...")

    sim = create_base_simulation()

    # Add OpticksActor in CPU mode to count hits at detector boundaries
    opticks = sim.add_actor("OpticksActor", "opticks")
    opticks.attached_to = "world"
    opticks.cpu_mode = True  # Track photons on CPU, count hits at boundaries
    opticks.detector_volume_name = "detector"

    sim.run()

    # Get track counts from stats
    stats = sim.get_actor("stats")
    counts = stats.counts
    track_types = counts.get("track_types", {})
    optical_photon_tracks = track_types.get("opticalphoton", 0)

    # Get CPU hits from OpticksActor
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
    """Run GPU simulation and return results."""
    print("Running GPU simulation (Opticks)...")

    sim = create_base_simulation()

    # Add OpticksActor for GPU tracking
    opticks = sim.add_actor("OpticksActor", "opticks")
    opticks.attached_to = "world"
    opticks.batch_size = 0
    opticks.output_path = "/tmp/validate_gpu"
    opticks.save_to_file = False
    opticks.verbose_batch = False

    Path("/tmp/validate_gpu").mkdir(exist_ok=True)
    sim.run()

    # Get results
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
    """Run simulation in a subprocess to avoid OpenGATE single-instance limitation."""
    env = os.environ.copy()
    cmd = [sys.executable, __file__, f"--{mode}", "--subprocess"]

    result = subprocess.run(cmd, capture_output=True, text=True, env=env)

    if result.returncode != 0:
        print(f"Subprocess error:\n{result.stderr}")
        return None

    # Parse JSON result from stdout
    for line in result.stdout.strip().split('\n'):
        if line.startswith('RESULT:'):
            return json.loads(line[7:])

    print(f"Could not parse result from subprocess:\n{result.stdout}")
    return None


def compare_results(cpu_results, gpu_results):
    """Compare CPU and GPU results."""
    print("\n" + "="*60)
    print("VALIDATION RESULTS")
    print("="*60)

    cpu_hits = cpu_results.get('total_hits', 0)

    print("\nCPU (Geant4):")
    print(f"  Optical photon tracks: {cpu_results['optical_photon_tracks']:,}")
    print(f"  Hits (boundary detection): {cpu_hits:,}")

    print("\nGPU (Opticks):")
    print(f"  Total photons generated: {gpu_results['total_photons']:,}")
    print(f"  Total hits detected: {gpu_results['total_hits']:,}")

    # Calculate comparison metrics
    cpu_photons = cpu_results['optical_photon_tracks']
    gpu_photons = gpu_results['total_photons']
    gpu_hits = gpu_results['total_hits']

    print("\n" + "="*60)
    print("COMPARISON")
    print("="*60)

    # Photon count comparison
    if cpu_photons > 0 and gpu_photons > 0:
        ratio = gpu_photons / cpu_photons
        diff = abs(gpu_photons - cpu_photons)
        diff_pct = diff / cpu_photons * 100
        print(f"\nPhoton count ratio (GPU/CPU): {ratio:.4f}")
        print(f"  CPU optical photons: {cpu_photons:,}")
        print(f"  GPU optical photons: {gpu_photons:,}")
        print(f"  Difference: {diff:,} ({diff_pct:.2f}%)")

    # Hit count comparison
    if cpu_hits > 0 and gpu_hits > 0:
        hit_ratio = gpu_hits / cpu_hits
        hit_diff = abs(gpu_hits - cpu_hits)
        hit_diff_pct = hit_diff / cpu_hits * 100
        print(f"\nHit count ratio (GPU/CPU): {hit_ratio:.4f}")
        print(f"  CPU hits: {cpu_hits:,}")
        print(f"  GPU hits: {gpu_hits:,}")
        print(f"  Difference: {hit_diff:,} ({hit_diff_pct:.2f}%)")

    # Detection efficiency comparison
    if cpu_photons > 0 and cpu_hits > 0:
        cpu_efficiency = cpu_hits / cpu_photons
        print(f"\nCPU detection efficiency: {cpu_efficiency:.2%}")
    if gpu_photons > 0:
        gpu_efficiency = gpu_hits / gpu_photons
        print(f"GPU detection efficiency: {gpu_efficiency:.2%}")

    print("\n" + "="*60)
    print("VALIDATION CHECKS")
    print("="*60)

    checks_passed = 0
    total_checks = 5

    # Check 1: CPU produced optical photons
    if cpu_photons > 0:
        print("[PASS] CPU simulation produced optical photons")
        checks_passed += 1
    else:
        print("[FAIL] CPU simulation produced no optical photons")

    # Check 2: GPU produced photons
    if gpu_photons > 0:
        print("[PASS] GPU simulation produced photons")
        checks_passed += 1
    else:
        print("[FAIL] GPU simulation produced no photons")

    # Check 3: GPU produced hits
    if gpu_hits > 0:
        print("[PASS] GPU simulation detected hits")
        checks_passed += 1
    else:
        print("[FAIL] GPU simulation detected no hits")

    # Check 4: Photon counts match within 5%
    if cpu_photons > 0 and gpu_photons > 0:
        ratio = gpu_photons / cpu_photons
        diff_pct = abs(1.0 - ratio) * 100
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
        hit_ratio = gpu_hits / cpu_hits
        hit_diff_pct = abs(1.0 - hit_ratio) * 100
        if hit_diff_pct <= 10.0:
            print(f"[PASS] Hit counts match within 10% (diff: {hit_diff_pct:.2f}%)")
            checks_passed += 1
        elif hit_diff_pct <= 20.0:
            print(f"[WARN] Hit counts differ by {hit_diff_pct:.2f}%")
            checks_passed += 0.5
        else:
            print(f"[FAIL] Hit counts differ by {hit_diff_pct:.2f}%")
    elif cpu_hits == 0:
        print("[WARN] CPU detected no hits - boundary detection may not be working")
    else:
        print("[SKIP] Cannot compare hit counts")

    print(f"\nValidation: {checks_passed}/{total_checks} checks passed")

    if checks_passed >= total_checks - 0.5:
        print("\n" + "="*60)
        print("VALIDATION SUCCESSFUL")
        print("="*60)
        return True
    else:
        print("\n" + "="*60)
        print("VALIDATION FAILED")
        print("="*60)
        return False


def main():
    parser = argparse.ArgumentParser(description="Validate OpenGATE + Opticks integration")
    parser.add_argument("--cpu", action="store_true", help="Run only CPU simulation")
    parser.add_argument("--gpu", action="store_true", help="Run only GPU simulation")
    parser.add_argument("--subprocess", action="store_true", help="Internal: running as subprocess")
    args = parser.parse_args()

    if args.subprocess:
        # Running as subprocess - output results as JSON
        if args.cpu:
            results = run_cpu()
        elif args.gpu:
            results = run_gpu()
        else:
            results = {}
        print(f"RESULT:{json.dumps(results)}")
        return 0

    if args.cpu:
        # Only run CPU
        results = run_cpu()
        print(f"\nResults: {results}")
        return 0

    if args.gpu:
        # Only run GPU
        results = run_gpu()
        print(f"\nResults: {results}")
        return 0

    # Full validation - run both as subprocesses
    print("="*60)
    print("OpenGATE + Opticks Validation")
    print("="*60)
    print("\nRunning CPU and GPU simulations for comparison...")

    print("\n" + "-"*60)
    cpu_results = run_subprocess("cpu")
    if cpu_results is None:
        print("ERROR: CPU simulation failed")
        return 1

    print("\n" + "-"*60)
    gpu_results = run_subprocess("gpu")
    if gpu_results is None:
        print("ERROR: GPU simulation failed")
        return 1

    # Compare
    success = compare_results(cpu_results, gpu_results)
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
