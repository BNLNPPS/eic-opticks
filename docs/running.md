# Running Simulations

## Running at NERSC (Perlmutter)

To submit a test run of `eic-opticks` on Perlmutter, use the following example.
Make sure to update any placeholder values as needed.

```shell
sbatch scripts/submit.sh
```

Example SLURM script:

```bash
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

## User-Defined Inputs

### Defining Primary Particles

The user/developer has to define certain inputs. In the `src/simg4oxmt` example
that imports `src/g4appmt.h` we provide a working example with a simple geometry.

Configure the following in a macro file:

* **Number of primary particles** to simulate
* **Number of G4 threads**

Example macro:

```
/run/numberOfThreads {threads}
/run/verbose 1
/process/optical/cerenkov/setStackPhotons {flag}
/run/initialize
/run/beamOn 500
```

The `setStackPhotons` flag defines whether G4 will propagate optical photons or
not. In production, Opticks (GPU) handles the optical photon propagation.

Additionally, define the **starting position**, **momentum**, etc. of the primary
particles in the **GeneratePrimaries** function in `src/g4appmt.h`.

The hits of the optical photons are returned in the **EndOfRunAction** function.
If more photons are simulated than can fit in the GPU RAM, move the GPU call
execution to **EndOfEventAction** together with retrieving the hits.
