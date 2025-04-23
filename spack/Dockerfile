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
# will build this below
RUN spack env create eodev

RUN spack install --only dependencies eic-opticks ^ optix-dev@8.1.0
#RUN spack install eic-opticks

## Create and populate the spack environment with dependencies.

RUN echo "spack env activate eodev -p" > /etc/profile.d/z10_spack_env_activate_eodev.sh

RUN bash -c 'test -n "$SPACK_ENV"'

# See README.md notes
# RUN spack install --add eic-opticks ^optix-dev@8.1.0 python py-pip cmake openssl
RUN spack install --add \
    geant4 \
    glew \
    glfw \
    glm \
    glu \
    nlohmann-json \
    mesa~llvm \
    optix-dev@8.1.0 \
    openssl \
    plog \
    python \
    py-pip \
    cmake \
    openssl


# Where eic-opticks will be installed
ENV OPTICKS_PREFIX=/opt/eic-opticks
# Where eic-opticks source is found
ENV OPTICKS_HOME=/src/eic-opticks
# Where eic-opticks build resides.  Note, for now, this must be under the prefix
# due to the test script hard-wiring this location.  This variable is not
# canonical and only invented for here!
ENV OPTICKS_BUILD=/opt/eic-opticks/build

RUN git clone https://github.com/BNLNPPS/eic-opticks.git $OPTICKS_HOME
RUN which python
RUN cd $OPTICKS_HOME && python -m pip install -e .

# See README.md notes for the next few chunks

## Build time

RUN cd $OPTICKS_HOME \
    && \
    cmake -S . -B $OPTICKS_BUILD \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$OPTICKS_PREFIX \
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-L$SPACK_ENV/.spack-env/view/lib64 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-L$SPACK_ENV/.spack-env/view/lib64


## Run time

COPY spack/bash.runtime /etc/profile.d/z11_eic_optics_runtime.sh

RUN cd $OPTICKS_HOME \
    && \
    cmake --build $OPTICKS_BUILD --parallel --target install

# RUN $OPTICKS_HOME/tests/test_opticks.sh

# LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/install/lib

# ./tests/test_opticks.sh  # no rpath to deps, fails to find libs

# LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/spack/var/spack/environments/eodev/.spack-env/view/lib

# ./tests/test_opticks.sh  # wrong libssl

# LD_LIBRARY_PATH=/install/lib:/opt/spack/var/spack/environments/eodev/.spack-env/view/lib:/opt/spack/var/spack/environments/eodev/.spack-env/view/lib64:/usr/local/nvidia/lib:/usr/local/nvidia/lib64

# ./tests/test_opticks.sh  # optix ABI wrongness

ENTRYPOINT ["bash","--login"]
