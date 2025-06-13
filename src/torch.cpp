#include <curand_kernel.h>

#include "torch.h"

using namespace std;


vector<sphoton> generate_photons(const storch& torch, int num_photons, unsigned int seed)
{
    curandStatePhilox4_32_10 rng;
    curand_init(seed, 0, 0, &rng);

    vector<sphoton> photons;
    int unused = -1;
    qtorch qt{.t = torch};

    for (int i = 0; i < num_photons; i++)
    {
        sphoton photon;
        storch::generate(photon, rng, qt.q, unused, unused);
        photons.push_back(photon);
    }

    return photons;
}
