# syntax=docker/dockerfile:latest

ARG OS=ubuntu22.04
ARG CUDA_VERSION=12.4.0

FROM nvidia/cuda:${CUDA_VERSION}-devel-${OS} AS base

ARG OPTIX_VERSION=7.7.0
ARG GEANT4_VERSION=11.3.2
ARG CMAKE_VERSION=4.1.2

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update \
 && apt install -y g++ gcc gzip tar python3 python-is-python3 python3-pip curl \
 && apt clean \
 && rm -rf /var/lib/apt/lists/*

RUN apt update \
 && apt install -y libssl-dev \
    nlohmann-json3-dev \
    libglfw3-dev libglu1-mesa-dev libxmu-dev libglew-dev libglm-dev \
    cmake qtbase5-dev libxerces-c-dev libexpat1-dev \
 && apt clean \
 && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /opt/cmake/src && curl -sL https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz | tar -xz --strip-components 1 -C /opt/cmake/src \
 && cmake -S /opt/cmake/src -B /opt/cmake/build \
 && cmake --build /opt/cmake/build --parallel --target install \
 && rm -fr /opt/cmake

RUN mkdir -p /opt/geant4/src && curl -sL https://github.com/Geant4/geant4/archive/refs/tags/v${GEANT4_VERSION}.tar.gz | tar -xz --strip-components 1 -C /opt/geant4/src \
 && cmake -S /opt/geant4/src -B /opt/geant4/build -DGEANT4_USE_OPENGL_X11=ON -DGEANT4_USE_QT=ON -DGEANT4_USE_GDML=ON -DGEANT4_INSTALL_DATA=ON -DGEANT4_BUILD_MULTITHREADED=ON \
 && cmake --build /opt/geant4/build --parallel --target install \
 && rm -fr /opt/geant4

RUN mkdir -p /opt/clhep/src && curl -sL https://gitlab.cern.ch/CLHEP/CLHEP/-/archive/CLHEP_2_4_7_1/CLHEP-CLHEP_2_4_7_1.tar.gz | tar -xz --strip-components 1 -C /opt/clhep/src \
 && cmake -S /opt/clhep/src -B /opt/clhep/build \
 && cmake --build /opt/clhep/build --parallel --target install \
 && rm -fr /opt/clhep

RUN mkdir -p /opt/plog/src && curl -sL https://github.com/SergiusTheBest/plog/archive/refs/tags/1.1.11.tar.gz | tar -xz --strip-components 1 -C /opt/plog/src \
 && cmake -S /opt/plog/src -B /opt/plog/build \
 && cmake --build /opt/plog/build --parallel --target install \
 && rm -fr /opt/plog

RUN mkdir -p /opt/optix && curl -sL https://github.com/NVIDIA/optix-dev/archive/refs/tags/v${OPTIX_VERSION}.tar.gz | tar -xz --strip-components 1 -C /opt/optix

RUN curl -LsSf https://astral.sh/uv/install.sh | sh

SHELL ["/bin/bash", "-l", "-c"]

# Set up non-interactive shells by sourcing all of the scripts in /etc/profile.d/
RUN cat <<"EOF" > /etc/bash.nonint
if [ -d /etc/profile.d ]; then
  for i in /etc/profile.d/*.sh; do
    if [ -r $i ]; then
      . $i
    fi
  done
  unset i
fi
EOF

RUN cat /etc/bash.nonint >> /etc/bash.bashrc

ENV BASH_ENV=/etc/bash.nonint
ENV OPTICKS_PREFIX=/opt/eic-opticks
ENV OPTICKS_HOME=/src/eic-opticks
ENV OPTICKS_BUILD=/opt/eic-opticks/build
ENV LD_LIBRARY_PATH=${OPTICKS_PREFIX}/lib:${LD_LIBRARY_PATH}
ENV PATH=${OPTICKS_PREFIX}/bin:/root/.local/bin:$OPTICKS_HOME/.venv/bin:${PATH}
ENV NVIDIA_DRIVER_CAPABILITIES=graphics,compute,utility
ENV CSGOptiX__optixpath=${OPTICKS_PREFIX}/ptx/CSGOptiX_generated_CSGOptiX7.cu.ptx
ENV VIRTUAL_ENV=$OPTICKS_HOME/.venv

WORKDIR $OPTICKS_HOME

# Install Python dependencies
COPY pyproject.toml uv.lock $OPTICKS_HOME/
COPY optiphy $OPTICKS_HOME/optiphy
RUN uv sync


FROM base AS release

COPY . $OPTICKS_HOME

RUN cmake -S $OPTICKS_HOME -B $OPTICKS_BUILD -DCMAKE_INSTALL_PREFIX=$OPTICKS_PREFIX -DCMAKE_BUILD_TYPE=Release \
 && cmake --build $OPTICKS_BUILD --parallel --target install


FROM base AS develop

RUN apt update && apt install -y x11-apps mesa-utils vim

COPY . $OPTICKS_HOME

RUN cmake -S $OPTICKS_HOME -B $OPTICKS_BUILD -DCMAKE_INSTALL_PREFIX=$OPTICKS_PREFIX -DCMAKE_BUILD_TYPE=Debug \
 && cmake --build $OPTICKS_BUILD --parallel --target install
