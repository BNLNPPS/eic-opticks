#!/usr/bin/env python3
"""
Benchmark: CPU vs GPU Optical Photon Simulation
================================================

This script compares the performance of:
1. CPU simulation: Standard Geant4 optical photon tracking
2. GPU simulation: Opticks GPU-accelerated optical photon propagation

Target: ~51 million optical photons from Cerenkov radiation in water.
Uses the same geometry as validate_opengate_opticks.py for consistency.

Based on testing: 1 electron at 10 MeV generates ~4600 Cerenkov photons
For 51 million photons: ~11000 electrons needed

Usage:
    export GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000
    export OPTICKS_MAX_SLOT=M100
    python3 benchmark_cpu_vs_gpu.py
"""

import time
import os
import sys
from pathlib import Path

# Ensure environment is set
if 'GLIBC_TUNABLES' not in os.environ:
    print("WARNING: GLIBC_TUNABLES not set. You may see TLS errors.")
    print("Run: export GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000")

import opengate as gate

# Units
m = gate.g4_units.m
cm = gate.g4_units.cm
mm = gate.g4_units.mm
MeV = gate.g4_units.MeV
eV = gate.g4_units.eV

# Configuration - same as validation
NUM_ELECTRONS = 11000  # Should generate ~51 million Cerenkov photons
ELECTRON_ENERGY = 10 * MeV
GEOM_DIR = Path(__file__).parent.parent.parent / "tests" / "geom"

def create_base_simulation():
    """Create base simulation setup matching validation geometry."""
    sim = gate.Simulation()
    sim.number_of_threads = 1
    sim.visu = False
    sim.random_seed = 12345

    # World
    sim.world.size = [1*m, 1*m, 1*m]
    sim.world.material = "G4_AIR"

    # Water box for Cerenkov radiation
    waterbox = sim.add_volume("Box", "waterbox")
    waterbox.size = [30*cm, 30*cm, 30*cm]
    waterbox.material = "G4_WATER"
    waterbox.translation = [0, 0, 0]

    # Detector inside waterbox (same as validation)
    det_thickness = 5*mm
    det_offset = 15*cm - det_thickness/2

    detector = sim.add_volume("Box", "detector")
    detector.mother = "waterbox"
    detector.size = [28*cm, 28*cm, det_thickness]
    detector.material = "G4_SILICON_DIOXIDE"
    detector.translation = [0, 0, det_offset]

    # Physics with optical
    sim.physics_manager.physics_list_name = "G4EmStandardPhysics_option4"
    sim.physics_manager.special_physics_constructors.G4OpticalPhysics = True
    sim.physics_manager.energy_range_min = 1 * eV
    sim.physics_manager.energy_range_max = 10 * MeV

    # Optical properties (same as validation)
    sim.physics_manager.optical_properties_file = str(GEOM_DIR / "WaterOpticalProperties.xml")
    sim.physics_manager.surface_properties_file = str(GEOM_DIR / "SurfaceProperties.xml")

    # Border surface with EFFICIENCY
    sim.physics_manager.add_optical_surface(
        volume_from="waterbox",
        volume_to="detector",
        g4_surface_name="DetectorSurface"
    )

    # High-energy electron source
    source = sim.add_source("GenericSource", "electron_beam")
    source.particle = "e-"
    source.energy.mono = ELECTRON_ENERGY
    source.position.type = "point"
    source.position.translation = [0, 0, -10*cm]
    source.direction.type = "momentum"
    source.direction.momentum = [0, 0, 1]
    source.n = NUM_ELECTRONS

    return sim


