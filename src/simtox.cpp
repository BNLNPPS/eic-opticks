#include <iostream>
#include <string>
#include <vector>

#include "sysrap/NP.hh"
#include "sysrap/SEvent.hh"
#include "sysrap/sphoton.h"
#include "sysrap/srng.h"
#include "sysrap/storch.h"
#include "sysrap/storchtype.h"

#include "torch.h"

#include <curand_kernel.h>

using namespace std;

int main(int argc, char **argv)
{
    unsigned n_photons = 100;

    // Initialize one torch object
    storch torch;

    // Assign values to all data members based on the FillGenstep function
    torch.gentype = OpticksGenstep_TORCH;
    torch.trackid = 0;
    torch.matline = 0;
    torch.numphoton = n_photons;

    // Assign default values for position, time, momentum, and other attributes
    torch.pos = {-10.0f, -30.0f, -90.0f};
    torch.time = 0.0f;

    torch.mom = {0.0f, 0.3f, 1.0f};
    torch.mom = normalize(torch.mom);
    torch.weight = 0.0f;

    torch.pol = {1.0f, 0.0f, 0.0f};
    torch.wavelength = 420.0f;

    torch.zenith = {0.0f, 1.0f};
    torch.azimuth = {0.0f, 1.0f};

    torch.radius = 15.0f;
    torch.distance = 0.0f;
    torch.mode = 255;
    torch.type = T_DISC;

    cout << torch.desc() << endl;

    vector<sphoton> phs = generate_photons(torch, n_photons);

    size_t num_floats = phs.size()*4*4;
    float* data = reinterpret_cast<float*>(phs.data());
    NP* photons = NP::MakeFromValues<float>(data, num_floats);

    photons->reshape({ static_cast<int64_t>(phs.size()), 4, 4});
    photons->dump();
    photons->save("out/photons.npy");

    return EXIT_SUCCESS;
}
