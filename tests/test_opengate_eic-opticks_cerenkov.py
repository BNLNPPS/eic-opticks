#!/usr/bin/env python3
"""
Test oepnGate integration: OpticksActor with Cerenkov photon generation in water.

This test creates a simple water box geometry with optical properties
and shoots electrons through it to generate Cerenkov photons.
The eic-Opticks actor intercepts the gensteps and simulates them on GPU.
"""

import opengate as gate
from pathlib import Path

# Units
m = gate.g4_units.m
cm = gate.g4_units.cm
mm = gate.g4_units.mm
MeV = gate.g4_units.MeV
keV = gate.g4_units.keV
eV = gate.g4_units.eV

def create_simulation():
    """Create simulation with optical properties for Cerenkov generation."""

    sim = gate.Simulation()
    sim.visu = False
    sim.number_of_threads = 1
    sim.random_seed = 12345

    # World
    world = sim.world
    world.size = [1*m, 1*m, 1*m]
    world.material = "G4_AIR"

    # Water box - where Cerenkov photons will be generated
    waterbox = sim.add_volume("Box", "waterbox")
    waterbox.size = [30*cm, 30*cm, 30*cm]
    waterbox.material = "G4_WATER"
    waterbox.translation = [0, 0, 0]

    # Physics list - need EM standard physics option4 for optical
    # See: https://opengate.readthedocs.io/en/latest/generating_and_tracking_optical_photons.html
    sim.physics_manager.physics_list_name = "G4EmStandardPhysics_option4"

    # Enable optical physics (Cerenkov, Scintillation, etc.)
    sim.physics_manager.special_physics_constructors.G4OpticalPhysics = True

    # Energy range for optical photons (visible light: ~1.5 - 4 eV)
    sim.physics_manager.energy_range_min = 1 * eV
    sim.physics_manager.energy_range_max = 10 * MeV

    # Production cut for electrons (needed for Cerenkov)
    sim.physics_manager.set_production_cut("waterbox", "electron", 0.1 * mm)

    # Set custom optical properties file with Water definition
    optical_props_file = Path(__file__).parent / "geom" / "WaterOpticalProperties.xml"
    sim.physics_manager.optical_properties_file = str(optical_props_file)

    # Electron source - will generate Cerenkov photons in water
    # Cerenkov threshold in water: ~0.26 MeV for electrons (v > c/n)
    source = sim.add_source("GenericSource", "electron_beam")
    source.particle = "e-"
    source.energy.mono = 10 * MeV  # Well above Cerenkov threshold
    source.position.type = "point"
    source.position.translation = [0, 0, -20*cm]  # Start outside water
    source.direction.type = "momentum"
    source.direction.momentum = [0, 0, 1]  # Shoot into water
    source.n = 5  # Just 5 electrons for quick test

    # Add OpticksActor - attach to world to see all steps
    opticks_actor = sim.add_actor("OpticksActor", "opticks")
    opticks_actor.attached_to = "world"
    opticks_actor.batch_size = 0  # Process all at end of run
    opticks_actor.output_path = "/tmp/opticks_test"
    opticks_actor.save_to_file = False
    opticks_actor.verbose_batch = True

    # Add statistics actor
    stats = sim.add_actor("SimulationStatisticsActor", "stats")
    stats.track_types_flag = True

    return sim


def main():
    print("="*60)
    print("eic-Opticks Cerenkov Test with openGATE framework")
    print("="*60)

    sim = create_simulation()

    print("\nSimulation configuration:")
    print(f"  Physics list: {sim.physics_manager.physics_list_name}")
    print(f"  Optical physics: {sim.physics_manager.special_physics_constructors.G4OpticalPhysics}")
    print(f"  Source: 5 electrons at 10 MeV")
    print(f"  Target: Water box (30x30x30 cm)")
    print()

    print("Starting simulation...")
    try:
        sim.run()
        print("\nSimulation completed!")
    except Exception as e:
        print(f"\nSimulation error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
