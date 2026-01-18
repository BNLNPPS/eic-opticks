# Performance Studies

To quantify the speed-up achieved by EIC-Opticks compared to Geant4, we provide
Python code that runs the same G4 simulation with and without tracking optical
photons in G4.

The difference between runs yields the time required to simulate photons in Geant4.
Meanwhile, the same photons are simulated on GPU with EIC-Opticks and the
simulation time is saved for comparison.

## Running Performance Tests

```shell
mkdir -p /tmp/out/dev
mkdir -p /tmp/out/rel

docker build -t eic-opticks:perf-dev --target=develop .
docker run --rm -t -v /tmp/out:/tmp/out eic-opticks:perf-dev run-performance -o /tmp/out/dev

docker build -t eic-opticks:perf-rel --target=release .
docker run --rm -t -v /tmp/out:/tmp/out eic-opticks:perf-rel run-performance -o /tmp/out/rel
```
