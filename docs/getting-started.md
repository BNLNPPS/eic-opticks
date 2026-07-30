# Getting started

This guide walks through the main ways to set up, build, test, and run Simphony.
The Dev Container is the easiest place to start because it brings the complete
toolchain with it. If you already manage the dependencies yourself, you can
skip ahead to [building directly on the host](#build-directly-on-the-host). The
later sections cover published Docker and Apptainer/Singularity images and a
test job on NERSC Perlmutter.

## Develop and test with the Dev Container

The repository's [Dev Container
configuration](../.devcontainer/devcontainer.json) gives you a consistent
development environment without installing CUDA, OptiX, or Geant4 directly on
the host.

### Install the host tools

Before you begin, install [Docker
Engine](https://docs.docker.com/engine/install/) if it is not available. Then install [Dev Container CLI](https://github.com/devcontainers/cli) 0.82 or newer with npm:

```shell
npm install -g @devcontainers/cli
```

If Node.js and npm are not installed, use the [standalone
installer](https://github.com/devcontainers/cli#install-script). You only need
a CUDA-capable NVIDIA GPU and the [NVIDIA Container
Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)
when you are ready to run GPU-backed code. The Dev Container CLI detects the
NVIDIA runtime and requests GPU access automatically.

### Get the source

Clone the repository and enter the checkout:

```shell
git clone https://github.com/BNLNPPS/simphony.git
cd simphony
```

If needed, switch to an existing branch with `git switch existing-branch`, or
create one with `git switch -c new-branch`.

### Choose dependency versions

Skip this section if the default dependency versions meet your needs. The root
`.env` file selects the OS and toolchain used to build the Dev Container.
`.devcontainer/.env` links to it so Compose can discover the same settings.
Edit `.env` when you want to use another supported combination or experiment
with a new dependency version.

The defaults match the `base` alias in the [published container
matrix](../README.md#published-container-images). Start with another `base`
combination from this matrix when possible.

After editing `.env`, recreate the container so it uses the new values:

```shell
devcontainer up --remove-existing-container
```

When trying versions outside the matrix, first confirm that the corresponding
CUDA image and OptiX, Geant4, and CMake releases exist. Also check that the host
NVIDIA driver supports the selected CUDA version. The first build may take
longer because a matching published build cache may not be available.

### Start the environment

From the repository root, start the environment and open a shell:

```shell
devcontainer up
devcontainer exec bash
```

The first `devcontainer up` builds the environment. Later runs reuse the image,
so getting back to work is much faster.

Also recreate the container after changing the `Dockerfile` or `.devcontainer`
configuration. This is useful when you want to discard container-local state
and start fresh:

```shell
devcontainer up --remove-existing-container
```

You do not need to recreate the container after switching source branches. The
checkout remains mounted directly into the environment.

The default Compose project name keeps different system users from colliding,
even when they share a Docker daemon. If one account uses multiple checkouts or
runs concurrent jobs, set a unique project name before starting the container:

```shell
export COMPOSE_PROJECT_NAME="simphony-${USER}-my_cool_feature"
```

The same configuration also works with the VS Code Dev Containers extension.

### Build and test

Once the shell opens, configure, build, and run the full test suite:

```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

Your source and build files stay in the host checkout, so they remain available
after the container stops. For a quicker iteration loop, list the tests, build
only the target you are changing, and run a relevant test:

```shell
ctest --test-dir build -N
cmake --build build --target simg4ox
ctest --test-dir build -R raindrop
```

The full suite includes GPU-backed tests. You can build without a GPU, but
running those tests requires a compatible NVIDIA driver and GPU access from the
container.

## Build directly on the host

If you prefer to manage the toolchain yourself, install:

- CUDA 12.1+
- NVIDIA OptiX 7+
- Geant4 11.3+
- CMake 3.22+
- Python 3.10+

With those dependencies available, clone, build, and test the project:

```shell
git clone https://github.com/BNLNPPS/simphony.git
cd simphony
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

Before running GPU workloads, make sure the installed driver supports your
OptiX version. The minimum driver versions are:

| OptiX version | Release date  | Minimum driver required |
|---            |---:           |---                      |
| 9.1.0         | December 2025 | 590                     |
| 9.0.0         | February 2025 | 570                     |
| 8.1.0         | October 2024  | 555                     |
| 8.0.0         | August 2023   | 535                     |
| 7.7.0         | March 2023    | 530.41                  |
| 7.6.0         | October 2022  | 522.25                  |
| 7.5.0         | June 2022     | 515.48                  |
| 7.4.0         | November 2021 | 495.89                  |
| 7.3.0         | April 2021    | 465.84                  |
| 7.2.0         | October 2020  | 455.28                  |
| 7.1.0         | June 2020     | 450                     |
| 7.0.0         | August 2019   | 435.80                  |

See NVIDIA's [OptiX download
page](https://developer.nvidia.com/designworks/optix/downloads/legacy) for the
release details.

## Run with Docker

To try the latest published release and confirm that GPU access works:

```shell
docker run --rm --gpus all ghcr.io/bnlnpps/simphony simg4ox -g tests/geom/raindrop.gdml -m tests/run.mac
```

To test an image built from your current checkout instead:

```shell
docker build -t simphony:develop .
docker run --rm --gpus all simphony:develop simg4ox -g tests/geom/raindrop.gdml -m tests/run.mac
```

For day-to-day development, the Dev Container is more convenient because source
edits are available immediately without rebuilding the image.

## Run with Apptainer or Singularity

On systems that provide Apptainer, run the same published release with:

```shell
apptainer exec --nv docker://ghcr.io/bnlnpps/simphony simg4ox -g /workspaces/simphony/tests/geom/raindrop.gdml -m /workspaces/simphony/tests/run.mac
```

Use `singularity` in place of `apptainer` on systems that provide the older
command name.

## Run a test job at NERSC

To try Simphony on Perlmutter, start with the repository's [batch-job
example](../scripts/submit.sh). Review the email address, allocation, image, and
command, then submit the job:

```shell
sbatch scripts/submit.sh
```
