# Docker

If you plan to develop or run the simulation in a containerized environment,
ensure that your system has the following tools installed:

* [Docker Engine](https://docs.docker.com/engine/install/)
* [NVIDIA container toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)

## Build the Image

Build latest `eic-opticks` image:

```shell
docker build -t ghcr.io/bnlnpps/eic-opticks:latest https://github.com/BNLNPPS/eic-opticks.git
```

Build for development:

```shell
docker build -t ghcr.io/bnlnpps/eic-opticks:develop --target=develop .
```

## Running Containers

Interactive session with X11 forwarding:

```shell
docker run --rm -it -v $HOME/.Xauthority:/root/.Xauthority -e DISPLAY=$DISPLAY --net=host ghcr.io/bnlnpps/eic-opticks:develop
```

Interactive session with home directory mounted:

```shell
docker run --rm -it -v $HOME:/esi -v $HOME/eic-opticks:/src/eic-opticks -e DISPLAY=$DISPLAY -e HOME=/esi --net=host ghcr.io/bnlnpps/eic-opticks:develop
```

Non-interactive test run:

```shell
docker run ghcr.io/bnlnpps/eic-opticks bash -c 'simg4ox -g tests/geom/sphere_leak.gdml -m tests/run.mac -c sphere_leak'
```

## Singularity

```shell
singularity run --nv -B eic-opticks-prefix/:/opt/eic-opticks -B eic-opticks:/src/eic-opticks docker://ghcr.io/bnlnpps/eic-opticks:develop
```
