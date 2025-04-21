# Copyright Spack Project Developers. See COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack.package import *


class EicOpticks(CMakePackage, CudaPackage):
    """EIC Opticks package for GPU simulation"""

    homepage = "https://github.com/bnlnpps/eic-opticks"
    url      = "https://github.com/bnlnpps/eic-opticks/archive/tags/1.0.0-rc1.tar.gz"
    git      = "https://github.com/bnlnpps/eic-opticks.git"

    maintainers("plexoos")

    version("main", branch="main")
    version("1.0.0-rc1", sha256="fd5a7c7848a3c1211a203d7223f4eff2da810fb7dd3d10d1ea0edff451f966c8")

    depends_on("cxx", type="build")
    depends_on("cmake@3.10:", type="build")
    depends_on("geant4")
    depends_on("glew")
    depends_on("glfw")
    depends_on("glm")
    depends_on("glu")
    depends_on("nlohmann-json")
    depends_on("mesa~llvm")
    depends_on("optix-dev@7:")
    depends_on("openssl")
    depends_on("plog")
