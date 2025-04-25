# syntax=docker/dockerfile:latest

FROM nvcr.io/nvidia/cuda:12.5.0-runtime-ubuntu22.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt update \
 && apt install -y bzip2 ca-certificates g++ gcc gfortran git gzip lsb-release patch python3 tar unzip xz-utils zstd \
 && apt clean \
 && rm -rf /var/lib/apt/lists/*

RUN apt update \
 && apt install -y curl cuda-nvcc-12-5 libcurand-dev-12-5 python-is-python3 \
 && apt clean \
 && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /opt/spack && curl -sL https://github.com/spack/spack/archive/v0.23.0.tar.gz | tar -xz --strip-components 1 -C /opt/spack

RUN echo "source /opt/spack/share/spack/setup-env.sh" > /etc/profile.d/z09_source_spack_setup.sh

SHELL ["/bin/bash", "-l", "-c"]

COPY  spack/bash.nonint /etc/bash.nonint

RUN cat /etc/bash.nonint >> /etc/bash.bashrc

ENV BASH_ENV=/etc/bash.nonint

### Avoid copying non-spack files
# COPY spack esi-spack
RUN mkdir esi-spack
COPY spack/packages esi-spack/packages
COPY spack/repo.yaml esi-spack/repo.yaml

RUN spack repo add esi-spack

# Install eic-opticks and dependencies (outside of any spack env).
RUN spack install eic-opticks ^optix-dev@8.1.0

RUN spack env create eodev
RUN echo "spack env activate eodev" > /etc/profile.d/z10_spack_env_activate_eodev.sh

# There is apparently no way to create a Spack Environment based on a seed (aka
# "root") package but to not actually have the root package.  Creating the Spack
# Environment and then uninstalling the root, also uninstalls (some)
# dependencies.
RUN spack install --add eic-opticks ^optix-dev@8.1.0

# These are required for development build and test.
RUN spack install --add python py-pip cmake openssl

# At this point we'd have an end-user, ready-to-run container.  The rest sets up
# a developer environment, builds eic-opticks outside of Spack, but based on
# dependencies from the Spack Environment.

# Where eic-opticks will be installed
ENV OPTICKS_PREFIX=/opt/eic-opticks
# Where eic-opticks source is found
ENV OPTICKS_HOME=/src/eic-opticks
# Where eic-opticks build resides.  Note, for now, this must be under the prefix
# due to the test script hard-wiring this location.  This variable is not
# canonical and only invented for here!
ENV OPTICKS_BUILD=/opt/eic-opticks/build

ENV NVIDIA_DRIVER_CAPABILITIES=graphics,compute,utility

RUN git clone https://github.com/BNLNPPS/eic-opticks.git $OPTICKS_HOME
RUN which python
RUN cd $OPTICKS_HOME && python -m pip install -e .

## Development build configuration
RUN cd $OPTICKS_HOME \
    && \
    cmake -S . -B $OPTICKS_BUILD \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$OPTICKS_PREFIX \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-L$SPACK_ENV/.spack-env/view/lib64 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-L$SPACK_ENV/.spack-env/view/lib64


## Set up runtime environment
COPY spack/bash.runtime /etc/profile.d/z11_eic_optics_runtime.sh

## Do build
RUN cd $OPTICKS_HOME \
    && \
    cmake --build $OPTICKS_BUILD --parallel --target install


## Ready for use
ENTRYPOINT ["bash","--login"]

