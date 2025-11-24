#include <iostream>
#include <string>
#include <vector>

#include "eic-opticks/g4cx/G4CXOpticks.hh"
#include "eic-opticks/sysrap/NP.hh"
#include "eic-opticks/sysrap/sphoton.h"

#include "eic-opticks/config.h"
#include "eic-opticks/torch.h"

using namespace std;

int main(int argc, char **argv)
{
    gphox::Config config("dev");

    cout << config.torch.desc() << endl;

    vector<sphoton> phs = generate_photons(config.torch);

    size_t num_floats = phs.size() * 4 * 4;
    float *data = reinterpret_cast<float *>(phs.data());
    NP *photons = NP::MakeFromValues<float>(data, num_floats);

    photons->reshape({static_cast<int64_t>(phs.size()), 4, 4});
    photons->dump();
    photons->save("out/photons.npy");

    return EXIT_SUCCESS;
}
