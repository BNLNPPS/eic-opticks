# Build Spack-based container

This directory holds files to build a Spack-based container with eic-spack
dependencies and eic-spack itself built.  A Spack "environment" defined and
eic-spack dependencies are added.  It is then used to build a development
version of eic-spack.

## Docker

TBD, but should be very similar to the `podman` instructions below.

## Podman

From the top level directory of this eic-opticks repo run:

```
podman build --format docker -t eic-opticks-spack -f spack/Dockerfile .
podman run --rm -ti --device nvidia.com/gpu=all eic-opticks-spack
```

For teaching `podman` about your GPUs, see https://podman-desktop.io/docs/podman/gpu

Note, the non-Spack container may be built similarly:

```
podman build --format docker -t eic-opticks-nospack -f Dockerfile .
```


# Notes

This section holds detailed notes on some weirdnesses in either Spack or
eic-opticks that needed special workaround in building the container image.

## Spack environment

It seems that (unlike views) one can not "exclude" a package that provides the
seed to build an environment.  Thus we make the environment with an exhaustive
list of dependencies as culled from the `package.py` for `eic-opticks`.  If/when
a Spack environment can be constructed like a Spack view, we should fix this to
avoid having to synchronize any changes that may be needed in `package.py`.

## Build time

Although `cmake` config step finds:

```
-- Found OpenSSL: /opt/spack/var/spack/environments/eodev/.spack-env/view/lib64/libcrypto.so (found version "3.4.0")
-- adding ssl crypto for UNIX AND NOT APPLE
```

It does not apparently link to `libssl` properly

```
/usr/bin/ld: cannot find -lssl: No such file or directory
collect2: error: ld returned 1 exit status
```

Spack installs `openssl` libs to the unusual location (for Deb/Ubu) of `lib64/` we tell `cmake` this with additional
```
    -DCMAKE_SHARED_LINKER_FLAGS_INIT=-L$SPACK_ENV/.spack-env/view/lib64 \
    -DCMAKE_EXE_LINKER_FLAGS_INIT=-L$SPACK_ENV/.spack-env/view/lib64
```

## Run time

Spack heavily uses `rpath` so executables and shared libraries can find shared
libraries in the Spack install area.  Apparently, eic-opticks `cmake` build does
not.  This means that `LD_LIBRARY_PATH` must be set.

One detail: the base `nvidia/cuda` image sets `LD_LIBRARY_PATH` with some
non-existent locations so here we set it absolutely instead of appending.

The setting is in `/etc/profile.d/z11_eic_optics_runtime.sh`.

## Tests

In principle, one may run tests from the container like:

```
/src/eic-opticks/tests/test_opticks.sh 
```

However, for now there are some bugs that are not understood.
