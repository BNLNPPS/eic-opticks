"""
GPU optical photon propagation using Opticks/OptiX.

Collects Cerenkov and Scintillation gensteps during simulation,
then propagates optical photons on GPU either:
- When batch_size gensteps are collected, OR
- At end of run (for remaining gensteps)

Results are saved to disk as .npy files per batch.
"""

import threading

import opengate_core as g4
from .base import ActorBase
from ..base import process_cls


class OpticksActor(ActorBase, g4.GateOpticksActor):
    """
    GPU optical photon propagation using Opticks/OptiX.

    Collects Cerenkov and Scintillation gensteps during simulation,
    then propagates optical photons on GPU either:
    - When batch_size gensteps are collected, OR
    - At end of run (for remaining gensteps)

    Results are saved to disk as .npy files per batch.
    """

    user_info_defaults = {
        "batch_size": (
            1e5,
            {
                "doc": "Number of gensteps to collect before triggering GPU simulation. "
                       "Set to 0 to only process at end of run.",
            },
        ),
        "output_path": (
            "opticks_output",
            {"doc": "Path to save GPU simulation results"},
        ),
        "save_to_file": (
            True,
            {"doc": "Whether to save results to disk"},
        ),
        "verbose_batch": (
            False,
            {"doc": "Print info after each batch is processed"},
        ),
        # START Only needed for CPU vs GPU validation
        "cpu_mode": (
            False,
            {"doc": "If True, track optical photons on CPU and count hits at detector boundaries. "
                    "Use this for validation against GPU results."},
        ),
        "detector_volume_name": (
            "detector",
            {"doc": "Volume name pattern for detector (used for CPU hit counting)"},
        ),
        # END Only needed for CPU vs GPU validation
    }

    def __init__(self, *args, **kwargs):
        ActorBase.__init__(self, *args, **kwargs)
        # Thread lock for multi-threaded access to Python callback
        self.lock = None
        # Cumulative tracking (Python-side mirror)
        self.total_hits = 0
        self.total_photons = 0
        self.total_gensteps = 0
        self.batches_processed = 0
        self.__initcpp__()

    def __initcpp__(self):
        g4.GateOpticksActor.__init__(self, self.user_info)
        self.AddActions({
            "SteppingAction",
            "BeginOfRunAction",
            "EndOfRunAction",
            "BeginOfRunActionMasterThread",
            "EndOfRunActionMasterThread",
        })

    def __getstate__(self):
        # Needed to not pickle objects that cannot be pickled (lock, etc.)
        return_dict = super().__getstate__()
        return_dict["lock"] = None
        return return_dict

    def initialize(self):
        ActorBase.initialize(self)
        self.lock = threading.Lock()

        # Reset counters
        self.total_hits = 0
        self.total_photons = 0
        self.total_gensteps = 0
        self.batches_processed = 0

        # Initialize C++ side
        self.InitializeUserInfo(self.user_info)
        self.InitializeCpp()
        self.SetProcessBatchFunction(self.process_batch)

    def process_batch(self, cpp_actor):
        """
        Called from C++ after each GPU batch simulation completes.
        Thread-safe via lock for multi-threaded simulations.
        """
        if self.simulation.use_multithread:
            with self.lock:
                self._process_batch_impl(cpp_actor)
        else:
            self._process_batch_impl(cpp_actor)

    def _process_batch_impl(self, cpp_actor):
        """Implementation of batch processing."""
        # Get current batch results
        batch_hits = cpp_actor.GetCurrentBatchNumHits()
        batch_photons = cpp_actor.GetCurrentBatchNumPhotons()
        batch_num = cpp_actor.GetNumBatchesProcessed()

        # Update Python-side totals
        self.total_hits += batch_hits
        self.total_photons += batch_photons
        self.batches_processed = batch_num

        if self.verbose_batch:
            print(f"Opticks batch {batch_num} complete:")
            print(f"  Batch photons: {batch_photons}, hits: {batch_hits}")
            print(f"  Total photons: {self.total_photons}, hits: {self.total_hits}")

    def EndOfRunActionMasterThread(self, run_id):
        """Called at end of run from master thread."""
        # Final stats are available via C++ getters
        return 0

    def EndSimulationAction(self):
        """Called at end of simulation."""
        g4.GateOpticksActor.EndSimulationAction(self)
        ActorBase.EndSimulationAction(self)

        # Print final summary
        # START Only needed for CPU vs GPU validation
        if self.IsCpuMode():
            print(f"\nOpticks CPU Mode Summary:")
            print(f"  Total hits detected: {self.GetTotalNumCpuHits()}")
        else:
        # END Only needed for CPU vs GPU validation
            print(f"\nOpticks GPU Simulation Summary:")
            print(f"  Total batches processed: {self.batches_processed}")
            print(f"  Total gensteps: {self.GetTotalNumGensteps()}")
            print(f"  Total photons generated: {self.total_photons}")
            print(f"  Total hits detected: {self.total_hits}")


process_cls(OpticksActor)