def run_cpu_simulation(optical_tracking=True):
    """
    Run simulation with CPU optical photon tracking.
    This uses standard Geant4 optical photon propagation.
    """
    mode_str = "ON" if optical_tracking else "OFF"
    print("\n" + "="*70)
    print(f"CPU SIMULATION (Optical Tracking {mode_str})")
    print("="*70)

    sim = create_base_simulation()

    # Disable optical physics if requested
    if not optical_tracking:
        sim.physics_manager.special_physics_constructors.G4OpticalPhysics = False

    # Add a simple actor to count optical photons (for verification)
    # We'll use SimulationStatisticsActor to get basic stats
    stats = sim.add_actor("SimulationStatisticsActor", "stats")
    stats.track_types_flag = True

    print(f"\nConfiguration:")
    print(f"  Primary particles: {NUM_ELECTRONS} electrons at {ELECTRON_ENERGY/MeV} MeV")
    if optical_tracking:
        print(f"  Expected photons: ~{NUM_ELECTRONS * 4600:,}")
    print(f"  Mode: CPU (Optical tracking {mode_str})")

    print("\nStarting CPU simulation...")
    start_time = time.time()
    sim.run()
    end_time = time.time()

    cpu_time = end_time - start_time

    # Get statistics
    stats_actor = sim.get_actor("stats")
    counts = stats_actor.counts
    track_types = counts.get("track_types", {})
    photons = track_types.get("opticalphoton", 0)

    print(f"\nCPU Simulation Results:")
    print(f"  Total time: {cpu_time:.2f} seconds")
    print(f"  Optical photons tracked: {photons:,}")

    return cpu_time, photons


def run_gpu_simulation():
    """
    Run simulation with GPU optical photon tracking via Opticks.
    Optical photons are killed on CPU and simulated on GPU.
    Returns: (total_time, gpu_only_time_ms, total_photons)
    """
    print("\n" + "="*70)
    print("GPU SIMULATION (Opticks GPU Optical Tracking)")
    print("="*70)

    sim = create_base_simulation()

    # Add OpticksActor for GPU simulation
    opticks = sim.add_actor("OpticksActor", "opticks")
    opticks.attached_to = "world"
    opticks.batch_size = 0  # Process all at end of run
    opticks.output_path = "/tmp/opticks_benchmark"
    opticks.save_to_file = False

    print(f"\nConfiguration:")
    print(f"  Primary particles: {NUM_ELECTRONS} electrons at {ELECTRON_ENERGY/MeV} MeV")
    print(f"  Expected photons: ~{NUM_ELECTRONS * 4600:,}")
    print(f"  Mode: GPU (Opticks)")

    print("\nStarting GPU simulation...")
    start_time = time.time()
    sim.run()
    end_time = time.time()

    gpu_time = end_time - start_time

    # Get Opticks statistics
    opticks_actor = sim.get_actor("opticks")
    total_photons = opticks_actor.GetTotalNumPhotons()
    total_gensteps = opticks_actor.GetTotalNumGensteps()

    print(f"\nGPU Simulation Results:")
    print(f"  Total time: {gpu_time:.2f} seconds")
    print(f"  Gensteps collected: {total_gensteps:,}")
    print(f"  Photons simulated on GPU: {total_photons:,}")

    return gpu_time, total_photons


def run_subprocess(mode):
    """Run simulation in a subprocess to avoid OpenGATE single-instance limitation."""
    import subprocess
    import json
    import re

    env = os.environ.copy()
    cmd = [sys.executable, __file__, f"--{mode}", "--subprocess"]

    result = subprocess.run(cmd, capture_output=True, text=True, env=env)

    if result.returncode != 0:
        print(f"Subprocess error:\n{result.stderr[-1000:]}")
        return None

    # Parse JSON result from stdout
    data = None
    for line in result.stdout.strip().split('\n'):
        if line.startswith('RESULT:'):
            data = json.loads(line[7:])
            break

    if data is None:
        print(f"Could not parse result from subprocess")
        return None

    # For GPU mode, also extract GPU_SIMULATE_TIME_MS from output
    if mode == "gpu":
        for line in result.stdout.split('\n'):
            if 'GPU_SIMULATE_TIME_MS:' in line:
                match = re.search(r'GPU_SIMULATE_TIME_MS:\s*(\d+)', line)
                if match:
                    data['gpu_only_ms'] = int(match.group(1))
                break

    return data


