#pragma once

#include <math.h>

#include "OpticksGenstep.h"
#include "SRG.h"

#if defined(__CUDACC__) || defined(__CUDABE__)
#define CSGOPTIX7_HELPER_METHOD __device__ __forceinline__
#else
#define CSGOPTIX7_HELPER_METHOD inline
#endif

CSGOPTIX7_HELPER_METHOD bool CSGOptiX7_GenstepCarriesMaterialLine(unsigned gentype)
{
    return gentype == OpticksGenstep_CERENKOV ||
           gentype == OpticksGenstep_SCINTILLATION ||
           gentype == OpticksGenstep_G4Cerenkov_modified;
}

CSGOPTIX7_HELPER_METHOD float CSGOptiX7_ReportedIntersectionDistance(
    float    distance,
    unsigned raygenmode,
    float    direction_dot_normal)
{
    const bool defer_exit = raygenmode == SRG_SIMULATE && direction_dot_normal > 0.f;
    return defer_exit ? nextafterf(distance, INFINITY) : distance;
}

#undef CSGOPTIX7_HELPER_METHOD
