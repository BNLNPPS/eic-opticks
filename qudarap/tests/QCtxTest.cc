#include "Opticks.hh"
#include "SPath.hh"
#include "NP.hh"
#include "GGeo.hh"
#include "GBndLib.hh"
#include "GScintillatorLib.hh"

#include "QRng.hh"
#include "QCtx.hh"
#include "scuda.h"

#include "OPTICKS_LOG.hh"

void test_wavelength(QCtx& qc)
{
    LOG(info); 

    unsigned num_wavelength = 1000000 ; 

    std::vector<float> wavelength ; 
    wavelength.resize(num_wavelength, 0.f); 

    qc.generate(   wavelength.data(), wavelength.size() ); 
    qc.dump(       wavelength.data(), wavelength.size() ); 
    NP::Write( "/tmp/QCtxTest", "wavelength.npy" ,  wavelength ); 
}

void test_photon(QCtx& qc)
{
    LOG(info); 
    unsigned num_photon = 100 ; 
    std::vector<quad4> photon ; 
    photon.resize(num_photon); 

    qc.generate(   photon.data(), photon.size() ); 
    qc.dump(       photon.data(), photon.size() ); 
    NP::Write( "/tmp/QCtxTest", "photon.npy" ,  (float*)photon.data(), photon.size(), 4, 4  ); 
}

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    Opticks ok(argc, argv); 
    ok.configure(); 

    GGeo* gg = GGeo::Load(&ok); 

    QCtx::Init(gg); 
    QCtx qc ;  

    test_wavelength(qc); 
    //test_photon(qc); 

    return 0 ; 
}