def main():
    import argparse
    import json

    parser = argparse.ArgumentParser(description="CPU vs GPU optical photon benchmark")
    parser.add_argument("--cpu", action="store_true", help="Run CPU with optical tracking ON")
    parser.add_argument("--cpu-no-optical", action="store_true", help="Run CPU with optical tracking OFF")
    parser.add_argument("--gpu", action="store_true", help="Run only GPU simulation")
    parser.add_argument("--subprocess", action="store_true", help="Internal: running as subprocess")
    args = parser.parse_args()

    if args.subprocess:
        # Running as subprocess - output results as JSON
        if args.cpu:
            cpu_time, photons = run_cpu_simulation(optical_tracking=True)
            print(f"RESULT:{json.dumps({'time': cpu_time, 'photons': photons, 'mode': 'cpu'})}")
        elif args.cpu_no_optical:
            cpu_time, photons = run_cpu_simulation(optical_tracking=False)
            print(f"RESULT:{json.dumps({'time': cpu_time, 'photons': photons, 'mode': 'cpu_no_optical'})}")
        elif args.gpu:
            gpu_time, total_photons = run_gpu_simulation()
            print(f"RESULT:{json.dumps({'time': gpu_time, 'photons': total_photons, 'mode': 'gpu'})}")
        return 0

    # Full benchmark - run simulations as subprocesses
    print("="*70)
    print("OpenGATE CPU vs GPU Optical Photon Benchmark")
    print("="*70)
    print(f"\nTarget: ~51 million Cerenkov photons in water")
    print(f"Primary particles: {NUM_ELECTRONS} electrons at {ELECTRON_ENERGY/MeV} MeV")

    # Run CPU without optical tracking (baseline)
    print("\n" + "-"*70)
    print("Running CPU simulation (optical tracking OFF) - baseline...")
    cpu_baseline = run_subprocess("cpu-no-optical")
    if cpu_baseline is None:
        print("ERROR: CPU baseline simulation failed")
        return 1
    print(f"  Baseline time: {cpu_baseline['time']:.1f}s")

    # Run CPU with optical tracking
    print("\n" + "-"*70)
    print("Running CPU simulation (optical tracking ON)...")
    cpu_optical = run_subprocess("cpu")
    if cpu_optical is None:
        print("ERROR: CPU optical simulation failed")
        return 1
    print(f"  Total time: {cpu_optical['time']:.1f}s, Photons: {cpu_optical['photons']:,}")

    # Run GPU simulation
    print("\n" + "-"*70)
    print("Running GPU simulation...")
    gpu_result = run_subprocess("gpu")
    if gpu_result is None:
        print("ERROR: GPU simulation failed")
        return 1
    print(f"  Total time: {gpu_result['time']:.1f}s, Photons: {gpu_result['photons']:,}")
    if 'gpu_only_ms' in gpu_result:
        print(f"  GPU-only time: {gpu_result['gpu_only_ms']}ms")

    # Calculate CPU optical-only time
    cpu_optical_only = cpu_optical['time'] - cpu_baseline['time']
    gpu_only_sec = gpu_result.get('gpu_only_ms', 0) / 1000.0
    total_photons = gpu_result['photons']

    # Results summary
    print("\n" + "="*70)
    print("BENCHMARK RESULTS")
    print("="*70)
    print(f"\nPhotons simulated: {total_photons:,}")

    print(f"\nTotal simulation time:")
    print(f"  CPU (no optical): {cpu_baseline['time']:.1f}s (baseline)")
    print(f"  CPU (with optical): {cpu_optical['time']:.1f}s")
    print(f"  GPU (with Opticks): {gpu_result['time']:.1f}s")

    print(f"\nOptical photon simulation time only:")
    print(f"  CPU: {cpu_optical_only:.1f}s")
    print(f"  GPU: {gpu_only_sec:.3f}s ({gpu_result.get('gpu_only_ms', 0)}ms)")

    if gpu_only_sec > 0:
        speedup = cpu_optical_only / gpu_only_sec
        print(f"\nSpeedup (optical only): {speedup:.0f}x faster with GPU")

    print(f"\nOptical photon throughput:")
    if cpu_optical_only > 0:
        print(f"  CPU: {total_photons/cpu_optical_only:,.0f} photons/sec")
    if gpu_only_sec > 0:
        print(f"  GPU: {total_photons/gpu_only_sec:,.0f} photons/sec")

    return 0


if __name__ == "__main__":
    sys.exit(main())
