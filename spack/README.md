# A spack-based container for eic-opticks

This directory holds files to build a Spack-based container for eic-opticks.  It
uses the eic-opticks Spack packaging in this directory to build the tip of the
"main" branch.  A Spack Environment is constructed with this Spack-built
eic-opticks package as the seed.  Finally, a git clone of the eic-opticks source
is produced and built based on the dependencies provided by the Spack Environment.

The container may be used to simply run eic-opticks, as part of a release
testing or to provided a basis for developing eic-opticks source.

# Build the image

A working Docker installation is a prerequisite.
From the top level directory of this eic-opticks repo run:

```
docker build -t eic-opticks-spack -f spack/Dockerfile .
```

# Run tests

```
docker run --rm --gpus=all eic-opticks-spack /src/eic-opticks/tests/test_opticks.sh
```

# Run interactively

```
docker run -ti --gpus=all eic-opticks-spack
```

Add `--rm` to not save any interactive changes.

# Run time issues

These global Bash init scripts are installed:

```
/etc/profile.d/z09_source_spack_setup.sh
/etc/profile.d/z10_spack_env_activate_eodev.sh
/etc/profile.d/z11_eic_optics_runtime.sh
```

All shells started will have the Spack Environment activated.  It may be
deactivated by running `despacktivate`.

# File system

Some directories of note are:

- `/src/eic-opticks` the git clone made at image build time
- `/opt/eic-opticks` the installed development build
- `$SPACK_ENV/.spack-env/view/` the installation prefix for dependencies

# Docker vs Podman

Podman man be used to build and run the image using very similar commands as
given above for Docker.  However there is some problem with Podman as of at
least version 4.3.1 that leads to a segfault in a test.  This is traced to an
"OptiX unknown error" (some info in GitHub Issue #79).  Until this is understood
and resolved, use of Podman is not recommended.

# Other images

The (non-Spack) container image build by used in GH CI workflows may also be
built locally.  Again, from the top level directory run:

```
docker build -t eic-opticks-nospack -f spack/Dockerfile .
```

