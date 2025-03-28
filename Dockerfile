# syntax=docker/dockerfile:latest

FROM nvcr.io/nvidia/cuda:12.5.0-runtime-ubuntu22.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt update \
 && apt install -y g++ gcc gzip tar python3 python-is-python3 python3-pip \
 && apt clean \
 && rm -rf /var/lib/apt/lists/*

RUN apt update \
 && apt install -y curl cuda-nvcc-12-5 libcurand-dev-12-5 \
 && apt clean \
 && rm -rf /var/lib/apt/lists/*

RUN apt update \
 && apt install -y libssl-dev \
    nlohmann-json3-dev \
    libglew-dev libglfw3-dev libglm-dev libglu1-mesa-dev libxmu-dev \
    cmake qtbase5-dev libxerces-c-dev libexpat1-dev \
 && apt clean \
 && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /opt/geant4/src && curl -sL https://github.com/Geant4/geant4/archive/refs/tags/v11.1.2.tar.gz | tar -xz --strip-components 1 -C /opt/geant4/src \
 && cmake -S /opt/geant4/src -B /opt/geant4/build -DGEANT4_USE_OPENGL_X11=ON -DGEANT4_USE_QT=ON -DGEANT4_USE_GDML=ON -DGEANT4_INSTALL_DATA=ON -DGEANT4_BUILD_MULTITHREADED=ON \
 && cmake --build /opt/geant4/build --parallel --target install \
 && rm -fr /opt/geant4

RUN mkdir -p /opt/clhep/src && curl -sL https://gitlab.cern.ch/CLHEP/CLHEP/-/archive/CLHEP_2_4_7_1/CLHEP-CLHEP_2_4_7_1.tar.gz | tar -xz --strip-components 1 -C /opt/clhep/src \
 && cmake -S /opt/clhep/src -B /opt/clhep/build \
 && cmake --build /opt/clhep/build --parallel --target install \
 && rm -fr /opt/clhep

RUN mkdir -p /opt/glew/src && curl -sL https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.tgz | tar -xz --strip-components 1 -C /opt/glew/src \
 && cmake -S /opt/glew/src/build/cmake -B /opt/glew/build \
 && cmake --build /opt/glew/build --parallel --target install \
 && rm -fr /opt/glew

RUN mkdir -p /opt/plog/src && curl -sL https://github.com/SergiusTheBest/plog/archive/refs/tags/1.1.10.tar.gz | tar -xz --strip-components 1 -C /opt/plog/src \
 && cmake -S /opt/plog/src -B /opt/plog/build \
 && cmake --build /opt/plog/build --parallel --target install \
 && rm -fr /opt/plog

RUN mkdir -p /opt/optix && curl -sL https://github.com/NVIDIA/optix-dev/archive/refs/tags/v8.1.0.tar.gz | tar -xz --strip-components 1 -C /opt/optix

RUN mkdir -p /opt/cmake/src && curl -sL https://github.com/Kitware/CMake/releases/download/v3.30.7/cmake-3.30.7.tar.gz | tar -xz --strip-components 1 -C /opt/cmake/src \
 && cmake -S /opt/cmake/src -B /opt/cmake/build \
 && cmake --build /opt/cmake/build --parallel --target install \
 && rm -fr /opt/cmake

ENV OPTICKS_PREFIX=/opt/eic-opticks
ENV OPTICKS_HOME=/src/eic-opticks
ENV OPTICKS_BUILD=/opt/eic-opticks/build
ENV LD_LIBRARY_PATH=${OPTICKS_PREFIX}/lib:${LD_LIBRARY_PATH}
ENV PATH=${OPTICKS_PREFIX}/bin:${PATH}
ENV NVIDIA_DRIVER_CAPABILITIES=graphics,compute,utility

SHELL ["/bin/bash", "-l", "-c"]

# Set up non-interactive shells by sourcing all of the scripts in /etc/profile.d/
COPY spack/bash.nonint /etc/bash.nonint

RUN cat /etc/bash.nonint >> /etc/bash.bashrc

ENV BASH_ENV=/etc/bash.nonint

COPY . $OPTICKS_HOME 

RUN python -m pip install -e $OPTICKS_HOME

RUN cmake -S $OPTICKS_HOME -B $OPTICKS_BUILD -DCMAKE_INSTALL_PREFIX=$OPTICKS_PREFIX -DOptiX_INSTALL_DIR=/opt/optix -DCMAKE_BUILD_TYPE=Release \
 && cmake --build $OPTICKS_BUILD --parallel --target install
