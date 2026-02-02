#!/usr/bin/env python3
"""
test_opengate_eic-opticks_cerenkov.py - GPU Optical Photon Test with Detector

PURPOSE:
Test eic-Opticks GPU integration with a detector that registers hits.
Includes a water box with PMT-like detector surfaces to capture Cerenkov photons.

GEOMETRY:
- Air world (1m cube)
- Water box (30x30x30 cm) at center - Cerenkov radiator
- Detector boxes on 4 sides of water box - photon detectors

EXPECTED OUTPUT:
- ~4000-5000 Cerenkov photons per electron
- Hits detected based on detector efficiency (~20-30% QE)
- GPU simulation time: ~10-50 ms

USAGE:
    U4SensorIdentifierDefault__GLOBAL_SENSOR_BOUNDARY_LIST='G4_WATER/DetectorSurface_0000//G4_SILICON_DIOXIDE' \
    GLIBC_TUNABLES=glibc.rtld.optional_static_tls=2000000 \
    OPTICKS_MAX_SLOT=M1 \
    python3 test_opengate_eic-opticks_cerenkov.py
"""

import opengate as gate
from pathlib import Path

# Units for readability
m = gate.g4_units.m
cm = gate.g4_units.cm
mm = gate.g4_units.mm
MeV = gate.g4_units.MeV
keV = gate.g4_units.keV
eV = gate.g4_units.eV


def create_simulation():
    """Create simulation with optical properties and detectors for Cerenkov detection."""

    sim = gate.Simulation()
    sim.visu = False
    sim.number_of_threads = 1
    sim.random_seed = 12345  # Fixed seed for reproducibility

    # ============================================================
    # GEOMETRY: Air world with water box and detectors
    # ============================================================

    # World volume (air)
    world = sim.world
    world.size = [1*m, 1*m, 1*m]
    world.material = "G4_AIR"

    # Water box - Cerenkov photons generated here
    waterbox = sim.add_volume("Box", "waterbox")
    waterbox.size = [30*cm, 30*cm, 30*cm]
    waterbox.material = "G4_WATER"
    waterbox.translation = [0, 0, 0]

    # Detector thickness - place detectors INSIDE waterbox at the surfaces
    det_thickness = 5*mm
    det_offset = 15*cm - det_thickness/2  # Place just inside waterbox boundary

    # Detector boxes surrounding the water box (4 sides)
    # Using G4_SILICON_DIOXIDE as detector material (glass-like)

    # +Z detector (front) - Name includes "PMT" for eic-Opticks sensor detection
    # Detectors placed as children of waterbox so border surfaces work correctly
    det_pz = sim.add_volume("Box", "PMT_pz")
    det_pz.mother = "waterbox"
    det_pz.size = [28*cm, 28*cm, det_thickness]  # Slightly smaller to fit inside
    det_pz.material = "G4_SILICON_DIOXIDE"
    det_pz.translation = [0, 0, det_offset]
    det_pz.color = [0, 1, 0, 0.5]  # Green, semi-transparent

    # -Z detector (back)
    det_nz = sim.add_volume("Box", "PMT_nz")
    det_nz.mother = "waterbox"
    det_nz.size = [28*cm, 28*cm, det_thickness]
    det_nz.material = "G4_SILICON_DIOXIDE"
    det_nz.translation = [0, 0, -det_offset]
    det_nz.color = [0, 1, 0, 0.5]

    # +X detector (right)
    det_px = sim.add_volume("Box", "PMT_px")
    det_px.mother = "waterbox"
    det_px.size = [det_thickness, 28*cm, 28*cm]
    det_px.material = "G4_SILICON_DIOXIDE"
    det_px.translation = [det_offset, 0, 0]
    det_px.color = [0, 1, 0, 0.5]

    # -X detector (left)
    det_nx = sim.add_volume("Box", "PMT_nx")
    det_nx.mother = "waterbox"
    det_nx.size = [det_thickness, 28*cm, 28*cm]
    det_nx.material = "G4_SILICON_DIOXIDE"
    det_nx.translation = [-det_offset, 0, 0]
    det_nx.color = [0, 1, 0, 0.5]

    # ============================================================
    # PHYSICS: Enable optical processes
    # ============================================================
    sim.physics_manager.physics_list_name = "G4EmStandardPhysics_option4"
    sim.physics_manager.special_physics_constructors.G4OpticalPhysics = True
    sim.physics_manager.energy_range_min = 1 * eV
    sim.physics_manager.energy_range_max = 10 * MeV
    sim.physics_manager.set_production_cut("waterbox", "electron", 0.1 * mm)

    # Optical properties files
    optical_props_file = Path(__file__).parent / "WaterOpticalProperties.xml"
    surface_props_file = Path(__file__).parent / "SurfaceProperties.xml"
    sim.physics_manager.optical_properties_file = str(optical_props_file)
    sim.physics_manager.surface_properties_file = str(surface_props_file)

    # ============================================================
    # OPTICAL SURFACES: Define detector surfaces
    # ============================================================
    # Create border surfaces between water and each detector
    # These surfaces have detection efficiency (EFFICIENCY property)

    detector_names = ["PMT_pz", "PMT_nz", "PMT_px", "PMT_nx"]

    for det_name in detector_names:
        # Border surface from water to detector
        sim.physics_manager.add_optical_surface(
            volume_from="waterbox",
            volume_to=det_name,
            g4_surface_name="DetectorSurface"  # References surface in XML
        )

    # ============================================================
    # SOURCE: Electron beam
    # ============================================================
    source = sim.add_source("GenericSource", "electron_beam")
    source.particle = "e-"
    source.energy.mono = 10 * MeV
    source.position.type = "point"
    source.position.translation = [0, 0, -20*cm]  # Start outside water
    source.direction.type = "momentum"
    source.direction.momentum = [0, 0, 1]  # Travel into water
    source.n = 5  # 5 electrons for quick test

    # ============================================================
    # OPTICKS ACTOR: GPU optical photon simulation
    # ============================================================
    opticks_actor = sim.add_actor("OpticksActor", "opticks")
    opticks_actor.attached_to = "world"
    opticks_actor.batch_size = 0  # Process all at end
    opticks_actor.output_path = "/tmp/opticks_test"
    opticks_actor.save_to_file = True  # Save photon data for analysis
    opticks_actor.verbose_batch = True

    # Statistics actor
    stats = sim.add_actor("SimulationStatisticsActor", "stats")
    stats.track_types_flag = True

    return sim


def main():
    print("=" * 60)
    print("Opticks Cerenkov Test with Detectors")
    print("=" * 60)

    sim = create_simulation()

    print("\nSimulation configuration:")
    print(f"  Physics list: {sim.physics_manager.physics_list_name}")
    print(f"  Optical physics: {sim.physics_manager.special_physics_constructors.G4OpticalPhysics}")
    print(f"  Source: 5 electrons at 10 MeV")
    print(f"  Target: Water box (30x30x30 cm)")
    print(f"  Detectors: 4 detector planes surrounding water box")
    print()

    print("Starting simulation...")
    try:
        sim.run()
        print("\nSimulation completed!")

        # Get results
        opticks = sim.get_actor("opticks")
        print(f"\nResults:")
        print(f"  Total photons: {opticks.GetTotalNumPhotons()}")
        print(f"  Total hits: {opticks.GetTotalNumHits()}")
        print(f"  Total gensteps: {opticks.GetTotalNumGensteps()}")

    except Exception as e:
        print(f"\nSimulation error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
