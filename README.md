This simulation package interfaces NVIDIA OptiX with Geant4 to accelerate
optical photon transport for physics experiments. It supports detector
geometries defined in the GDML format and is based on the work by Simon Blyth,
whose original Opticks framework can be found
[here](https://simoncblyth.bitbucket.io/opticks/).


## Prerequisites

Before building or running this package, ensure that your system meets both the
hardware and software requirements listed below.

* A CUDA-capable NVIDIA GPU

* CUDA 12+
* NVIDIA OptiX 7+
* Geant4 11+
* CMake 3.18+
* Python 3.8+

Optionally, if you plan to develop or run the simulation in a containerized
environment, ensure that your system has the following tools installed:

* [Docker Engine](https://docs.docker.com/engine/install/)
* NVIDIA container toolkit ([installation guide](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html))

## Build

```shell
git clone https://github.com/BNLNPPS/eic-opticks.git
cmake -S eic-opticks -B build
cmake --build build
```

## Docker

Build latest `eic-opticks` image by hand:

```shell
docker build -t ghcr.io/bnlnpps/eic-opticks:latest https://github.com/BNLNPPS/eic-opticks.git
```

Build and run for development:

```shell
docker build -t ghcr.io/bnlnpps/eic-opticks:develop --target=develop .
```

Example commands for interactive and non-interactive tests:

```shell
docker run --rm -it -v $HOME/.Xauthority:/root/.Xauthority -e DISPLAY=$DISPLAY --net=host ghcr.io/bnlnpps/eic-opticks:develop

docker run --rm -it -v $HOME:/esi -v $HOME/eic-opticks:/src/eic-opticks -e DISPLAY=$DISPLAY -e HOME=/esi --net=host ghcr.io/bnlnpps/eic-opticks:develop

docker run ghcr.io/bnlnpps/eic-opticks bash -c 'simg4ox -g tests/geom/sphere_leak.gdml -m tests/run.mac -c sphere_leak'
```


## Singularity

```shell
singularity run --nv -B eic-opticks-prefix/:/opt/eic-opticks -B eic-opticks:/src/eic-opticks docker://ghcr.io/bnlnpps/eic-opticks:develop
```


## Running a test job at NERSC (Perlmutter)

To submit a test run of `eic-opticks` on Perlmutter, use the following example. Make sure to update
any placeholder values as needed.

```
sbatch scripts/submit.sh
```

```
#!/bin/bash

#SBATCH -N 1                    # number of nodes
#SBATCH -C gpu                  # constraint: use GPU partition
#SBATCH -G 1                    # request 1 GPU
#SBATCH -q regular              # queue
#SBATCH -J eic-opticks          # job name
#SBATCH --mail-user=<USER_EMAIL>
#SBATCH --mail-type=ALL
#SBATCH -A m4402                # allocation account
#SBATCH -t 00:05:00             # time limit (hh:mm:ss)

# Path to your image on Perlmutter
IMAGE="docker:bnlnpps/eic-opticks:develop"
CMD='cd /src/eic-opticks && simg4ox -g $OPTICKS_HOME/tests/geom/sphere_leak.gdml -m $OPTICKS_HOME/tests/run.mac -c sphere_leak'

# Launch the container using Shifter
srun -n 1 -c 8 --cpu_bind=cores -G 1 --gpu-bind=single:1 shifter --image=$IMAGE /bin/bash -l -c "$CMD"
```


## Optical Surface Models in Geant4

In Geant4, optical surface properties such as **finish**, **model**, and **type** are defined using enums in the
`G4OpticalSurface` and `G4SurfaceProperty` header files:

- [`G4OpticalSurface.hh`](https://github.com/Geant4/geant4/blob/geant4-11.3-release/source/materials/include/G4OpticalSurface.hh#L52-L113)
- [`G4SurfaceProperty.hh`](https://github.com/Geant4/geant4/blob/geant4-11.3-release/source/materials/include/G4SurfaceProperty.hh#L58-L68)

These enums allow users to configure how optical photons interact with surfaces, controlling behaviors like reflection,
transmission, and absorption based on physical models and surface qualities. The string values corresponding to these
enums (e.g. `"ground"`, `"glisur"`, `"dielectric_dielectric"`) can also be used directly in **GDML** files when defining
`<opticalsurface>` elements for geometry. Geant4 will parse these attributes and apply the corresponding surface
behavior.

For a physics-motivated explanation of how Geant4 handles optical photon boundary interactions, refer to the [Geant4
Application Developer Guide — Boundary
Process](https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/physicsProcess.html#boundary-process).

```gdml
<gdml>
  ...
  <solids>
    <opticalsurface finish="ground" model="glisur" name="medium_surface" type="dielectric_dielectric" value="1">
      <property name="EFFICIENCY" ref="EFFICIENCY_DEF"/>
      <property name="REFLECTIVITY" ref="REFLECTIVITY_DEF"/>
    </opticalsurface>
  </solids>
  ...
</gdml>
```
## User/developer defined inputs

### Defining primary particles

There are certain user defined inputs that the user/developer has to define. In the ```src/simg4oxmt``` example that imports ```src/g4appmt.h``` we provide a working example with a simple geometry. The User/developer has to change the following details:
**Number of primary particles** to simulate in a macro file and the **number of G4 threads**. For example:

```
/run/numberOfThreads {threads}
/run/verbose 1
/process/optical/cerenkov/setStackPhotons {flag}
/run/initialize
/run/beamOn 500
```

Here if the setStackPhotons defines **whether G4 will propagate optical photons or not**. In production Opticks (GPU) takes care of the optical photon propagation. Additionally the user has to define the **starting position**, **momentum** etc of the primary particles define in the **GeneratePrimaries** function in ``src/g4appmt.h```. The hits of the optical photons are returned in the **EndOfRunAction** function. If more photons are simulated than can fit in the GPU RAM the execution of a GPU call should be moved to **EndOfEventAction** together with retriving the hits.

### Loading in geometry into EIC-Opticks

## Performance studies

```
mkdir -p /tmp/out/dev
mkdir -p /tmp/out/rel

docker build -t eic-opticks:perf-dev --target=develop
docker run --rm -t -v /tmp/out:/tmp/out eic-opticks:perf-dev run-performance -o /tmp/out/dev

docker build -t eic-opticks:perf-rel --target=release
docker run --rm -t -v /tmp/out:/tmp/out eic-opticks:perf-rel run-performance -o /tmp/out/rel
```
