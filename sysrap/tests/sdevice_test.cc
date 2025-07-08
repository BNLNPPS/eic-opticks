/**
sdevice_test.cc
==================

See notes in sdevice_test.sh 

**/


#include "sdevice.h"

int main(int argc, char** argv)
{  
    std::vector<sdevice> devs;

    bool nosave = true;
    sdevice::Visible(devs, nullptr, nosave);

    std::cout << sdevice::Desc( devs )
              << sdevice::Brief( devs ) << '\n'
              << sdevice::VRAM( devs );

    std::vector<sdevice> devs2;
    sdevice::Load(devs2, "./");

    std::cout << devs2.size() << '\n'
              << sdevice::Desc( devs2 )
              << sdevice::Brief( devs2 ) << '\n'
              << sdevice::VRAM( devs2 );

    return 0;
}
