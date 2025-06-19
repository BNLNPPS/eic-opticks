#pragma once

#include <vector>

#include "sysrap/sphoton.h"
#include "sysrap/srng.h"
#include "sysrap/storch.h"


constexpr storch default_torch{
    .gentype = OpticksGenstep_TORCH,
    .trackid = 0,
    .matline = 0,
    .numphoton = 100,
    
    // Assign default values for position, time, momentum, and other attributes
    .pos = {-10.0f, -30.0f, -90.0f},
    .time = 0.0f,
    
    .mom = float3{0.0f, 0.3f, 1.0f},
    .weight = 0.0f,
    
    .pol = {1.0f, 0.0f, 0.0f},
    .wavelength = 420.0f,
    
    .zenith = {0.0f, 1.0f},
    .azimuth = {0.0f, 1.0f},
    
    .radius = 15.0f,
    .distance = 0.0f,
    .mode = 255,
    .type = T_DISC,
};


std::vector<sphoton> generate_photons(const storch& torch = default_torch, int num_photons = 100, unsigned int seed = 0);
